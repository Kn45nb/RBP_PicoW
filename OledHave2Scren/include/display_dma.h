#ifndef DISPLAY_DMA_H
#define DISPLAY_DMA_H

#include "hardware/i2c.h"

// Bus phải đang rảnh và đã chọn địa chỉ đích qua một giao dịch I2C trước đó.
// Bộ đệm word phải dành riêng cho DMA cho đến khi hàm trả về.
bool display_dma_write(i2c_inst_t *bus, uint baud_rate, uint16_t words[static 1], size_t count, uint timeout_us);

#endif
