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
#include "hardware/uart.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

/*============================================================================================================================================================================
Cấu hình ngoại vi
============================================================================================================================================================================*/

// SPI0 dự phòng. Chân CS do GPIO điều khiển và ở mức cao khi rảnh.
#define SPI_PORT      spi0
#define SPI_MISO_PIN  16
#define SPI_CS_PIN    17
#define SPI_SCK_PIN   18
#define SPI_MOSI_PIN  19
#define SPI_BAUD_RATE 1000000

// I2C0 dành cho OLED 1.
#define I2C0_PORT      i2c0
#define I2C0_SDA_PIN   4
#define I2C0_SCL_PIN   5
#define I2C0_BAUD_RATE 400000

// I2C1 dành cho OLED 2.
#define I2C1_PORT      i2c1
#define I2C1_SDA_PIN   6
#define I2C1_SCL_PIN   7
#define I2C1_BAUD_RATE 400000

// UART1 dự phòng, cấu hình 8N1.
#define UART_PORT      uart1
#define UART_TX_PIN    8
#define UART_RX_PIN    9
#define UART_BAUD_RATE 115200

// LED báo lỗi dự phòng, nối qua điện trở xuống GND.
#define FALLBACK_LED_PIN 22

/*============================================================================================================================================================================
Cấu hình chẩn đoán
============================================================================================================================================================================*/

#define BLINK_PULSE_MS          100
#define BLINK_PAUSE_MS          1200
#define DMA_TIMEOUT_MS          10
#define CORE1_LAUNCH_TIMEOUT_MS 1000
#define CORE1_RESULT_TIMEOUT_US 1000000
#define CORE1_LAUNCH_MARKER     0x434F5231u

/*============================================================================================================================================================================
Mã lỗi và bộ đệm DMA
============================================================================================================================================================================*/

// Giá trị khác 0 là số nháy ngắn trong mỗi cụm.
typedef enum
{
    STATUS_OK = 0,
    ERROR_CYW43_INIT = 1,
    ERROR_DMA_NO_CHANNEL = 2,
    ERROR_DMA_TIMEOUT = 3,
    ERROR_DMA_DATA = 4,
    ERROR_CORE1_NO_RESULT = 5,
    ERROR_OLED_UI_TEST = 6
}
diagnostic_status_t;

static const char src[] = "Hello, world! (from DMA)";
static char dst[sizeof(src)];

/*============================================================================================================================================================================
UX/UI kiểm thử — Ảnh trong SRAM qua DMA lên OLED 1
============================================================================================================================================================================*/

#ifdef PICO_OLED_UI_TEST

#define UI_OLED_ADDRESS    0x3C
#define UI_OLED_WIDTH      128
#define UI_OLED_PAGES      8
#define UI_OLED_FRAME_SIZE (UI_OLED_WIDTH * UI_OLED_PAGES)
#define UI_OLED_TIMEOUT_US 50000
#define UI_OLED_PREVIEW_MS 1200

static uint8_t ui_frame[UI_OLED_FRAME_SIZE];
static uint16_t ui_dma_words[1 + UI_OLED_FRAME_SIZE];

static bool ui_test_write_commands(const uint8_t *commands, size_t size)
{
    return i2c_write_timeout_us(I2C0_PORT, UI_OLED_ADDRESS, commands, size, false, UI_OLED_TIMEOUT_US) == (int)size;
}

