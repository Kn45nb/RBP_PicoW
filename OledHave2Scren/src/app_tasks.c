#include "oled_tasks.h"

/*============================================================================================================================================================================
Phân lệnh và bộ đệm dùng chung cho tác vụ OLED
============================================================================================================================================================================*/

static ssd1306_t oled1;
static uint8_t frame[SSD1306_FRAME_SIZE];

diagnostic_status_t app_run_task(uint32_t task_id)
{
    // Chỉ một nhân dùng OLED và framebuffer tại một thời điểm.
    oled1.bus = I2C0_PORT;
    oled1.baud_rate = I2C0_BAUD_RATE;
    oled1.address = OLED1_ADDRESS;

    switch (task_id)
    {
        case TASK_OLED_SEQUENCE:
            return task1_run(&oled1, frame);
#ifdef PICO_PIO_TASKS
        case TASK_DMA_COPY:
            return task0_run();
        case TASK_OLED_CHECKERBOARD:
            return task2_run(&oled1, frame);
        case TASK_OLED_STRIPES:
            return task3_run(&oled1, frame);
#endif
        default:
            return ERROR_CORE1_NO_RESULT;
    }
}
