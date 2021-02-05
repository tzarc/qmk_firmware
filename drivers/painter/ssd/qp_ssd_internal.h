/* Copyright 2021 Michael Spradling <mike@mspradling.com>
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

#include <quantum.h>
#include <qp.h>
#include <qp_mono2_surface.h>

/*
 *Quantum Painter SSD types
 */

// Communication Bus
typedef enum { QP_BUS_SPI_3WIRE, QP_BUS_SPI_4WIRE , QP_BUS_I2C } painter_bus_t;

/*
 * Quantum Painter SSD internals
 */

/* Device definition */
typedef struct ssd_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    bool                    allocated;
    pin_t                   chip_select_pin;
    pin_t                   data_pin;
    pin_t                   reset_pin;
    uint16_t                spi_divisor;
    uint8_t                 i2c_addr;
    painter_bus_t           bus;
    painter_rotation_t      rotation;
    painter_device_t        fb;
} ssd_painter_device_t;

/*
 * Device Forward declarations
 */
bool qp_ssd_flush(painter_device_t device);
bool qp_ssd_clear(painter_device_t device);
bool qp_ssd_power(painter_device_t device, bool power_on);
bool qp_ssd_brightness(painter_device_t device, uint8_t val);
bool qp_ssd_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
bool qp_ssd_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
bool qp_ssd_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
bool qp_ssd_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val);
bool qp_ssd_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val);
bool qp_ssd_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_ssd_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_ssd_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_ssd_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val);
bool qp_ssd_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg);

/*
 * Low-level LCD Forward Declarations
 */
void qp_ssd_internal_lcd_init(ssd_painter_device_t *lcd);
void qp_ssd_internal_lcd_start(ssd_painter_device_t *lcd);
void qp_ssd_internal_lcd_stop(ssd_painter_device_t *lcd);
void qp_ssd_internal_lcd_cmd_set(ssd_painter_device_t *lcd, uint8_t cmd);
void qp_ssd_internal_lcd_cmd_set_val(ssd_painter_device_t *lcd, uint8_t cmd, uint8_t val);
void qp_ssd_internal_lcd_cmd(ssd_painter_device_t *lcd, const uint8_t *data, uint16_t len);
void qp_ssd_internal_lcd_data(ssd_painter_device_t *lcd, const uint8_t *data, uint16_t len);
