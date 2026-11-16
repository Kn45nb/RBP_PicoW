/*============================================================================================================================================================================
Lib
============================================================================================================================================================================*/

#include "app_hardware.h"
#include "app_tasks.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

/*============================================================================================================================================================================
Khởi tạo ngoại vi
============================================================================================================================================================================*/

static void init_i2c_bus(i2c_inst_t *bus, uint sda_pin, uint scl_pin, uint baud_rate)
{
    i2c_init(bus, baud_rate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

diagnostic_status_t app_prepare_hardware(void)
{
    spi_init(SPI_PORT, SPI_BAUD_RATE);
    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);

    // Giữ CS ở mức cao khi chưa chọn thiết bị SPI.
    gpio_init(SPI_CS_PIN);
    gpio_put(SPI_CS_PIN, 1);
    gpio_set_dir(SPI_CS_PIN, GPIO_OUT);

    init_i2c_bus(I2C0_PORT, I2C0_SDA_PIN, I2C0_SCL_PIN, I2C0_BAUD_RATE);
    init_i2c_bus(I2C1_PORT, I2C1_SDA_PIN, I2C1_SCL_PIN, I2C1_BAUD_RATE);

    // Chuẩn bị UART1 để trao đổi dữ liệu khi cần.
    uart_init(UART_PORT, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    return task0_run();
}
