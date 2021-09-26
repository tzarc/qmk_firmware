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

#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool (*painter_driver_init_func)(painter_device_t device, painter_rotation_t rotation);
typedef bool (*painter_driver_power_func)(painter_device_t device, bool power_on);
typedef bool (*painter_driver_clear_func)(painter_device_t device);
typedef bool (*painter_driver_flush_func)(painter_device_t device);
typedef bool (*painter_driver_viewport_func)(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
typedef bool (*painter_driver_pixdata_func)(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
typedef void (*painter_driver_convert_palette_func)(painter_device_t device, int16_t palette_size, qp_pixel_color_t *palette);
typedef void (*painter_driver_append_pixels)(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices);

// Driver vtable definition
struct painter_driver_vtable_t {
    painter_driver_init_func            init;
    painter_driver_power_func           power;
    painter_driver_clear_func           clear;
    painter_driver_flush_func           flush;
    painter_driver_viewport_func        viewport;
    painter_driver_pixdata_func         pixdata;
    painter_driver_convert_palette_func palette_convert;
    painter_driver_append_pixels        append_pixels;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Comms callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool (*painter_driver_comms_init_func)(painter_device_t device);
typedef bool (*painter_driver_comms_start_func)(painter_device_t device);
typedef void (*painter_driver_comms_stop_func)(painter_device_t device);
typedef uint32_t (*painter_driver_comms_send_func)(painter_device_t device, const void *data, uint32_t byte_count);

struct painter_comms_vtable_t {
    painter_driver_comms_init_func  comms_init;
    painter_driver_comms_start_func comms_start;
    painter_driver_comms_stop_func  comms_stop;
    painter_driver_comms_send_func  comms_send;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OLD TEMPORARY callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool (*painter_driver_drawimage_func)(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val);
typedef int16_t (*painter_driver_drawtext_func)(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg);

// Temporary vtable definition for APIs currently in transition
struct painter_driver_TEMP_FUNC_vtable_t {
    painter_driver_drawimage_func drawimage;
    painter_driver_drawtext_func  drawtext;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver base definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct painter_driver_t {
    const struct painter_driver_vtable_t QP_RESIDENT_FLASH *driver_vtable;
    const struct painter_comms_vtable_t QP_RESIDENT_FLASH *comms_vtable;
    const struct painter_driver_TEMP_FUNC_vtable_t QP_RESIDENT_FLASH *TEMP_vtable;

    // Number of bits per pixel, used for determining how many pixels can be sent during a transmission of the pixdata buffer
    uint8_t native_bits_per_pixel;

    // Comms config pointer -- needs to point to an appropriate comms config if the comms driver requires it.
    void *comms_config;

    // Flag signifying if validation was successful
    bool validate_ok;
};
