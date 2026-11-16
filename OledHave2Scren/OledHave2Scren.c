/*============================================================================================================================================================================
Info
============================================================================================================================================================================
SDK:		v2.3.0
Toolchain:	15.2Rel1
Ninja:		v1.13.2
CMake:		v4.4.0
Author:		@Kn45nb
============================================================================================================================================================================*/

/*============================================================================================================================================================================
Lib
============================================================================================================================================================================*/

#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

/*============================================================================================================================================================================
Cấu hình ngoại vi
============================================================================================================================================================================*/

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define FALLBACK_LED_PIN 6
#define BLINK_PULSE_MS 100
#define BLINK_PAUSE_MS 1200
#define DMA_TIMEOUT_MS 10
#define CORE1_LAUNCH_TIMEOUT_MS 1000
#define CORE1_RESULT_TIMEOUT_US 1000000
#define CORE1_LAUNCH_MARKER 0x434F5231u

/*============================================================================================================================================================================
Mã lỗi và bộ đệm DMA
============================================================================================================================================================================*/

// Giá trị khác 0 là số nháy ngắn trong mỗi cụm.
typedef enum {
    STATUS_OK = 0,
    ERROR_CYW43_INIT = 1,
    ERROR_DMA_NO_CHANNEL = 2,
    ERROR_DMA_TIMEOUT = 3,
    ERROR_DMA_DATA = 4,
    ERROR_CORE1_NO_RESULT = 5
} diagnostic_status_t;

static const char src[] = "Hello, world! (from DMA)";
static char dst[sizeof(src)];

/*============================================================================================================================================================================
Nhân 1 — Khởi tạo ngoại vi và kiểm tra DMA
============================================================================================================================================================================*/

static diagnostic_status_t check_dma_copy(void)
{
    int channel = dma_claim_unused_channel(false);
    if (channel < 0) {
        return ERROR_DMA_NO_CHANNEL;
    }

    // Sao chép từng byte từ nguồn sang bộ đệm SRAM.
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, true);
    dma_channel_configure(channel, &config, dst, src, sizeof(src), true);

    absolute_time_t deadline = make_timeout_time_ms(DMA_TIMEOUT_MS);
    while (dma_channel_is_busy(channel)) {
        if (time_reached(deadline)) {
            // Giữ kênh và bộ đệm riêng khi DMA vẫn có thể ghi vào SRAM.
            return ERROR_DMA_TIMEOUT;
        }
        tight_loop_contents();
    }

    // Không cho trình biên dịch đọc dst trước khi DMA hoàn tất.
    __compiler_memory_barrier();
    dma_channel_unclaim(channel);
    if (memcmp(dst, src, sizeof(src)) != 0) {
        return ERROR_DMA_DATA;
    }
    return STATUS_OK;
}

static void core1_entry(void)
{
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Giữ CS ở mức cao khi chưa chọn thiết bị SPI.
    gpio_init(PIN_CS);
    gpio_put(PIN_CS, 1);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    multicore_fifo_push_blocking((uint32_t)check_dma_copy());
    while (true) {
        __wfi();
    }
}

/*============================================================================================================================================================================
Nhân 0 — Báo lỗi bằng LED
============================================================================================================================================================================*/

static void init_fallback_led(void)
{
    // Mắc LED rời từ GP6 qua điện trở xuống GND.
    gpio_init(FALLBACK_LED_PIN);
    gpio_put(FALLBACK_LED_PIN, 0);
    gpio_set_dir(FALLBACK_LED_PIN, GPIO_OUT);
}

static bool set_error_led(bool onboard, bool on)
{
    if (onboard) {
        return cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, on) == 0;
    }
    gpio_put(FALLBACK_LED_PIN, on);
    return true;
}

static bool blink_error_pulse(bool onboard)
{
    if (!set_error_led(onboard, true)) {
        return false;
    }
    sleep_ms(BLINK_PULSE_MS);
    return set_error_led(onboard, false);
}

static _Noreturn void blink_error_forever(diagnostic_status_t status, bool onboard)
{
    if (!onboard) {
        init_fallback_led();
    }

    // Khoảng nghỉ dài phân cách hai cụm nháy liên tiếp.
    while (true) {
        for (unsigned int pulse = 0; pulse < (unsigned int)status; ++pulse) {
            if (pulse > 0) {
                sleep_ms(BLINK_PULSE_MS);
            }
            if (!blink_error_pulse(onboard)) {
                // Phát lại cả cụm trên LED dự phòng nếu ghi LED tích hợp thất bại.
                onboard = false;
                init_fallback_led();
                break;
            }
        }
        sleep_ms(BLINK_PAUSE_MS);
    }
}

int main(void)
{
    bool core1_launch_timed_out = watchdog_enable_caused_reboot() &&
                                  watchdog_hw->scratch[0] == CORE1_LAUNCH_MARKER;
    watchdog_hw->scratch[0] = 0;

    if (cyw43_arch_init() != 0 || !set_error_led(true, false)) {
        blink_error_forever(ERROR_CYW43_INIT, false);
    }
    if (core1_launch_timed_out) {
        blink_error_forever(ERROR_CORE1_NO_RESULT, true);
    }

    // SDK chờ đồng bộ khi khởi chạy nhân 1. Watchdog giới hạn bước chờ này.
    watchdog_hw->scratch[0] = CORE1_LAUNCH_MARKER;
    watchdog_enable(CORE1_LAUNCH_TIMEOUT_MS, true);
    multicore_launch_core1(core1_entry);
    watchdog_disable();
    watchdog_hw->scratch[0] = 0;

    uint32_t status;
    if (!multicore_fifo_pop_timeout_us(CORE1_RESULT_TIMEOUT_US, &status) ||
        status == ERROR_CYW43_INIT || status > ERROR_DMA_DATA) {
        blink_error_forever(ERROR_CORE1_NO_RESULT, true);
    }
    if (status != STATUS_OK) {
        blink_error_forever((diagnostic_status_t)status, true);
    }

    // Kiểm tra thành công thì để LED tắt.
    while (true) {
        __wfi();
    }
}
