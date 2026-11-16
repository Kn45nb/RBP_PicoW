#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <stdint.h>
#include "app_config.h"

typedef enum
{
    TASK_DMA_COPY = 0,
    TASK_OLED_SEQUENCE = 1,
    TASK_OLED_CHECKERBOARD = 2,
    TASK_OLED_STRIPES = 3
}
app_task_id_t;

diagnostic_status_t task0_run(void);
// Chạy tuần tự. Bộ đệm OLED do bộ phân lệnh sở hữu.
diagnostic_status_t app_run_task(uint32_t task_id);

#endif
