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
void qp_generate_palette(qp_pixel_color_t* lookup_table, int16_t items, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg);

// Convert from input pixel data + palette to equivalent pixels
void qp_decode_to_hsv(int16_t count, qp_image_format_t src_format, const void* src_data, qp_pixel_color_t* palette, qp_pixel_color_t* dest_colors);
