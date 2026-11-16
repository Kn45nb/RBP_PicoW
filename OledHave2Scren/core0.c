/*============================================================================================================================================================================
Lib
============================================================================================================================================================================*/

#include "app_config.h"
#include "app_cores.h"
#include "app_hardware.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/sync.h"
#if defined(PICO_INIT_ON_CORE1) || defined(PICO_PIO_TASKS)
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#endif
#ifdef PICO_PIO_TASKS
#include "hardware/pio.h"
#include "task_input.pio.h"
#endif
#ifdef PICO_OLED_UI_TEST
#include "app_tasks.h"
#endif

/*============================================================================================================================================================================
Nhân vật lý 0 — Điều phối ứng dụng và báo lỗi bằng LED
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

static bool blink_error_group(diagnostic_status_t status, bool onboard)
{
    for (unsigned int pulse = 0; pulse < (unsigned int)status; ++pulse)
    {
        if (pulse > 0)
        {
            sleep_ms(BLINK_PULSE_MS);
        }
        if (!blink_error_pulse(onboard))
        {
            return false;
        }
    }
    return true;
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
        if (!blink_error_group(status, onboard))
        {
            // Phát lại cả cụm trên LED dự phòng nếu ghi LED tích hợp thất bại.
            onboard = false;
            init_fallback_led();
        }
        sleep_ms(BLINK_PAUSE_MS);
    }
}

#if defined(PICO_INIT_ON_CORE1) || defined(PICO_PIO_TASKS)
static void launch_core1(void)
{
    // SDK chờ đồng bộ khi khởi chạy nhân 1. Watchdog giới hạn bước chờ này.
    watchdog_hw->scratch[0] = CORE1_LAUNCH_MARKER;
    watchdog_enable(CORE1_LAUNCH_TIMEOUT_MS, true);
    multicore_launch_core1(core1_entry);
    watchdog_disable();
    watchdog_hw->scratch[0] = 0;
}
#endif

#ifdef PICO_INIT_ON_CORE1
static diagnostic_status_t prepare_hardware_on_core1(void)
{
    launch_core1();
    uint32_t status;
    if (!multicore_fifo_pop_timeout_us(CORE1_RESULT_TIMEOUT_US, &status) || status == ERROR_CYW43_INIT || status > ERROR_DMA_DATA)
    {
        return ERROR_CORE1_NO_RESULT;
    }
    return (diagnostic_status_t)status;
}
#endif

#ifdef PICO_PIO_TASKS
_Static_assert(TASK_D1_PIN == TASK_D0_PIN + 1 && TASK_STROBE_PIN == TASK_D0_PIN + 2,
               "PIO task input needs three consecutive GPIOs");

static _Noreturn void dispatch_pio_tasks(void)
{
    PIO pio;
    uint sm;
    uint offset;
    if (!pio_claim_free_sm_and_add_program_for_gpio_range(&task_input_program, &pio, &sm, &offset, TASK_D0_PIN, 3, false))
    {
        blink_error_forever(ERROR_TASK_PIO_INIT, true);
    }

    for (uint pin = TASK_D0_PIN; pin <= TASK_STROBE_PIN; ++pin)
    {
        pio_gpio_init(pio, pin);
        gpio_pull_down(pin);
    }

    pio_sm_config config = task_input_program_get_default_config(offset);
    sm_config_set_in_pins(&config, TASK_D0_PIN);
    sm_config_set_in_shift(&config, false, false, 32);
    if (pio_sm_init(pio, sm, offset, &config) != PICO_OK ||
        pio_sm_set_consecutive_pindirs(pio, sm, TASK_D0_PIN, 3, false) != PICO_OK)
    {
        pio_remove_program_and_unclaim_sm(&task_input_program, pio, sm, offset);
        blink_error_forever(ERROR_TASK_PIO_INIT, true);
    }
    pio_sm_set_enabled(pio, sm, true);

#ifdef PICO_INIT_ON_CORE1
    bool core1_started = true;
#else
    bool core1_started = false;
#endif
    bool onboard_led = true;
    while (true)
    {
        while (pio_sm_is_rx_fifo_empty(pio, sm))
        {
            sleep_ms(1);
        }
        uint32_t task_id = pio_sm_get(pio, sm);
        if (!core1_started)
        {
            launch_core1();
            core1_started = true;
        }

        uint32_t result;
        if (!multicore_fifo_push_timeout_us(task_id, CORE1_RESULT_TIMEOUT_US) ||
            !multicore_fifo_pop_timeout_us(CORE1_TASK_TIMEOUT_US, &result) ||
            result == ERROR_CYW43_INIT || result == ERROR_CORE1_NO_RESULT || result > ERROR_OLED_UI_TEST)
        {
            blink_error_forever(ERROR_CORE1_NO_RESULT, onboard_led);
        }
        if (result != STATUS_OK && !blink_error_group((diagnostic_status_t)result, onboard_led))
        {
            onboard_led = false;
            init_fallback_led();
            blink_error_group((diagnostic_status_t)result, false);
        }
        if (result != STATUS_OK)
        {
            sleep_ms(BLINK_PAUSE_MS);
        }
    }
}
#endif

_Noreturn void core0_entry(void)
{
#if defined(PICO_INIT_ON_CORE1) || defined(PICO_PIO_TASKS)
    bool core1_launch_timed_out = watchdog_enable_caused_reboot() && watchdog_hw->scratch[0] == CORE1_LAUNCH_MARKER;
    watchdog_hw->scratch[0] = 0;
#endif
    if (cyw43_arch_init() != 0 || !set_error_led(true, false))
    {
        blink_error_forever(ERROR_CYW43_INIT, false);
    }
#if defined(PICO_INIT_ON_CORE1) || defined(PICO_PIO_TASKS)
    if (core1_launch_timed_out)
    {
        blink_error_forever(ERROR_CORE1_NO_RESULT, true);
    }
#endif

#ifdef PICO_INIT_ON_CORE1
    diagnostic_status_t status = prepare_hardware_on_core1();
#else
    diagnostic_status_t status = app_prepare_hardware();
#endif
    if (status != STATUS_OK)
    {
        blink_error_forever(status, true);
    }

#ifdef PICO_OLED_UI_TEST
    status = app_run_task(TASK_OLED_SEQUENCE);
    if (status != STATUS_OK)
    {
        blink_error_forever(status, true);
    }
#endif

#ifdef PICO_PIO_TASKS
    dispatch_pio_tasks();
#else
    // Kiểm tra thành công thì để LED tắt.
    while (true)
    {
        __wfi();
    }
#endif
}
