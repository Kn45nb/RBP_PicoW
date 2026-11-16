#ifndef APP_CORES_H
#define APP_CORES_H

// Điểm vào của ứng dụng trên từng nhân vật lý.
_Noreturn void core0_entry(void);
#if defined(PICO_INIT_ON_CORE1) || defined(PICO_PIO_TASKS)
void core1_entry(void);
#endif

#endif
