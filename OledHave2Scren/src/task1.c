#include "oled_tasks.h"
#include "pico/stdlib.h"

/*============================================================================================================================================================================
Tác vụ 1 — Chạy hai mẫu OLED liên tiếp
============================================================================================================================================================================*/

#define OLED_PREVIEW_MS 1200

diagnostic_status_t task1_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE])
{
    diagnostic_status_t status = task2_run(display, frame);
    if (status != STATUS_OK)
    {
        return status;
    }

    sleep_ms(OLED_PREVIEW_MS);
    task3_draw(frame);
    return ssd1306_write_frame(display, frame) ? STATUS_OK : ERROR_OLED_UI_TEST;
}
