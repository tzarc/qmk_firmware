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
#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Global variable used for native pixel data streaming.
extern uint8_t qp_global_pixdata_buffer[QP_PIXDATA_BUFFER_SIZE];

// Returns the number of pixels that can fit in the pixdata buffer
uint32_t qp_num_pixels_in_buffer(painter_device_t device);

// Fills the supplied buffer with equivalent native pixels matching the supplied HSV
void qp_fill_pixdata(painter_device_t device, uint32_t num_pixels, uint8_t hue, uint8_t sat, uint8_t val);

// qp_setpixel internal implementation, but uses the global pixdata buffer with pre-converted native pixel. Only the first pixel is used.
bool qp_setpixel_impl(painter_device_t device, uint16_t x, uint16_t y);

// qp_rect internal implementation, but uses the global pixdata buffer with pre-converted native pixels.
bool qp_fillrect_helper_impl(painter_device_t device, uint16_t l, uint16_t t, uint16_t r, uint16_t b);

// Generates a color-interpolated lookup table based off the number of items, from foreground to background, for use with monochrome image rendering
void qp_interpolate_palette(qp_pixel_color_t* lookup_table, int16_t items, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888);

// Convert from input pixel data + palette to equivalent pixels
typedef void (*pixel_output_callback)(qp_pixel_color_t color, void* cb_arg);
void qp_decode_palette(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* cb_arg);
void qp_decode_grayscale(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, pixel_output_callback output_callback, void* cb_arg);
void qp_decode_recolor(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* cb_arg);
