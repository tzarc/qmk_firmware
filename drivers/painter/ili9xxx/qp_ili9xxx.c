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

#include <color.h>
#include <utf8.h>
#include <wait.h>
#include <spi_master.h>

#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_draw.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD control functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void qp_ili9xxx_cmd8(painter_device_t device, uint8_t cmd) {
    struct ili9xxx_painter_device_t *lcd = (struct ili9xxx_painter_device_t *)device;
    lcd->ili9xxx_vtable->send_cmd8(device, cmd);
}

void qp_ili9xxx_cmd8_data8(painter_device_t device, uint8_t cmd, uint8_t data) {
    qp_ili9xxx_cmd8(device, cmd);
    qp_comms_send(device, &data, sizeof(data));
}

uint32_t qp_ili9xxx_cmd8_databuf(painter_device_t device, uint8_t cmd, const void *data, uint32_t byte_count) {
    qp_ili9xxx_cmd8(device, cmd);
    return qp_comms_send(device, data, byte_count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Native pixel format conversion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Color conversion to LCD-native
static inline rgb565_t rgb_to_ili9xxx(uint8_t r, uint8_t g, uint8_t b) {
    rgb565_t rgb565 = (((rgb565_t)r) >> 3) << 11 | (((rgb565_t)g) >> 2) << 5 | (((rgb565_t)b) >> 3);
    return BYTE_SWAP(rgb565);
}

// Color conversion to LCD-native
static inline rgb565_t hsv_to_ili9xxx(uint8_t hue, uint8_t sat, uint8_t val) {
    RGB rgb = hsv_to_rgb_nocie((HSV){hue, sat, val});
    return rgb_to_ili9xxx(rgb.r, rgb.g, rgb.b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter API implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Power control
bool qp_ili9xxx_power(painter_device_t device, bool power_on) {
    qp_ili9xxx_cmd8(device, power_on ? ILI9XXX_CMD_DISPLAY_ON : ILI9XXX_CMD_DISPLAY_OFF);
    return true;
}

// Screen clear
bool qp_ili9xxx_clear(painter_device_t device) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->qp_driver.driver_vtable->init(device, lcd->rotation);  // Re-init the LCD
    return true;
}

// Screen flush
bool qp_ili9xxx_flush(painter_device_t device) {
    // No-op, as there's no framebuffer in RAM for this device.
    (void)device;
    return true;
}

// Viewport to draw to
bool qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    // Set up the x-window
    uint8_t xbuf[4] = {left >> 8, left & 0xFF, right >> 8, right & 0xFF};
    qp_ili9xxx_cmd8_databuf(device,
                            0x2A,  // column address set
                            xbuf, sizeof(xbuf));

    // Set up the y-window
    uint8_t ybuf[4] = {top >> 8, top & 0xFF, bottom >> 8, bottom & 0xFF};
    qp_ili9xxx_cmd8_databuf(device,
                            0x2B,  // page (row) address set
                            ybuf, sizeof(ybuf));

    // Lock in the window
    qp_ili9xxx_cmd8(device, 0x2C);  // enable memory writes
    return true;
}

// Stream pixel data to the current write position in GRAM
bool qp_ili9xxx_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qp_comms_send(device, pixel_data, native_pixel_count * sizeof(uint16_t));
    return true;
}

bool qp_ili9xxx_palette_convert(painter_device_t device, int16_t palette_size, qp_pixel_color_t *palette) {
    for (int16_t i = 0; i < palette_size; ++i) {
        palette[i].rgb565 = hsv_to_ili9xxx(palette[i].hsv888.h, palette[i].hsv888.s, palette[i].hsv888.v);
    }
    return true;
}

bool qp_ili9xxx_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices) {
    uint16_t *buf = (uint16_t *)target_buffer;
    for (uint32_t i = 0; i < pixel_count; ++i) {
        buf[pixel_offset + i] = palette[palette_indices[i]].rgb565;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OLD FOLLOWS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static globals
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

// Static buffer used for transmitting image data
static rgb565_t pixdata_transmit_buf[ILI9XXX_PIXDATA_BUFSIZE];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format image rendering
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct pixdata_state {
    painter_device_t device;
    uint16_t         write_pos;
};

static inline bool lcd_append_pixel(qp_pixel_color_t *palette, uint8_t index, void *cb_arg) {
    struct pixdata_state *state              = (struct pixdata_state *)cb_arg;
    pixdata_transmit_buf[state->write_pos++] = palette[index].rgb565;

    // If we've hit the transmit limit, send out the entire buffer and reset
    if (state->write_pos == ILI9XXX_PIXDATA_BUFSIZE) {
        qp_comms_send(state->device, pixdata_transmit_buf, ILI9XXX_PIXDATA_BUFSIZE * sizeof(rgb565_t));
        state->write_pos = 0;
    }

    return true;
}

static inline void lcd_decode_send_pixdata(painter_device_t device, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data) {
    // State describing the decoder
    struct pixdata_state state = {
        .device    = device,
        .write_pos = 0,
    };

    // Decode and transmit each block of pixels
    qp_decode_palette(device, pixel_count, bits_per_pixel, pixel_data, hsv_lookup_table, lcd_append_pixel, &state);

    // If we have a partial decode, send any remaining pixels
    if (state.write_pos > 0) {
        qp_comms_send(device, pixdata_transmit_buf, state.write_pos * sizeof(rgb565_t));
    }
}

// Recolored renderer
static inline void lcd_send_palette_pixdata(painter_device_t device, const uint8_t *const rgb_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    for (uint16_t i = 0; i < items; ++i) {
        hsv_lookup_table[i].rgb565 = rgb_to_ili9xxx(rgb_palette[i * 3 + 0], rgb_palette[i * 3 + 1], rgb_palette[i * 3 + 2]);
    }

    lcd_decode_send_pixdata(lcd, bits_per_pixel, pixel_count, pixel_data);
}

// Recolored renderer
static inline void lcd_send_mono_pixdata_recolor(painter_device_t device, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg) {
    ili9xxx_painter_device_t *lcd       = (ili9xxx_painter_device_t *)device;
    qp_pixel_color_t          fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
    qp_pixel_color_t          bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};

    int16_t steps = 1 << bits_per_pixel;  // number of items we need to interpolate
    if (qp_interpolate_palette(fg_hsv888, bg_hsv888, steps)) {
        qp_ili9xxx_palette_convert((painter_device_t)lcd, steps, qp_global_pixel_lookup_table);
    }

    lcd_decode_send_pixdata(lcd, bits_per_pixel, pixel_count, pixel_data);
}

// Uncompressed image drawing helper
static bool drawimage_uncompressed_impl(painter_device_t device, painter_image_format_t image_format, uint8_t image_bpp, const uint8_t *pixel_data, uint32_t byte_count, int32_t width, int32_t height, const uint8_t *palette_data, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    // Stream data to the LCD
    if (image_format == IMAGE_FORMAT_RAW) {
        // The pixel data is in the correct format already -- send it directly to the device
        qp_comms_send(device, pixel_data, width * height * sizeof(rgb565_t));
    } else if (image_format == IMAGE_FORMAT_GRAYSCALE) {
        // Supplied pixel data is in 4bpp monochrome -- decode it to the equivalent pixel data
        lcd_send_mono_pixdata_recolor(lcd, image_bpp, width * height, pixel_data, byte_count, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    } else if (image_format == IMAGE_FORMAT_PALETTE) {
        // Supplied pixel data is in 1bpp monochrome -- decode it to the equivalent pixel data
        lcd_send_palette_pixdata(lcd, palette_data, image_bpp, width * height, pixel_data, byte_count);
    }

    return true;
}

// Draw an image
bool qp_ili9xxx_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    qp_comms_start(device);

    // Configure where we're going to be rendering to
    qp_ili9xxx_viewport(lcd, x, y, x + image->width - 1, y + image->height - 1);

    bool ret = false;
    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        ret                                                  = drawimage_uncompressed_impl(lcd, image->image_format, image->image_bpp, raw_image_desc->image_data, raw_image_desc->byte_count, image->width, image->height, raw_image_desc->image_palette, hue, sat, val, hue, sat, 0);
    }

    qp_comms_stop(device);

    return ret;
}

