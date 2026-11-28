/*==============================================================================
 * Information
 * File         : .lib/ssd1308.h
 * Tools        : Arm GNU Toolchain 15.3.rel2, Ninja 1.13.2, CMake 4.5.1
 * SDK          : Pico SDK (pico-sdk-master)
 * Author       : @Kn45nb
 * Description  : Public interface for a 128 x 64 I2C SSD1308 OLED.
 *============================================================================*/

#ifndef SSD1308_H
#define SSD1308_H

/*------------------------------ Include Lib ---------------------------------*/
/* Base */
#include <stdbool.h>
#include <stdint.h>

/* Peripherals */
#include "hardware/i2c.h"

/*-------------------------------- Types -------------------------------------*/
typedef struct {
    i2c_inst_t *bus;       /* Initialized Pico I2C peripheral */
    uint8_t     address;   /* 7-bit I2C address */
} ssd1308_t;

/*------------------------------- Functions ----------------------------------*/
/* Configure the display. The panel stays off until the first frame is sent. */
bool ssd1308_init(const ssd1308_t *display);

/* Replace the entire frame with a centered digit and turn the panel on. */
bool ssd1308_show_digit(const ssd1308_t *display, uint8_t digit);

#endif