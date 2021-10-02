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

void qp_decode_palette(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* cb_arg) {
    const uint8_t  pixel_bitmask    = (1 << bits_per_pixel) - 1;
    const uint8_t  pixels_per_byte  = 8 / bits_per_pixel;
    const uint8_t* pixdata          = (const uint8_t*)src_data;
    uint32_t       remaining_pixels = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte
    while (remaining_pixels > 0) {
        uint8_t pixval      = *pixdata;
        uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
        for (uint8_t q = 0; q < loop_pixels; ++q) {
            output_callback(palette, pixval & pixel_bitmask, cb_arg);
            pixval >>= bits_per_pixel;
        }
        ++pixdata;
        remaining_pixels -= loop_pixels;
    }
}

void qp_decode_grayscale(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, pixel_output_callback output_callback, void* cb_arg) {
    qp_pixel_color_t white = {.hsv888 = {.h = 0, .s = 0, .v = 255}};
    qp_pixel_color_t black = {.hsv888 = {.h = 0, .s = 0, .v = 0}};
    qp_decode_recolor(device, pixel_count, bits_per_pixel, src_data, white, black, output_callback, cb_arg);
}

void qp_decode_recolor(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* cb_arg) {
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    int16_t                  steps  = 1 << bits_per_pixel;  // number of items we need to interpolate
    if (qp_interpolate_palette(fg_hsv888, bg_hsv888, steps)) {
        driver->driver_vtable->palette_convert(device, steps, qp_global_pixel_lookup_table);
    }

    qp_decode_palette(device, pixel_count, bits_per_pixel, src_data, qp_global_pixel_lookup_table, output_callback, cb_arg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderer internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct decoder_state {
    painter_device_t device;
    uint16_t         write_pos;
};

static inline void qp_drawimage_append_pixel(qp_pixel_color_t* palette, uint8_t index, void* cb_arg) {
    struct decoder_state*    state  = (struct decoder_state*)cb_arg;
    struct painter_driver_t* driver = (struct painter_driver_t*)state->device;

    driver->driver_vtable->append_pixels(state->device, qp_global_pixdata_buffer, palette, state->write_pos++, 1, &index);

    // If we've hit the transmit limit, send out the entire buffer and reset
    if (state->write_pos == qp_num_pixels_in_buffer(state->device)) {
        driver->driver_vtable->pixdata(state->device, qp_global_pixdata_buffer, state->write_pos);
        state->write_pos = 0;
    }
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

    bool ret = false;
    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM* raw_image_desc = (const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM*)image;
        // Stream data to the LCD
        if (image->image_format == IMAGE_FORMAT_RGB565) {
            ret = driver->driver_vtable->pixdata(device, raw_image_desc->image_data, raw_image_desc->byte_count / sizeof(uint16_t));
        } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
            qp_pixel_color_t     fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
            qp_pixel_color_t     bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};
            struct decoder_state state     = {.device = device, .write_pos = 0};
            qp_decode_recolor(device, image->width * image->height, image->image_bpp, raw_image_desc->image_data, fg_hsv888, bg_hsv888, qp_drawimage_append_pixel, &state);
            if (state.write_pos > 0) {
                driver->driver_vtable->pixdata(device, qp_global_pixdata_buffer, state.write_pos);
            }
        } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
        }
    }

    qp_dprintf("qp_drawimage_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}
