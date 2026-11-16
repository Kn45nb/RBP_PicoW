#include "oled_tasks.h"

/*============================================================================================================================================================================
Tác vụ 3 — Hiển thị sọc lệch pha
============================================================================================================================================================================*/

void task3_draw(uint8_t frame[static SSD1306_FRAME_SIZE])
{
    // Hai nửa màn có sọc ngang lệch pha.
    for (size_t i = 0; i < SSD1306_FRAME_SIZE; ++i)
    {
        frame[i] = (i % SSD1306_WIDTH < SSD1306_WIDTH / 2) ? 0x55 : 0xAA;
    }
}

diagnostic_status_t task3_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE])
{
    task3_draw(frame);
    return ssd1306_show_frame(display, frame) ? STATUS_OK : ERROR_OLED_UI_TEST;
}
