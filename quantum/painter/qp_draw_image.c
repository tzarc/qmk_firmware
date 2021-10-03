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

#include <quantum.h>
#include <utf8.h>

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
// Renderer internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum rle_mode_t {
    MARKER_BYTE,
    REPEATING_RUN,
    NON_REPEATING_RUN,
};

struct byte_input_state {
    painter_device_t device;
    const uint8_t QP_RESIDENT_FLASH_OR_RAM* src_data;

    // RLE-specific
    enum rle_mode_t rle_mode;
    uint8_t         rle_remain;  // number of bytes remaining in the current mode
};

static inline int16_t qp_drawimage_byte_uncompressed_decoder(void* cb_arg) {
    struct byte_input_state* state = (struct byte_input_state*)cb_arg;
    return *state->src_data++;
}

static inline int16_t qp_drawimage_byte_rle_decoder(void* cb_arg) {
    struct byte_input_state* state = (struct byte_input_state*)cb_arg;

    // Work out if we're parsing the initial marker byte
    if (state->rle_mode == MARKER_BYTE) {
        uint8_t c = *state->src_data++;
        if (c >= 128) {
            state->rle_mode   = NON_REPEATING_RUN;  // non-repeated run
            state->rle_remain = c - 127;
        } else {
            state->rle_mode   = REPEATING_RUN;  // repeated run
            state->rle_remain = c;
        }
    }

    // Work out which byte we're returning
    uint8_t c = *state->src_data;

    // Advance the position if we're in a non-repeated run
    if (state->rle_mode == NON_REPEATING_RUN) {
        state->src_data++;
    }

    // Decrement the counter of the bytes remaining
    state->rle_remain--;
    if (state->rle_remain == 0) {
        // Advance the position if we're in a repeated run
        if (state->rle_mode == REPEATING_RUN) {
            state->src_data++;
        }

        // Swap back to querying the marker byte
        state->rle_mode = MARKER_BYTE;
    }

    return c;
}

struct pixel_output_state {
    painter_device_t device;
    uint32_t         pixel_write_pos;
    uint32_t         max_pixels;
};

static inline bool qp_drawimage_pixel_appender(qp_pixel_color_t* palette, uint8_t index, void* cb_arg) {
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Image renderer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image) { return qp_drawimage_recolor(device, x, y, image, 0, 0, 255, 0, 0, 0); }

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    qp_dprintf("qp_drawimage_recolor: entry\n");
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_drawimage_recolor: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_drawimage_recolor: fail (could not start comms)\n");
        return false;
    }

    // Configure where we're going to be rendering to
    if (!driver->driver_vtable->viewport(device, x, y, x + image->width - 1, y + image->height - 1)) {
        qp_dprintf("qp_drawimage_recolor: fail (could not set viewport)\n");
        qp_comms_stop(device);
        return false;
    }

    const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM* raw_image_desc = (const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM*)image;

    // Work out byte input function to use
    byte_input_callback input_callback;
    switch (image->compression) {
        case IMAGE_UNCOMPRESSED:
            input_callback = qp_drawimage_byte_uncompressed_decoder;
            break;
        case IMAGE_COMPRESSED_RLE:
            input_callback = qp_drawimage_byte_rle_decoder;
            break;
        default:
            qp_dprintf("qp_drawimage_recolor: fail (invalid image compression scheme)\n");
            qp_comms_stop(device);
            return false;
    }

    // Set up the input/output states
    struct byte_input_state input_state = {
        .device     = device,
        .src_data   = raw_image_desc->image_data,
        .rle_mode   = 0,
        .rle_remain = 0,
    };
    struct pixel_output_state output_state = {.device = device, .pixel_write_pos = 0, .max_pixels = qp_num_pixels_in_buffer(device)};

    // Stream data to the LCD
    bool ret = false;
    if (image->image_format == IMAGE_FORMAT_RAW) {
        // Transmit the raw data.
        ret = driver->driver_vtable->pixdata(device, raw_image_desc->image_data, ((uint32_t)image->width) * image->height);
    } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
        // Specify the fg/bg
        qp_pixel_color_t fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
        qp_pixel_color_t bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};

        // Decode the pixel data
        ret = qp_decode_recolor(device, ((uint32_t)image->width) * image->height, image->image_bpp, input_callback, &input_state, fg_hsv888, bg_hsv888, qp_drawimage_pixel_appender, &output_state);

        // Any leftovers need transmission as well.
        if (ret && output_state.pixel_write_pos > 0) {
            ret &= driver->driver_vtable->pixdata(device, qp_global_pixdata_buffer, output_state.pixel_write_pos);
        }
    } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
        // Read the palette entries
        const uint8_t QP_RESIDENT_FLASH_OR_RAM* rgb_palette     = raw_image_desc->image_palette;
        uint16_t                                palette_entries = 1 << image->image_bpp;
        for (uint16_t i = 0; i < palette_entries; ++i) {
            qp_global_pixel_lookup_table[i] = (qp_pixel_color_t){.hsv888 = {.h = rgb_palette[i * 3 + 0], .s = rgb_palette[i * 3 + 1], .v = rgb_palette[i * 3 + 2]}};
        }

        // Convert the palette to native format
        if (!driver->driver_vtable->palette_convert(device, palette_entries, qp_global_pixel_lookup_table)) {
            qp_dprintf("qp_drawimage_recolor: fail (could not set viewport)\n");
            qp_comms_stop(device);
            return false;
        }

        // Decode the pixel data
        ret = qp_decode_palette(device, ((uint32_t)image->width) * image->height, image->image_bpp, input_callback, &input_state, qp_global_pixel_lookup_table, qp_drawimage_pixel_appender, &output_state);

        // Any leftovers need transmission as well.
        if (ret && output_state.pixel_write_pos > 0) {
            ret &= driver->driver_vtable->pixdata(device, qp_global_pixdata_buffer, output_state.pixel_write_pos);
        }
    }

    qp_dprintf("qp_drawimage_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}
