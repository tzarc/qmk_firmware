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

#include "qp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter st7735r internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device definition
typedef struct st7735r_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    bool                    allocated;
    pin_t                   chip_select_pin;
    pin_t                   data_pin;
    pin_t                   reset_pin;
    uint16_t                spi_divisor;
    painter_rotation_t      rotation;
#ifdef BACKLIGHT_ENABLE
    bool uses_backlight;
#endif
} st7735r_painter_device_t;

// Colour definition
typedef uint16_t rgb565_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_st7735r_clear(painter_device_t device);
bool qp_st7735r_power(painter_device_t device, bool power_on);
bool qp_st7735r_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
bool qp_st7735r_pixdata(painter_device_t device, const void *pixel_data, uint32_t byte_count);
bool qp_st7735r_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val);
bool qp_st7735r_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val);
bool qp_st7735r_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_st7735r_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void qp_st7735r_internal_lcd_start(st7735r_painter_device_t *lcd);
void qp_st7735r_internal_lcd_stop(void);
void qp_st7735r_internal_lcd_cmd(st7735r_painter_device_t *lcd, uint8_t b);
void qp_st7735r_internal_lcd_sendbuf(st7735r_painter_device_t *lcd, const void *data, uint16_t len);
void qp_st7735r_internal_lcd_data(st7735r_painter_device_t *lcd, uint8_t b);
void qp_st7735r_internal_lcd_reg(st7735r_painter_device_t *lcd, uint8_t reg, uint8_t val);
void qp_st7735r_internal_lcd_viewport(st7735r_painter_device_t *lcd, uint16_t xbegin, uint16_t ybegin, uint16_t xend, uint16_t yend);