static bool ui_test_send_frame(const uint8_t frame[static UI_OLED_FRAME_SIZE])
{
    // Lệnh đặt cửa sổ ảnh cũng thiết lập địa chỉ đích cho giao dịch DMA tiếp theo.
    static const uint8_t window[] = {0x00, 0x21, 0, UI_OLED_WIDTH - 1, 0x22, 0, UI_OLED_PAGES - 1};
    if (!ui_test_write_commands(window, sizeof(window)))
    {
        return false;
    }

    int channel = dma_claim_unused_channel(false);
    if (channel < 0)
    {
        return false;
    }

    i2c_hw_t *hw = i2c_get_hw(I2C0_PORT);
    (void)hw->clr_stop_det;
    (void)hw->clr_tx_abrt;

    // Byte 0x40 mở luồng dữ liệu SSD1306. Chỉ word cuối yêu cầu phát STOP.
    ui_dma_words[0] = 0x40;
    for (size_t i = 0; i < UI_OLED_FRAME_SIZE; ++i)
    {
        ui_dma_words[i + 1] = frame[i];
    }
    ui_dma_words[UI_OLED_FRAME_SIZE] |= I2C_IC_DATA_CMD_STOP_BITS;

    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, i2c_get_dreq(I2C0_PORT, true));
    absolute_time_t deadline = make_timeout_time_us(UI_OLED_TIMEOUT_US);
    __compiler_memory_barrier();
    dma_channel_configure(channel, &config, &hw->data_cmd, ui_dma_words, count_of(ui_dma_words), true);

    bool complete = false;
    while (!time_reached(deadline))
    {
        uint32_t interrupts = hw->raw_intr_stat;
        if ((interrupts & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) ||
            (dma_channel_hw_addr(channel)->ctrl_trig & DMA_CH0_CTRL_TRIG_AHB_ERROR_BITS))
        {
            break;
        }
        if ((interrupts & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS) &&
            !dma_channel_is_busy(channel) && !(hw->status & I2C_IC_STATUS_MST_ACTIVITY_BITS))
        {
            complete = true;
            break;
        }
        tight_loop_contents();
    }

    if (!complete)
    {
        // Dừng DMA trước khi đặt lại I2C0 để loại bỏ dữ liệu còn trong FIFO.
        dma_channel_abort(channel);
        i2c_deinit(I2C0_PORT);
        i2c_init(I2C0_PORT, I2C0_BAUD_RATE);
    }
    else
    {
        (void)hw->clr_stop_det;
    }
    dma_channel_unclaim(channel);
    return complete;
}

static bool run_oled_ui_test(void)
{
    // Màn 128 × 64 dùng charge pump nội và chế độ địa chỉ ngang.
    static const uint8_t setup[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 63, 0xD3, 0, 0x40, 0x8D, 0x14,
        0x20, 0, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1,
        0xDB, 0x40, 0x2E, 0xA4, 0xA6
    };
    static const uint8_t display_on[] = {0x00, 0xAF};

    if (!ui_test_write_commands(setup, sizeof(setup)))
    {
        return false;
    }

    // Mẫu 1: ô bàn cờ 8 × 8 để kiểm tra thứ tự cột và page.
    for (size_t page = 0; page < UI_OLED_PAGES; ++page)
    {
        for (size_t x = 0; x < UI_OLED_WIDTH; ++x)
        {
            ui_frame[page * UI_OLED_WIDTH + x] = ((page + x / 8) & 1u) ? 0xFF : 0x00;
        }
    }
    if (!ui_test_send_frame(ui_frame) || !ui_test_write_commands(display_on, sizeof(display_on)))
    {
        return false;
    }
    sleep_ms(UI_OLED_PREVIEW_MS);

    // Mẫu 2: sọc ngang lệch pha giữa hai nửa màn để kiểm tra cập nhật ảnh.
    for (size_t i = 0; i < sizeof(ui_frame); ++i)
    {
        ui_frame[i] = (i % UI_OLED_WIDTH < UI_OLED_WIDTH / 2) ? 0x55 : 0xAA;
    }
    return ui_test_send_frame(ui_frame);
}

#endif

/*============================================================================================================================================================================
Nhân 1 — Khởi tạo ngoại vi và kiểm tra DMA
============================================================================================================================================================================*/

static diagnostic_status_t check_dma_copy(void)
{
    int channel = dma_claim_unused_channel(false);
    if (channel < 0)
    {
        return ERROR_DMA_NO_CHANNEL;
    }

    // Sao chép từng byte từ nguồn sang bộ đệm SRAM.
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, true);
    dma_channel_configure(channel, &config, dst, src, sizeof(src), true);

    absolute_time_t deadline = make_timeout_time_ms(DMA_TIMEOUT_MS);
    while (dma_channel_is_busy(channel))
    {
        if (time_reached(deadline))
        {
            // Giữ kênh và bộ đệm riêng khi DMA vẫn có thể ghi vào SRAM.
            return ERROR_DMA_TIMEOUT;
        }
        tight_loop_contents();
    }

    // Không cho trình biên dịch đọc dst trước khi DMA hoàn tất.
    __compiler_memory_barrier();
    dma_channel_unclaim(channel);
    if (memcmp(dst, src, sizeof(src)) != 0)
    {
        return ERROR_DMA_DATA;
    }
    return STATUS_OK;
}

