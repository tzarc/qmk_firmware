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

#include "color.h"

#include "qp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter configurables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This value must match the chunk size used when converting images via qmk convert-image
#ifndef QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE
#    define QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE 128
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QUANTUM_PAINTER_COMPRESSION_ENABLE
// Decodes a compressed chunk of data
uint32_t qp_decode(const void* const input_buffer, const uint32_t input_size, void* output_buffer, const uint32_t output_size);
#endif  // QUANTUM_PAINTER_COMPRESSION_ENABLE

// Generates a color-interpolated lookup table based off the number of items, from foreground to background, for use with monochrome image rendering
void qp_generate_palette(HSV* lookup_table, int16_t items, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg);
