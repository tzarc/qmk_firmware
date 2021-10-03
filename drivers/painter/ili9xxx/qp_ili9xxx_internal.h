/* Copyright 2021 Nick Brassel (@tzarc)
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

#include <qp.h>
#include <qp_comms_spi.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9xxx internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device callbacks
typedef void (*ili9xxx_send_cmd8_func)(painter_device_t device, uint8_t cmd);

// Device vtable
typedef struct ili9xxx_painter_device_vtable_t {
    ili9xxx_send_cmd8_func send_cmd8;
} ili9xxx_painter_device_vtable_t;

// Device definition
typedef struct ili9xxx_painter_device_t {
    struct painter_driver_t                      qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    const struct ili9xxx_painter_device_vtable_t QP_RESIDENT_FLASH *ili9xxx_vtable;

    union {
        struct qp_comms_spi_dc_reset_config_t spi_dc_reset_config;
        // TODO: I2C/parallel etc.
    };
    painter_rotation_t rotation;
} ili9xxx_painter_device_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_ili9xxx_power(painter_device_t device, bool power_on);
bool qp_ili9xxx_clear(painter_device_t device);
bool qp_ili9xxx_flush(painter_device_t device);
bool qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
bool qp_ili9xxx_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
bool qp_ili9xxx_palette_convert(painter_device_t driver, int16_t palette_size, qp_pixel_color_t *pixels);
bool qp_ili9xxx_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void     qp_ili9xxx_command(painter_device_t device, uint8_t cmd);
void     qp_ili9xxx_command_databyte(painter_device_t device, uint8_t cmd, uint8_t data);
uint32_t qp_ili9xxx_command_databuf(painter_device_t device, uint8_t cmd, const void *data, uint32_t byte_count);
