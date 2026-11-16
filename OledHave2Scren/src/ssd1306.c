#include "ssd1306.h"
#include "display_dma.h"

/*============================================================================================================================================================================
Điều khiển OLED SSD1306
============================================================================================================================================================================*/

#define SSD1306_TIMEOUT_US 50000

static bool write_commands(const ssd1306_t *display, const uint8_t *commands, size_t size)
{
    return i2c_write_timeout_us(display->bus, display->address, commands, size, false, SSD1306_TIMEOUT_US) == (int)size;
}

static bool ssd1306_init(const ssd1306_t *display)
{
    // Màn 128 × 64 dùng charge pump nội và chế độ địa chỉ ngang.
    static const uint8_t setup[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 63, 0xD3, 0, 0x40, 0x8D, 0x14,
        0x20, 0, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1,
        0xDB, 0x40, 0x2E, 0xA4, 0xA6
    };

    return write_commands(display, setup, sizeof(setup));
}

static bool ssd1306_display_on(const ssd1306_t *display)
{
    static const uint8_t display_on[] = {0x00, 0xAF};

    return write_commands(display, display_on, sizeof(display_on));
}

bool ssd1306_write_frame(ssd1306_t *display, const uint8_t frame[static SSD1306_FRAME_SIZE])
{
    // Lệnh đặt cửa sổ ảnh cũng chọn địa chỉ đích cho giao dịch DMA tiếp theo.
    static const uint8_t window[] = {0x00, 0x21, 0, SSD1306_WIDTH - 1, 0x22, 0, SSD1306_PAGES - 1};
    if (!write_commands(display, window, sizeof(window)))
    {
        return false;
    }

    // Byte 0x40 mở luồng dữ liệu ảnh của SSD1306.
    display->dma_words[0] = 0x40;
    for (size_t i = 0; i < SSD1306_FRAME_SIZE; ++i)
    {
        display->dma_words[i + 1] = frame[i];
    }

    return display_dma_write(display->bus, display->baud_rate, display->dma_words,
                             1 + SSD1306_FRAME_SIZE, SSD1306_TIMEOUT_US);
}

bool ssd1306_show_frame(ssd1306_t *display, const uint8_t frame[static SSD1306_FRAME_SIZE])
{
    return ssd1306_init(display) && ssd1306_write_frame(display, frame) && ssd1306_display_on(display);
}
