#include "display_dma.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"

/*============================================================================================================================================================================
Truyền I2C bằng DMA
============================================================================================================================================================================*/

bool display_dma_write(i2c_inst_t *bus, uint baud_rate, uint16_t words[static 1], size_t count, uint timeout_us)
{
    if (count == 0)
    {
        return false;
    }

    int channel = dma_claim_unused_channel(false);
    if (channel < 0)
    {
        return false;
    }

    i2c_hw_t *hw = i2c_get_hw(bus);
    (void)hw->clr_stop_det;
    (void)hw->clr_tx_abrt;

    // Chỉ word cuối yêu cầu phát STOP.
    words[count - 1] |= I2C_IC_DATA_CMD_STOP_BITS;

    dma_channel_config config = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, i2c_get_dreq(bus, true));
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    __compiler_memory_barrier();
    dma_channel_configure(channel, &config, &hw->data_cmd, words, count, true);

    bool complete = false;
    while (!time_reached(deadline))
    {
        uint32_t interrupts = hw->raw_intr_stat;
        if ((interrupts & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) ||
            (dma_channel_hw_addr(channel)->ctrl_trig & DMA_CH0_CTRL_TRIG_AHB_ERROR_BITS))
        {
            break;
        }
        if ((interrupts & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS) &&
            !dma_channel_is_busy(channel) && !(hw->status & I2C_IC_STATUS_MST_ACTIVITY_BITS))
        {
            complete = true;
            break;
        }
        tight_loop_contents();
    }

    if (!complete)
    {
        // Dừng DMA trước khi đặt lại bus để loại bỏ dữ liệu còn trong FIFO.
        dma_channel_abort(channel);
        i2c_deinit(bus);
        i2c_init(bus, baud_rate);
    }
    else
    {
        (void)hw->clr_stop_det;
    }
    dma_channel_unclaim(channel);
    return complete;
}
