#ifndef OLED_TASKS_H
#define OLED_TASKS_H

#include "app_tasks.h"
#include "ssd1306.h"

diagnostic_status_t task1_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE]);
diagnostic_status_t task2_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE]);
diagnostic_status_t task3_run(ssd1306_t *display, uint8_t frame[static SSD1306_FRAME_SIZE]);

// Vẽ mẫu sọc để Task 1 có thể đổi ảnh mà không khởi tạo lại màn hình.
void task3_draw(uint8_t frame[static SSD1306_FRAME_SIZE]);

#endif
