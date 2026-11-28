/*==============================================================================
 * Information
 * File         : main.c
 * Target       : Raspberry Pi Pico W (RP2040)
 * IDE tools    : Arm GNU Toolchain 15.3.rel2, Ninja 1.13.2, CMake 4.5.1
 * SDK          : Pico SDK (pico-sdk-master)
 * Author       : @Kn45nb
 * Description  : Show digit 1 and digit 2 on two independent I2C OLEDs.
 *============================================================================*/

/*------------------------------ Include Lib ---------------------------------*/
/* Base */
#include <stdbool.h>

/* SDK Pico */
#include "pico/stdlib.h"

/* Project libs */
#include "ssd1308.h"

/* Peripherals */
#include "hardware/i2c.h"

/*-------------------------------- Defines -----------------------------------*/
#define OLED_I2C_BAUD_HZ        100000u  /* I2C bus rate [Hz] */

#define OLED_1_I2C              i2c0
#define OLED_1_SDA_GPIO         4u
#define OLED_1_SCL_GPIO         5u
#define OLED_1_ADDRESS          0x3Cu     /* 7-bit I2C address */

#define OLED_2_I2C              i2c1
#define OLED_2_SDA_GPIO         6u
#define OLED_2_SCL_GPIO         7u
#define OLED_2_ADDRESS          0x3Cu     /* 7-bit I2C address */

#define OLED_POWER_WAIT_MS      100u    /* Power-up settling time [ms] */

/*------------------------------- Variables ----------------------------------*/
static volatile bool oled_1_ok;
static volatile bool oled_2_ok;

/*------------------------------- Functions ----------------------------------*/
static void oled_bus_init(i2c_inst_t *bus, uint sda_gpio, uint scl_gpio)
{
    i2c_init(bus, OLED_I2C_BAUD_HZ);
    gpio_set_function(sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(scl_gpio, GPIO_FUNC_I2C);
}

int main(void)
{
    const ssd1308_t oled_1 = { .bus = OLED_1_I2C, .address = OLED_1_ADDRESS };
    const ssd1308_t oled_2 = { .bus = OLED_2_I2C, .address = OLED_2_ADDRESS };

    oled_bus_init(oled_1.bus, OLED_1_SDA_GPIO, OLED_1_SCL_GPIO);
    oled_bus_init(oled_2.bus, OLED_2_SDA_GPIO, OLED_2_SCL_GPIO);
    sleep_ms(OLED_POWER_WAIT_MS);

    oled_1_ok = ssd1308_init(&oled_1) && ssd1308_show_digit(&oled_1, 1u);
    oled_2_ok = ssd1308_init(&oled_2) && ssd1308_show_digit(&oled_2, 2u);

    for (;;) {
        sleep_ms(1000u);
    }
}