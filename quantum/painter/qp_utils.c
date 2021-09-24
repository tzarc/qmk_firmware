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

#include <quantum.h>
#include <qp_utils.h>

void qp_interpolate_palette(qp_pixel_color_t* lookup_table, int16_t items, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888) {
    int16_t hue_fg = fg_hsv888.hsv888.h;
    int16_t hue_bg = bg_hsv888.hsv888.h;

    // Make sure we take the "shortest" route from one hue to the other
    if ((hue_fg - hue_bg) >= 128) {
        hue_bg += 256;
    } else if ((hue_fg - hue_bg) <= -128) {
        hue_bg -= 256;
    }

    // Interpolate each of the lookup table entries
    for (int16_t i = 0; i < items; ++i) {
        lookup_table[i].hsv888.h = (uint8_t)((hue_fg - hue_bg) * i / (items - 1) + hue_bg);
        lookup_table[i].hsv888.s = (uint8_t)((fg_hsv888.hsv888.s - bg_hsv888.hsv888.s) * i / (items - 1) + bg_hsv888.hsv888.s);
        lookup_table[i].hsv888.v = (uint8_t)((fg_hsv888.hsv888.v - bg_hsv888.hsv888.v) * i / (items - 1) + bg_hsv888.hsv888.v);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//
// NOTE: The variables in this section are intentionally outside a stack frame. They are able to be defined with larger
//       sizes than the normal stack frames would allow, and as such need to be external.
//
//       **** DO NOT refactor this and decide to place the variables inside the function calling them -- you will ****
//       **** very likely get artifacts rendered to the screen as a result.                                       ****
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Static buffer to contain a generated color palette
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
static qp_pixel_color_t hsv_lookup_table[256];
#else
static qp_pixel_color_t hsv_lookup_table[16];
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format codec
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void qp_decode_palette(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* cb_arg) {
    const uint8_t  pixel_bitmask    = (1 << bits_per_pixel) - 1;
    const uint8_t  pixels_per_byte  = 8 / bits_per_pixel;
    const uint8_t* pixdata          = (const uint8_t*)src_data;
    uint32_t       remaining_pixels = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte
    while (remaining_pixels > 0) {
        uint8_t pixval      = *pixdata;
        uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
        for (uint8_t q = 0; q < loop_pixels; ++q) {
            output_callback(palette[pixval & pixel_bitmask], cb_arg);
            pixval >>= bits_per_pixel;
        }
        ++pixdata;
        remaining_pixels -= loop_pixels;
    }
}

void qp_decode_grayscale(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, pixel_output_callback output_callback, void* cb_arg) {
    qp_pixel_color_t white = {.hsv888 = {.h = 0, .s = 0, .v = 255}};
    qp_pixel_color_t black = {.hsv888 = {.h = 0, .s = 0, .v = 0}};
    qp_decode_recolor(pixel_count, bits_per_pixel, src_data, white, black, output_callback, cb_arg);
}

void qp_decode_recolor(uint32_t pixel_count, uint8_t bits_per_pixel, const void* src_data, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* cb_arg) {
    static qp_pixel_color_t last_fg_hsv888 = {.hsv888 = {.h = 0x01, .s = 0x02, .v = 0x03}};  // unlikely color
    static qp_pixel_color_t last_bg_hsv888 = {.hsv888 = {.h = 0x01, .s = 0x02, .v = 0x03}};  // unlikely color
    if (memcmp(&fg_hsv888.hsv888, &last_fg_hsv888.hsv888, sizeof(fg_hsv888.hsv888)) || memcmp(&bg_hsv888.hsv888, &last_bg_hsv888.hsv888, sizeof(bg_hsv888.hsv888))) {
        qp_interpolate_palette(hsv_lookup_table, (1 << bits_per_pixel), fg_hsv888, bg_hsv888);
        last_fg_hsv888 = fg_hsv888;
        last_bg_hsv888 = bg_hsv888;
    }

    qp_decode_palette(pixel_count, bits_per_pixel, src_data, hsv_lookup_table, output_callback, cb_arg);
}

struct pixdata_state {
    painter_device_t device;
    uint16_t         write_pos;
};

static inline void lcd_append_pixel(qp_pixel_color_t color, void* cb_arg) {
    /*
    struct pixdata_state* state              = (struct pixdata_state*)cb_arg;
    pixdata_transmit_buf[state->write_pos++] = color.rgb565;

    // If we've hit the transmit limit, send out the entire buffer and reset
    if (state->write_pos == ILI9XXX_PIXDATA_BUFSIZE) {
        qp_comms_send(state->device, pixdata_transmit_buf, ILI9XXX_PIXDATA_BUFSIZE * sizeof(rgb565_t));
        state->write_pos = 0;
    }
    */
}

// Draw an image
bool qp_fallback_ili9xxx_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t* image, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    qp_comms_start(device);

    // Configure where we're going to be rendering to
    driver->driver_vtable->viewport(device, x, y, x + image->width - 1, y + image->height - 1);

    bool ret = false;
    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t* raw_image_desc = (const painter_raw_image_descriptor_t*)image;

        // Stream data to the LCD
        if (image->image_format == IMAGE_FORMAT_RGB565) {
            ret = driver->driver_vtable->pixdata(device, raw_image_desc->image_data, raw_image_desc->byte_count / sizeof(uint16_t));
        } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
        } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
        }
    }

    qp_comms_stop(device);
    return ret;
}
