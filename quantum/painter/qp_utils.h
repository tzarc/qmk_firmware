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

#include <qp.h>
#include <qp_internal.h>
#include <color.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Generates a color-interpolated lookup table based off the number of items, from foreground to background, for use with monochrome image rendering
void qp_interpolate_palette(qp_pixel_color_t* lookup_table, int16_t items, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888);

// Convert from input pixel data + palette to equivalent pixels
typedef void (*pixel_output_callback)(qp_pixel_color_t color, void* cb_arg);
void qp_decode_palette(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* cb_arg);
void qp_decode_grayscale(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, pixel_output_callback output_callback, void* cb_arg);
void qp_decode_recolor(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* cb_arg);
