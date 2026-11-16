#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"

#define SSD1306_WIDTH      128
#define SSD1306_PAGES      8
#define SSD1306_FRAME_SIZE (SSD1306_WIDTH * SSD1306_PAGES)

typedef struct
{
    i2c_inst_t *bus;
    uint baud_rate;
    uint8_t address;
    uint16_t dma_words[1 + SSD1306_FRAME_SIZE];
}
ssd1306_t;

bool ssd1306_write_frame(ssd1306_t *display, const uint8_t frame[static SSD1306_FRAME_SIZE]);
// Khởi tạo và hiện ảnh đầu tiên. Bus I2C phải được cấu hình trước.
bool ssd1306_show_frame(ssd1306_t *display, const uint8_t frame[static SSD1306_FRAME_SIZE]);

#endif
