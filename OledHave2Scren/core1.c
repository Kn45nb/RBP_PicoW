/*============================================================================================================================================================================
Lib
============================================================================================================================================================================*/

#include "app_cores.h"
#include "app_hardware.h"
#include "pico/multicore.h"
#include "hardware/sync.h"
#ifdef PICO_PIO_TASKS
#include "app_tasks.h"
#endif

/*============================================================================================================================================================================
Nhân vật lý 1 — Chạy tác vụ được giao
============================================================================================================================================================================*/

void core1_entry(void)
{
#ifdef PICO_INIT_ON_CORE1
    multicore_fifo_push_blocking((uint32_t)app_prepare_hardware());
#endif
#ifdef PICO_PIO_TASKS
    while (true)
    {
        uint32_t task_id = multicore_fifo_pop_blocking();
        multicore_fifo_push_blocking((uint32_t)app_run_task(task_id));
    }
#else
    while (true)
    {
        __wfi();
    }
#endif
}