int16_t qp_ili9xxx_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    ili9xxx_painter_device_t *           lcd   = (ili9xxx_painter_device_t *)device;
    const painter_raw_font_descriptor_t *fdesc = (const painter_raw_font_descriptor_t *)font;

    qp_comms_start(device);

    const char *c = str;
    while (*c) {
        int32_t code_point = 0;
        c                  = decode_utf8(c, &code_point);

        if (code_point >= 0) {
            if (code_point >= 0x20 && code_point < 0x7F) {
                if (fdesc->ascii_glyph_definitions != NULL) {
                    // Search the font's ascii table
                    uint8_t                                  index      = code_point - 0x20;
                    const painter_font_ascii_glyph_offset_t *glyph_desc = &fdesc->ascii_glyph_definitions[index];
                    uint16_t                                 byte_count = 0;
                    if (code_point < 0x7E) {
                        byte_count = (glyph_desc + 1)->offset - glyph_desc->offset;
                    } else if (code_point == 0x7E) {
#ifdef UNICODE_ENABLE
                        // Unicode glyphs directly follow ascii glyphs, so we take the first's offset
                        if (fdesc->unicode_glyph_count > 0) {
                            byte_count = fdesc->unicode_glyph_definitions[0].offset - glyph_desc->offset;
                        } else {
                            byte_count = fdesc->byte_count - glyph_desc->offset;
                        }
#else   // UNICODE_ENABLE
                        byte_count = fdesc->byte_count - glyph_desc->offset;
#endif  // UNICODE_ENABLE
                    }

                    qp_ili9xxx_viewport(lcd, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1);
                    drawimage_uncompressed_impl(lcd, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
                    x += glyph_desc->width;
                }
            }
#ifdef UNICODE_ENABLE
            else {
                // Search the font's unicode table
                if (fdesc->unicode_glyph_definitions != NULL) {
                    for (uint16_t index = 0; index < fdesc->unicode_glyph_count; ++index) {
                        const painter_font_unicode_glyph_offset_t *glyph_desc = &fdesc->unicode_glyph_definitions[index];
                        if (glyph_desc->unicode_glyph == code_point) {
                            uint16_t byte_count = (index == fdesc->unicode_glyph_count - 1) ? (fdesc->byte_count - glyph_desc->offset) : ((glyph_desc + 1)->offset - glyph_desc->offset);
                            qp_ili9xxx_viewport(lcd, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1);
                            drawimage_uncompressed_impl(lcd, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
                            x += glyph_desc->width;
                        }
                    }
                }
            }
#endif  // UNICODE_ENABLE
        }
    }

    qp_comms_stop(device);

    return (int16_t)x;
}
