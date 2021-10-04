/* Copyright 2021 Paul Cotter (@gr1mr3aver), Nick Brassel (@tzarc)
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

static inline bool qp_font_get_ascii_glyph(painter_font_t font, int32_t code_point, const painter_font_ascii_glyph_offset_t **glyph) {
    const painter_raw_font_descriptor_t *fdesc = (const painter_raw_font_descriptor_t *)font;
    if (code_point >= 0x20 && code_point < 0x7F) {
        if (fdesc->ascii_glyph_definitions != NULL) {
            // Search the font's ascii table
            uint8_t index = code_point - 0x20;
            *glyph        = &fdesc->ascii_glyph_definitions[index];
            return true;
        }
    }
    return false;
}

#ifdef UNICODE_ENABLE
static inline bool qp_font_get_unicode_glyph(painter_font_t font, int32_t code_point, const painter_font_unicode_glyph_offset_t **glyph) {
    const painter_raw_font_descriptor_t *fdesc = (const painter_raw_font_descriptor_t *)font;
    if (fdesc->unicode_glyph_definitions != NULL) {
        for (uint16_t index = 0; index < fdesc->unicode_glyph_count; ++index) {
            const painter_font_unicode_glyph_offset_t *glyph_desc = &fdesc->unicode_glyph_definitions[index];
            if (glyph_desc->unicode_glyph == code_point) {
                *glyph = glyph_desc;
                return true;
            }
        }
    }
    return false;
}
#endif  // UNICODE_ENABLE

typedef bool (*code_point_handler)(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg);

struct code_point_iter_calcwidth_state {
    int16_t width;
};

bool qp_font_code_point_handler_calcwidth(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg) {
    struct code_point_iter_calcwidth_state *state = (struct code_point_iter_calcwidth_state *)cb_arg;
    state->width += width;
    return true;
}

struct code_point_iter_drawglyph_state {
    painter_device_t           device;
    int16_t                    xpos;
    int16_t                    ypos;
    qp_pixel_color_t           fg_hsv888;
    qp_pixel_color_t           bg_hsv888;
    byte_input_callback        input_callback;
    struct byte_input_state *  input_state;
    struct pixel_output_state *output_state;
};

bool qp_font_code_point_handler_drawglyph(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg) {
    struct code_point_iter_drawglyph_state *state  = (struct code_point_iter_drawglyph_state *)cb_arg;
    struct painter_driver_t *               driver = (struct painter_driver_t *)state->device;
    const painter_raw_font_descriptor_t *   fdesc  = (const painter_raw_font_descriptor_t *)font;

    // Reset the input state
    state->input_state->src_data = &fdesc->image_data[offset];
    state->input_state->rle.mode = MARKER_BYTE;  // ignored if not using RLE

    // Reset the output state
    state->output_state->pixel_write_pos = 0;

    // Configure where we're going to be rendering to
    driver->driver_vtable->viewport(state->device, state->xpos, state->ypos, state->xpos + width - 1, state->ypos + height - 1);

    // Move the x-position for the next glyph
    state->xpos += width;

    // Decode the pixel data for the glyph
    uint32_t pixel_count = ((uint32_t)width) * height;
    bool     ret         = qp_decode_recolor(state->device, pixel_count, font->image_bpp, state->input_callback, state->input_state, state->fg_hsv888, state->bg_hsv888, qp_drawimage_pixel_appender, state->output_state);

    // Any leftovers need transmission as well.
    if (ret && state->output_state->pixel_write_pos > 0) {
        ret &= driver->driver_vtable->pixdata(state->device, qp_global_pixdata_buffer, state->output_state->pixel_write_pos);
    }

    return ret;
}

bool qp_iterate_code_points(painter_font_t font, const char *str, code_point_handler handler, void *cb_arg) {
    bool    ret        = true;
    int32_t code_point = 0;
    while (*str) {
        str = decode_utf8(str, &code_point);
        if (ret && code_point >= 0) {
            const painter_font_ascii_glyph_offset_t *ascii_glyph;
            if (qp_font_get_ascii_glyph(font, code_point, &ascii_glyph)) {
                ret = handler(font, code_point, ascii_glyph->width, font->glyph_height, ascii_glyph->offset, cb_arg);
                if (!ret) {
                    break;
                }
            }
#ifndef UNICODE_ENABLE
            // No unicode, missing glyph means failure
            else {
                ret = false;
                break;
            }
#else
            // Unicode enabled, not ascii -- try unicode instead
            else {
                const painter_font_unicode_glyph_offset_t *unicode_glyph;
                if (qp_font_get_unicode_glyph(font, code_point, &unicode_glyph)) {
                    ret = handler(font, code_point, unicode_glyph->width, font->glyph_height, unicode_glyph->offset, cb_arg);
                    if (!ret) {
                        break;
                    }
                } else {
                    ret = false;
                    break;
                }
            }
#endif  // UNICODE_ENABLE
        }
    }

    return ret;
}

int16_t qp_textwidth(painter_font_t font, const char *str) {
    struct code_point_iter_calcwidth_state state = {.width = 0};
    return qp_iterate_code_points(font, str, qp_font_code_point_handler_calcwidth, &state) ? state.width : 0;
}

int16_t qp_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str) { return qp_drawtext_recolor(device, x, y, font, str, 0, 0, 255, 0, 0, 0); }

int16_t qp_drawtext_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    qp_dprintf("qp_drawtext_recolor: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_drawtext_recolor: fail (validation_ok == false)\n");
        return 0;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_drawtext_recolor: fail (could not start comms)\n");
        return 0;
    }

    // Set up the input/output states
    struct byte_input_state input_state = {
        .device   = device,
        .src_data = NULL,
    };

    byte_input_callback input_callback;
    switch (font->compression) {
        case IMAGE_UNCOMPRESSED:
            input_callback = qp_drawimage_byte_uncompressed_decoder;
            break;
        case IMAGE_COMPRESSED_RLE:
            input_callback         = qp_drawimage_byte_rle_decoder;
            input_state.rle.mode   = MARKER_BYTE;
            input_state.rle.remain = 0;
            break;
        default:
            qp_dprintf("qp_drawimage_recolor: fail (invalid image compression scheme)\n");
            qp_comms_stop(device);
            return false;
    }

    struct pixel_output_state output_state = {.device = device, .pixel_write_pos = 0, .max_pixels = qp_num_pixels_in_buffer(device)};

    qp_pixel_color_t fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
    qp_pixel_color_t bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};

    struct code_point_iter_drawglyph_state state = {// Common
                                                    .device = device,
                                                    .xpos   = x,
                                                    .ypos   = y,
                                                    // Colors
                                                    .fg_hsv888 = fg_hsv888,
                                                    .bg_hsv888 = bg_hsv888,
                                                    // Input
                                                    .input_callback = input_callback,
                                                    .input_state    = &input_state,
                                                    // Output
                                                    .output_state = &output_state};

    bool ret = qp_iterate_code_points(font, str, qp_font_code_point_handler_drawglyph, &state);

    qp_dprintf("qp_drawtext_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret ? (state.xpos - x) : 0;
}
