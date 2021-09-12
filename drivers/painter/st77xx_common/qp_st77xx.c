/* Copyright 2021 Paul Cotter (@gr1mr3aver)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <debug.h>
#include <print.h>
#include <color.h>
#include <utf8.h>
#include <spi_master.h>


#ifdef BACKLIGHT_ENABLE
#    include <backlight/backlight.h>
#endif

#include <qp.h>
#include <qp_internal.h>
#include <qp_utils.h>
#include <qp_fallback.h>
#include "qp_st77xx.h"
#include "qp_st77xx_internal.h"
#include "qp_st77xx_opcodes.h"

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))
#define LIMIT(x, limit_min, limit_max) (x > limit_max ? limit_max : (x < limit_min ? limit_min : x))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD control functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void qp_st77xx_internal_lcd_init(st77xx_painter_device_t *lcd) {
    setPinOutput(lcd->dc_pin);
    setPinOutput(lcd->reset_pin);
}

// Enable comms
void qp_st77xx_internal_lcd_start(st77xx_painter_device_t *lcd) {
    qp_start(lcd);
}

// Disable comms
void qp_st77xx_internal_lcd_stop(st77xx_painter_device_t *lcd) {
    qp_stop(lcd);
}

// Send a command
void qp_st77xx_internal_lcd_cmd(st77xx_painter_device_t *lcd, uint8_t b) {
    writePinLow(lcd->dc_pin);
    qp_send(lcd, &b, sizeof(b));
}

// Send data
void qp_st77xx_internal_lcd_sendbuf(st77xx_painter_device_t *lcd, const void *data, uint16_t len) {
    writePinHigh(lcd->dc_pin);
    qp_send(lcd, data, len);
}

// Send data (single byte)
void qp_st77xx_internal_lcd_data(st77xx_painter_device_t *lcd, uint8_t b) {
    qp_st77xx_internal_lcd_sendbuf(lcd, &b, sizeof(b));
}

// Set a register value
void qp_st77xx_internal_lcd_reg(st77xx_painter_device_t *lcd, uint8_t reg, uint8_t val) {
    qp_st77xx_internal_lcd_cmd(lcd, reg);
    qp_st77xx_internal_lcd_data(lcd, val);
}

// Set the drawing viewport area
void qp_st77xx_internal_lcd_viewport(st77xx_painter_device_t *lcd, uint16_t xbegin, uint16_t ybegin, uint16_t xend, uint16_t yend) {
    // Set up the x-window
    uint8_t xbuf[4] = {xbegin >> 8, xbegin & 0xFF, xend >> 8, xend & 0xFF};
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_SET_COL_ADDR);  // column address set
    qp_st77xx_internal_lcd_sendbuf(lcd, xbuf, sizeof(xbuf));

    // Set up the y-window
    uint8_t ybuf[4] = {ybegin >> 8, ybegin & 0xFF, yend >> 8, yend & 0xFF};
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_SET_ROW_ADDR);  // page (row) address set
    qp_st77xx_internal_lcd_sendbuf(lcd, ybuf, sizeof(ybuf));

    // Lock in the window
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_SET_MEM);  // enable memory writes
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter API implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Screen clear
bool qp_st77xx_clear(painter_device_t device) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;

    // Re-init the LCD
    lcd->qp_driver.init(device, lcd->rotation);

    return true;
}

// Power control -- on/off (will also handle backlight if set to use the normal QMK backlight driver)
bool qp_st77xx_power(painter_device_t device, bool power_on) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;
    qp_st77xx_internal_lcd_start(lcd);

    // Turn on/off the display
    qp_st77xx_internal_lcd_cmd(lcd, power_on ? ST77XX_CMD_DISPLAY_ON : ST77XX_CMD_DISPLAY_OFF);

    qp_st77xx_internal_lcd_stop(lcd);

#ifdef BACKLIGHT_ENABLE
    // If we're using the backlight to control the display as well, toggle that too.
    if (lcd->uses_backlight) {
        if (power_on) {
            // There's a small amount of time for the LCD to get the display back on the screen -- it's all white beforehand.
            // Delay for a small amount of time and let the LCD catch up before turning the backlight on.
            wait_ms(20);
            backlight_set(get_backlight_level());
        } else {
            backlight_set(0);
        }
    }
#endif

    return true;
}

// Viewport to draw to
bool qp_st77xx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;
    qp_st77xx_internal_lcd_start(lcd);

    // Configure where we're going to be rendering to
    qp_st77xx_internal_lcd_viewport(lcd, left, top, right, bottom);

    qp_st77xx_internal_lcd_stop(lcd);

    return true;
}
