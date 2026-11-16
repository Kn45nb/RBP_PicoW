#include <string.h>
#include "app_tasks.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define DMA_TIMEOUT_MS 10

/*============================================================================================================================================================================
Bộ đệm kiểm tra DMA
============================================================================================================================================================================*/

static const char src[] = "Hello, world! (from DMA)";
static char dst[sizeof(src)];

/*============================================================================================================================================================================
Tác vụ 0 — Kiểm tra DMA sao chép vào SRAM
============================================================================================================================================================================*/

diagnostic_status_t task0_run(void)
{
    int channel = dma_claim_unused_channel(false);
    if (channel < 0)
    {
        return ERROR_DMA_NO_CHANNEL;
    }

    // Sao chép từng byte từ nguồn sang bộ đệm SRAM.
    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, true);
    dma_channel_configure(channel, &config, dst, src, sizeof(src), true);

    absolute_time_t deadline = make_timeout_time_ms(DMA_TIMEOUT_MS);
    while (dma_channel_is_busy(channel))
    {
        if (time_reached(deadline))
        {
            // Dừng DMA trước khi trả kênh để lần kiểm thử sau dùng lại được.
            dma_channel_abort(channel);
            dma_channel_unclaim(channel);
            return ERROR_DMA_TIMEOUT;
        }
        tight_loop_contents();
    }

    // Không cho trình biên dịch đọc dst trước khi DMA hoàn tất.
    __compiler_memory_barrier();
    dma_channel_unclaim(channel);
    if (memcmp(dst, src, sizeof(src)) != 0)
    {
        return ERROR_DMA_DATA;
    }
    return STATUS_OK;
}
