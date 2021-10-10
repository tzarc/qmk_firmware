// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum.h>
#include <utf8.h>

#include <qp_internal.h>
#include <qp_draw.h>
#include <qp_comms.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers

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

// Callback to be invoked for each codepoint detected in the UTF8 input string
typedef bool (*code_point_handler)(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg);

// Function to iterate over each UTF8 codepoint, invoking the callback for each decoded glyph
static inline bool qp_iterate_code_points(painter_font_t font, const char QP_RESIDENT_FLASH_OR_RAM *str, code_point_handler handler, void *cb_arg) {
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// String width calculation

// Callback state
struct code_point_iter_calcwidth_state {
    int16_t width;
};

// Codepoint handler callback: width calc
static inline bool qp_font_code_point_handler_calcwidth(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg) {
    struct code_point_iter_calcwidth_state *state = (struct code_point_iter_calcwidth_state *)cb_arg;

    // Increment the overall width by this glyph's width
    state->width += width;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// String drawing implementation

// Callback state
struct code_point_iter_drawglyph_state {
    painter_device_t                       device;
    int16_t                                xpos;
    int16_t                                ypos;
    qp_pixel_t                             fg_hsv888;
    qp_pixel_t                             bg_hsv888;
    qp_internal_byte_input_callback        input_callback;
    struct qp_internal_byte_input_state *  input_state;
    struct qp_internal_pixel_output_state *output_state;
};

// Codepoint handler callback: drawing
static inline bool qp_font_code_point_handler_drawglyph(painter_font_t font, uint32_t code_point, uint8_t width, uint8_t height, uint32_t offset, void *cb_arg) {
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
    bool     ret         = qp_internal_decode_recolor(state->device, pixel_count, font->image_bpp, state->input_callback, state->input_state, state->fg_hsv888, state->bg_hsv888, qp_internal_pixel_appender, state->output_state);

    // Any leftovers need transmission as well.
    if (ret && state->output_state->pixel_write_pos > 0) {
        ret &= driver->driver_vtable->pixdata(state->device, qp_internal_global_pixdata_buffer, state->output_state->pixel_write_pos);
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_textwidth

int16_t qp_textwidth(painter_font_t font, const char QP_RESIDENT_FLASH_OR_RAM *str) {
    // Create the codepoint iterator state
    struct code_point_iter_calcwidth_state state = {.width = 0};
    // Iterate each codepoint, return the calculated width if successful.
    return qp_iterate_code_points(font, str, qp_font_code_point_handler_calcwidth, &state) ? state.width : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawtext

int16_t qp_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char QP_RESIDENT_FLASH_OR_RAM *str) {
    // Offload to the recolor variant, substituting fg=white bg=black.
    // Traditional LCDs with those colours will need to manually invoke qp_drawtext_recolor with the colors reversed.
    return qp_drawtext_recolor(device, x, y, font, str, 0, 0, 255, 0, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawtext_recolor

int16_t qp_drawtext_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char QP_RESIDENT_FLASH_OR_RAM *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
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

    // Set up the byte input state and input callback
    struct qp_internal_byte_input_state input_state    = {.device = device, .src_data = NULL};
    qp_internal_byte_input_callback     input_callback = qp_internal_prepare_input_state(&input_state, font->compression);
    if (input_callback == NULL) {
        qp_dprintf("qp_drawtext_recolor: fail (invalid font compression scheme)\n");
        qp_comms_stop(device);
        return false;
    }

    // Set up the pixel output state
    struct qp_internal_pixel_output_state output_state = {.device = device, .pixel_write_pos = 0, .max_pixels = qp_internal_num_pixels_in_buffer(device)};

    // Set up the codepoint iteration state
    struct code_point_iter_drawglyph_state state = {// Common
                                                    .device = device,
                                                    .xpos   = x,
                                                    .ypos   = y,
                                                    // Colors
                                                    .fg_hsv888 = (qp_pixel_t){.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}},
                                                    .bg_hsv888 = (qp_pixel_t){.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}},
                                                    // Input
                                                    .input_callback = input_callback,
                                                    .input_state    = &input_state,
                                                    // Output
                                                    .output_state = &output_state};

    // Iterate the codepoints with the drawglyph callback
    bool ret = qp_iterate_code_points(font, str, qp_font_code_point_handler_drawglyph, &state);

    qp_dprintf("qp_drawtext_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret ? (state.xpos - x) : 0;
}