static void init_i2c_bus(i2c_inst_t *bus, uint sda_pin, uint scl_pin, uint baud_rate)
{
    i2c_init(bus, baud_rate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

static void core1_entry(void)
{
    spi_init(SPI_PORT, SPI_BAUD_RATE);
    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);

    // Giữ CS ở mức cao khi chưa chọn thiết bị SPI.
    gpio_init(SPI_CS_PIN);
    gpio_put(SPI_CS_PIN, 1);
    gpio_set_dir(SPI_CS_PIN, GPIO_OUT);

    init_i2c_bus(I2C0_PORT, I2C0_SDA_PIN, I2C0_SCL_PIN, I2C0_BAUD_RATE);
    init_i2c_bus(I2C1_PORT, I2C1_SDA_PIN, I2C1_SCL_PIN, I2C1_BAUD_RATE);

    // Chuẩn bị UART1 để trao đổi dữ liệu khi cần.
    uart_init(UART_PORT, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    multicore_fifo_push_blocking((uint32_t)check_dma_copy());
    while (true)
    {
        __wfi();
    }
}

/*============================================================================================================================================================================
Nhân 0 — Báo lỗi bằng LED
============================================================================================================================================================================*/

static void init_fallback_led(void)
{
    gpio_init(FALLBACK_LED_PIN);
    gpio_put(FALLBACK_LED_PIN, 0);
    gpio_set_dir(FALLBACK_LED_PIN, GPIO_OUT);
}

static bool set_error_led(bool onboard, bool on)
{
    if (onboard)
    {
        return cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, on) == 0;
    }
    gpio_put(FALLBACK_LED_PIN, on);
    return true;
}

static bool blink_error_pulse(bool onboard)
{
    if (!set_error_led(onboard, true))
    {
        return false;
    }
    sleep_ms(BLINK_PULSE_MS);
    return set_error_led(onboard, false);
}

static _Noreturn void blink_error_forever(diagnostic_status_t status, bool onboard)
{
    if (!onboard)
    {
        init_fallback_led();
    }

    // Khoảng nghỉ dài phân cách hai cụm nháy liên tiếp.
    while (true)
    {
        for (unsigned int pulse = 0; pulse < (unsigned int)status; ++pulse)
        {
            if (pulse > 0)
            {
                sleep_ms(BLINK_PULSE_MS);
            }
            if (!blink_error_pulse(onboard))
            {
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
    bool core1_launch_timed_out = watchdog_enable_caused_reboot() && watchdog_hw->scratch[0] == CORE1_LAUNCH_MARKER;
    watchdog_hw->scratch[0] = 0;

    if (cyw43_arch_init() != 0 || !set_error_led(true, false))
    {
        blink_error_forever(ERROR_CYW43_INIT, false);
    }
    if (core1_launch_timed_out)
    {
        blink_error_forever(ERROR_CORE1_NO_RESULT, true);
    }

    // SDK chờ đồng bộ khi khởi chạy nhân 1. Watchdog giới hạn bước chờ này.
    watchdog_hw->scratch[0] = CORE1_LAUNCH_MARKER;
    watchdog_enable(CORE1_LAUNCH_TIMEOUT_MS, true);
    multicore_launch_core1(core1_entry);
    watchdog_disable();
    watchdog_hw->scratch[0] = 0;

    uint32_t status;
    if (!multicore_fifo_pop_timeout_us(CORE1_RESULT_TIMEOUT_US, &status) || status == ERROR_CYW43_INIT || status > ERROR_DMA_DATA)
    {
        blink_error_forever(ERROR_CORE1_NO_RESULT, true);
    }
    if (status != STATUS_OK)
    {
        blink_error_forever((diagnostic_status_t)status, true);
    }

#ifdef PICO_OLED_UI_TEST
    if (!run_oled_ui_test())
    {
        blink_error_forever(ERROR_OLED_UI_TEST, true);
    }
#endif

    // Kiểm tra thành công thì để LED tắt.
    while (true)
    {
        __wfi();
    }
}
