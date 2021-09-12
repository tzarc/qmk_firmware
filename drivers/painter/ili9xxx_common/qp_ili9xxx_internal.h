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

#pragma once

#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9xxx internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device definition
typedef struct ili9xxx_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    bool                    allocated;
    pin_t                   reset_pin;
    pin_t                   dc_pin;
    painter_rotation_t      rotation;
} ili9xxx_painter_device_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool    qp_ili9xxx_clear(painter_device_t device);
bool    qp_ili9xxx_power(painter_device_t device, bool power_on);
bool    qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, bool render_continue);
bool    qp_ili9xxx_brightness(painter_device_t device, uint8_t val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_ili9xxx_internal_lcd_init(ili9xxx_painter_device_t *lcd);
bool qp_ili9xxx_internal_lcd_start(ili9xxx_painter_device_t *lcd);
bool qp_ili9xxx_internal_lcd_stop(ili9xxx_painter_device_t *lcd);
bool qp_ili9xxx_internal_lcd_cmd(ili9xxx_painter_device_t *lcd, uint8_t b);
bool qp_ili9xxx_internal_lcd_sendbuf(ili9xxx_painter_device_t *lcd, const void *data, uint16_t len);
bool qp_ili9xxx_internal_lcd_data(ili9xxx_painter_device_t *lcd, uint8_t b);
bool qp_ili9xxx_internal_lcd_reg(ili9xxx_painter_device_t *lcd, uint8_t reg, uint8_t val);
bool qp_ili9xxx_internal_lcd_viewport(ili9xxx_painter_device_t *lcd, uint16_t xbegin, uint16_t ybegin, uint16_t xend, uint16_t yend);
