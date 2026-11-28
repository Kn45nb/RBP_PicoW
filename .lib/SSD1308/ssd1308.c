/*==============================================================================
 * Information
 * File         : .lib/ssd1308.c
 * Tools        : Arm GNU Toolchain 15.3.rel2, Ninja 1.13.2, CMake 4.5.1
 * SDK          : Pico SDK (pico-sdk-master)
 * Author       : @Kn45nb
 * Description  : SSD1308 initialization and centered digit rendering.
 *============================================================================*/

/*------------------------------ Include Lib ---------------------------------*/
/* Base */
#include <stddef.h>

/* Project libs */
#include "ssd1308.h"

/*-------------------------------- Defines -----------------------------------*/
#define OLED_WIDTH              128u
#define OLED_HEIGHT             64u
#define OLED_PAGE_HEIGHT        8u
#define OLED_PAGE_COUNT         (OLED_HEIGHT / OLED_PAGE_HEIGHT)
#define OLED_COMMAND_CONTROL    0x00u
#define OLED_DATA_CONTROL       0x40u
#define OLED_I2C_TIMEOUT_US     30000u   /* Timeout per I2C write [us] */

#define DIGIT_COUNT             10u
#define DIGIT_WIDTH             5u
#define DIGIT_HEIGHT            7u
#define DIGIT_SCALE             8u
#define DIGIT_LEFT              ((OLED_WIDTH - DIGIT_WIDTH * DIGIT_SCALE) / 2u)
#define DIGIT_TOP               ((OLED_HEIGHT - DIGIT_HEIGHT * DIGIT_SCALE) / 2u)

/*------------------------------- Variables ----------------------------------*/
/* Each entry is one column. Bit 0 is the top pixel. */
static const uint8_t digit_font[DIGIT_COUNT][DIGIT_WIDTH] = {
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 0 */
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 1 */
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* 2 */
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, /* 3 */
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 4 */
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 5 */
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, /* 6 */
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* 7 */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, /* 9 */
};

/*------------------------------- Functions ----------------------------------*/
static bool oled_write(const ssd1308_t *display,
                       const uint8_t *packet,
                       size_t length)
{
    const int written = i2c_write_timeout_us(
        display->bus, display->address, packet, length, false,
        OLED_I2C_TIMEOUT_US
    );

    return written == (int)length;
}

bool ssd1308_init(const ssd1308_t *display)
{
    static const uint8_t commands[] = {
        OLED_COMMAND_CONTROL,
        0xAE,                      /* Display off */
        0xD5, 0x80,                /* Display clock */
        0xA8, OLED_HEIGHT - 1u,    /* Multiplex ratio for 64 rows */
        0xD3, 0x00,                /* Display offset */
        0x40,                      /* Display start line 0 */
        0x8D, 0x14,                /* Enable internal charge pump */
        0x20, 0x02,                /* Page addressing mode */
        0xA1,                      /* Segment remap */
        0xC8,                      /* Reversed COM scan direction */
        0xDA, 0x12,                /* COM pin configuration for 64 rows */
        0x81, 0x7F,                /* Contrast */
        0xD9, 0xF1,                /* Pre-charge period */
        0xDB, 0x40,                /* VCOMH deselect level */
        0xA4,                      /* Use display RAM */
        0xA6,                      /* Normal display */
        0x2E,                      /* Disable scroll */
    };

    return oled_write(display, commands, sizeof commands);
}

bool ssd1308_show_digit(const ssd1308_t *display, uint8_t digit)
{
    static const uint8_t display_on[] = { OLED_COMMAND_CONTROL, 0xAF };

    if (digit >= DIGIT_COUNT) {
        return false;
    }

    for (unsigned int page = 0; page < OLED_PAGE_COUNT; ++page) {
        const uint8_t set_page[] = {
            OLED_COMMAND_CONTROL,
            (uint8_t)(0xB0u + page),
            0x00,                  /* Column address low nibble */
            0x10,                  /* Column address high nibble */
        };
        uint8_t data[1u + OLED_WIDTH] = { OLED_DATA_CONTROL };

        for (unsigned int column = 0; column < DIGIT_WIDTH; ++column) {
            uint8_t pixels = 0;
            const uint8_t glyph_column = digit_font[digit][column];

            for (unsigned int bit = 0; bit < OLED_PAGE_HEIGHT; ++bit) {
                const unsigned int y = page * OLED_PAGE_HEIGHT + bit;

                if (y >= DIGIT_TOP && y < DIGIT_TOP + DIGIT_HEIGHT * DIGIT_SCALE) {
                    const unsigned int row = (y - DIGIT_TOP) / DIGIT_SCALE;

                    if ((glyph_column & (1u << row)) != 0u) {
                        pixels |= (uint8_t)(1u << bit);
                    }
                }
            }

            for (unsigned int dx = 0; dx < DIGIT_SCALE; ++dx) {
                const unsigned int x = DIGIT_LEFT + column * DIGIT_SCALE + dx;
                data[1u + x] = pixels;
            }
        }

        if (!oled_write(display, set_page, sizeof set_page) ||
            !oled_write(display, data, sizeof data)) {
            return false;
        }
    }

    return oled_write(display, display_on, sizeof display_on);
}