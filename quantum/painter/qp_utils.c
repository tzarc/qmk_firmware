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

#ifdef QUANTUM_PAINTER_COMPRESSION_ENABLE
#    include "lzf.h"
#endif  // QUANTUM_PAINTER_COMPRESSION_ENABLE

#include "quantum.h"

#include "qp_utils.h"

#ifdef QUANTUM_PAINTER_COMPRESSION_ENABLE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compression Function
//
// NOTE: The variables in this section are intentionally outside a stack frame. They are able to be defined with larger
//       sizes than the normal stack frames would allow, and as such need to be external.
//
//       **** DO NOT refactor this and decide to place the variables inside the function calling them -- you will ****
//       **** very likely get artifacts rendered to the screen as a result.                                       ****
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Static buffer used for decompression
static uint8_t decompressed_buf[QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE];

uint32_t qp_decode(const uint8_t* const input_buffer, const uint32_t input_size, void* output_buffer, const uint32_t output_size) {
    // Use LZF decompressor to decode the chunk data
    return (uint32_t)lzf_decompress(input_buffer, input_size, output_buffer, output_size);
}

void qp_decode_chunks(const uint8_t* const compressed_data, uint32_t compressed_size, const uint32_t* const chunk_offsets, uint16_t chunk_count, void* cb_arg, void (*callback)(void* arg, uint16_t chunk_index, const uint8_t* const decoded_bytes, uint32_t byte_count)) {
    for (uint16_t i = 0; i < chunk_count; ++i) {
        // Check if we're the last chunk
        bool last_chunk = (i == (chunk_count - 1));
        // Work out the current chunk size
        uint32_t chunk_size = last_chunk ? (compressed_size - chunk_offsets[i])        // last chunk
                                         : (chunk_offsets[i + 1] - chunk_offsets[i]);  // any other chunk
        // Decode the image data
        uint32_t decompressed_size = qp_decode(&compressed_data[chunk_offsets[i]], chunk_size, decompressed_buf, sizeof(decompressed_buf));
        // Invoke the callback
        callback(cb_arg, i, decompressed_buf, decompressed_size);
    }
}
#endif  // QUANTUM_PAINTER_COMPRESSION_ENABLE

void qp_generate_palette(HSV* lookup_table, int16_t items, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg) {
    // Make sure we take the "shortest" route from one hue to the other
    if ((hue_fg - hue_bg) >= 128) {
        hue_bg += 256;
    } else if ((hue_fg - hue_bg) <= -128) {
        hue_bg -= 256;
    }

    // Interpolate each of the lookup table entries
    for (int16_t i = 0; i < items; ++i) {
        lookup_table[i].h = (uint8_t)((hue_fg - hue_bg) * i / (items - 1) + hue_bg);
        lookup_table[i].s = (uint8_t)((sat_fg - sat_bg) * i / (items - 1) + sat_bg);
        lookup_table[i].v = (uint8_t)((val_fg - val_bg) * i / (items - 1) + val_bg);
    }
}
