/* Copyright 2020 Nick Brassel (@tzarc)
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


#include <print.h>
#include <color.h>
#include <utf8.h>
#include <wait.h>

#ifdef BACKLIGHT_ENABLE
#    include <backlight/backlight.h>
#endif

#include <qp_internal.h>
#include <qp_utils.h>
#include <qp_ili9xxx.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD control functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialize the lcd
bool qp_ili9xxx_internal_lcd_init(ili9xxx_painter_device_t *lcd) {
    setPinOutput(lcd->dc_pin);
    setPinOutput(lcd->reset_pin);
    return true;
}

// Enable comms
bool qp_ili9xxx_internal_lcd_start(ili9xxx_painter_device_t *lcd) {
    return qp_start(lcd);
}

// Disable comms
bool qp_ili9xxx_internal_lcd_stop(ili9xxx_painter_device_t *lcd) {
    return qp_stop(lcd);
}

// Send a command
bool qp_ili9xxx_internal_lcd_cmd(ili9xxx_painter_device_t *lcd, uint8_t command) {
    writePinLow(lcd->dc_pin);
    return qp_send(lcd, &command, sizeof(command));
}

// Send data
bool qp_ili9xxx_internal_lcd_sendbuf(ili9xxx_painter_device_t *lcd, const void *data, uint16_t len) {
    writePinHigh(lcd->dc_pin);
    return qp_send(lcd, data, len);
}

// Send data (single byte)
bool qp_ili9xxx_internal_lcd_data(ili9xxx_painter_device_t *lcd, uint8_t b) {
    return qp_ili9xxx_internal_lcd_sendbuf(lcd, &b, sizeof(b));
}

// Set a register value
bool qp_ili9xxx_internal_lcd_reg(ili9xxx_painter_device_t *lcd, uint8_t reg, uint8_t val) {
    return (qp_ili9xxx_internal_lcd_cmd(lcd, reg) && qp_ili9xxx_internal_lcd_data(lcd, val));
}

// Set the drawing viewport area
bool qp_ili9xxx_internal_lcd_viewport(ili9xxx_painter_device_t *lcd, uint16_t xbegin, uint16_t ybegin, uint16_t xend, uint16_t yend) {
    // Set up the x-window
    uint8_t xbuf[4] = {xbegin >> 8, xbegin & 0xFF, xend >> 8, xend & 0xFF};
    // Set up the y-window
    uint8_t ybuf[4] = {ybegin >> 8, ybegin & 0xFF, yend >> 8, yend & 0xFF};

    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_COL_ADDR); // column address set
    qp_ili9xxx_internal_lcd_sendbuf(lcd, xbuf, sizeof(xbuf));
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_PAGE_ADDR);  // page (row) address set
    qp_ili9xxx_internal_lcd_sendbuf(lcd, ybuf, sizeof(ybuf));
    // Lock in the window
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_MEM);  // enable memory writes
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter API implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Screen clear
bool qp_ili9xxx_clear(painter_device_t device) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;

    // Re-init the LCD
//    lcd->qp_driver.init(device, lcd->rotation);
    return qp_rect(lcd, 0, 0, lcd->qp_driver.screen_width - 1, lcd->qp_driver.screen_height -1, HSV_BLACK, true, false);

}

bool qp_ili9xxx_brightness(painter_device_t device, uint8_t val) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    qp_ili9xxx_internal_lcd_start(lcd);

    qp_ili9xxx_internal_lcd_reg(lcd, ILI9XXX_SET_BRIGHTNESS, val);

    qp_ili9xxx_internal_lcd_stop(lcd);
    return true;
}

// Power control -- on/off (will also handle backlight if set to use the normal QMK backlight driver)
bool qp_ili9xxx_power(painter_device_t device, bool power_on) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    qp_ili9xxx_internal_lcd_start(lcd);
    wait_ms(20);

    bool retval;
    retval = qp_ili9xxx_internal_lcd_cmd(lcd, power_on ? ILI9XXX_CMD_DISPLAY_ON : ILI9XXX_CMD_DISPLAY_OFF);
    qp_ili9xxx_internal_lcd_stop(lcd);

    // There's a small amount of time for the LCD to get the display back on the screen -- it's all white beforehand.
    // Delay for a small amount of time and let the LCD catch up before turning the backlight on.
    wait_ms(20);

    // Turn on/off the display
#ifdef BACKLIGHT_ENABLE
    // If we're using the backlight to control the display as well, toggle that too.
    if (retval && lcd->uses_backlight) {
        if (power_on) {
            backlight_set(get_backlight_level());
        } else {
            backlight_set(0);
        }
    }
#endif
    return retval;
}

// Viewport to draw to
bool qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, bool render_continue) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    if(!lcd->qp_driver.render_started) qp_ili9xxx_internal_lcd_start(lcd);
#ifdef CONSOLE_ENABLE
        dprintf("opening viewport at %i, %i, %i, %i\n", left, top, right, bottom);
#endif

    // Configure where we're going to be rendering to
    qp_ili9xxx_internal_lcd_viewport(lcd, left, top, right, bottom);

    if(!render_continue) qp_ili9xxx_internal_lcd_stop(lcd);

    return true;
}


