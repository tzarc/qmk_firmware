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

#include <qp_internal.h>
#include <qp_draw.h>
#include <qp_comms.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format decoder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_decode_palette(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* output_arg) {
    const uint8_t pixel_bitmask    = (1 << bits_per_pixel) - 1;
    const uint8_t pixels_per_byte  = 8 / bits_per_pixel;
    uint32_t      remaining_pixels = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte
    while (remaining_pixels > 0) {
        uint8_t byteval = input_callback(input_arg);
        if (byteval < 0) {
            return false;
        }
        uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
        for (uint8_t q = 0; q < loop_pixels; ++q) {
            if (!output_callback(palette, byteval & pixel_bitmask, output_arg)) {
                return false;
            }
            byteval >>= bits_per_pixel;
        }
        remaining_pixels -= loop_pixels;
    }
    return true;
}

bool qp_decode_grayscale(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, pixel_output_callback output_callback, void* output_arg) {
    qp_pixel_color_t white = {.hsv888 = {.h = 0, .s = 0, .v = 255}};
    qp_pixel_color_t black = {.hsv888 = {.h = 0, .s = 0, .v = 0}};
    return qp_decode_recolor(device, pixel_count, bits_per_pixel, input_callback, input_arg, white, black, output_callback, output_arg);
}

bool qp_decode_recolor(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* output_arg) {
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    int16_t                  steps  = 1 << bits_per_pixel;  // number of items we need to interpolate
    if (qp_interpolate_palette(fg_hsv888, bg_hsv888, steps)) {
        if (!driver->driver_vtable->palette_convert(device, steps, qp_global_pixel_lookup_table)) {
            return false;
        }
    }

    return qp_decode_palette(device, pixel_count, bits_per_pixel, input_callback, input_arg, qp_global_pixel_lookup_table, output_callback, output_arg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Progressive pull of bytes, push of pixels
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int16_t qp_drawimage_byte_uncompressed_decoder(void* cb_arg) {
    struct byte_input_state* state = (struct byte_input_state*)cb_arg;
    return *state->src_data++;
}

int16_t qp_drawimage_byte_rle_decoder(void* cb_arg) {
    struct byte_input_state* state = (struct byte_input_state*)cb_arg;

    // Work out if we're parsing the initial marker byte
    if (state->rle.mode == MARKER_BYTE) {
        uint8_t c = *state->src_data++;
        if (c >= 128) {
            state->rle.mode   = NON_REPEATING_RUN;  // non-repeated run
            state->rle.remain = c - 127;
        } else {
            state->rle.mode   = REPEATING_RUN;  // repeated run
            state->rle.remain = c;
        }
    }

    // Work out which byte we're returning
    uint8_t c = *state->src_data;

    // Advance the position if we're in a non-repeated run
    if (state->rle.mode == NON_REPEATING_RUN) {
        state->src_data++;
    }

    // Decrement the counter of the bytes remaining
    state->rle.remain--;
    if (state->rle.remain == 0) {
        // Advance the position if we're in a repeated run
        if (state->rle.mode == REPEATING_RUN) {
            state->src_data++;
        }

        // Swap back to querying the marker byte
        state->rle.mode = MARKER_BYTE;
    }

    return c;
}

bool qp_drawimage_pixel_appender(qp_pixel_color_t* palette, uint8_t index, void* cb_arg) {
    struct pixel_output_state* state  = (struct pixel_output_state*)cb_arg;
    struct painter_driver_t*   driver = (struct painter_driver_t*)state->device;

    if (!driver->driver_vtable->append_pixels(state->device, qp_global_pixdata_buffer, palette, state->pixel_write_pos++, 1, &index)) {
        return false;
    }

    // If we've hit the transmit limit, send out the entire buffer and reset the write position
    if (state->pixel_write_pos == state->max_pixels) {
        if (!driver->driver_vtable->pixdata(state->device, qp_global_pixdata_buffer, state->pixel_write_pos)) {
            return false;
        }
        state->pixel_write_pos = 0;
    }

    return true;
}
