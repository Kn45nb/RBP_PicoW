#include "oled_tasks.h"

/*============================================================================================================================================================================
Tác vụ 2 — Hiển thị bàn cờ
============================================================================================================================================================================*/

static void draw_checkerboard(uint8_t frame[static SSD1306_FRAME_SIZE])
{
    // Ô bàn cờ 8 × 8 dùng để kiểm tra thứ tự cột và page.
    for (size_t page = 0; page < SSD1306_PAGES; ++page)
    {
        for (size_t x = 0; x < SSD1306_WIDTH; ++x)
        {
            frame[page * SSD1306_WIDTH + x] = ((page + x / 8) & 1u) ? 0xFF : 0x00;
        }
    }
}

diagnostic_status_t task2_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE])
{
    draw_checkerboard(frame);
    return ssd1306_show_frame(display, frame) ? STATUS_OK : ERROR_OLED_UI_TEST;
}
