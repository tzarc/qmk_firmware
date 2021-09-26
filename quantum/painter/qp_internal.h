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

#pragma once

#include <quantum.h>
#include <qp.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum painter: helper defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define QP_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define QP_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum painter: cater for AVR address space
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define QP_PACKED __attribute__((packed))

#ifdef __FLASH
#    define QP_RESIDENT_FLASH __flash
#else
#    define QP_RESIDENT_FLASH
#endif

#ifdef __MEMX
#    define QP_RESIDENT_FLASH_OR_RAM __memx
#else
#    define QP_RESIDENT_FLASH_OR_RAM
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum painter types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum qp_image_format_t {
    // Pixel formats available in the QGF frame format
    GRAYSCALE_1BPP = 0x00,
    GRAYSCALE_2BPP = 0x01,
    GRAYSCALE_4BPP = 0x02,
    GRAYSCALE_8BPP = 0x03,
    PALETTE_1BPP   = 0x04,
    PALETTE_2BPP   = 0x05,
    PALETTE_4BPP   = 0x06,
    PALETTE_8BPP   = 0x07,
} qp_image_format_t;

// Datatype containing a pixel's colour
typedef union QP_PACKED qp_pixel_color_t {
    uint8_t mono;
    uint8_t palette_idx;

    struct QP_PACKED {
        uint8_t h;
        uint8_t s;
        uint8_t v;
    } hsv888;

    struct QP_PACKED {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgb888;

    uint16_t rgb565;

    uint32_t dummy;
} qp_pixel_color_t;
_Static_assert(sizeof(qp_pixel_color_t) == 4, "Invalid size for qp_pixel_color_t");

// Uncompressed raw image descriptor
typedef struct painter_raw_image_descriptor_t {
    const painter_image_descriptor_t base;
    const uint32_t                   byte_count;     // number of bytes in the image
    const uint8_t *const             image_palette;  // pointer to the image palette
    const uint8_t *const             image_data;     // pointer to the image data
} painter_raw_image_descriptor_t;

// ASCII font glyph offset descriptor
typedef struct painter_font_ascii_glyph_offset_t {
    uint32_t offset : 24;  // The offset into the data block where it starts
    uint8_t  width;        // The width of the glyph (in pixels)
} painter_font_ascii_glyph_offset_t;

#ifdef UNICODE_ENABLE
// Extra font glyph offset descriptor
typedef struct painter_font_unicode_glyph_offset_t {
    int32_t  unicode_glyph;  // The unicode glyph
    uint32_t offset : 24;    // The offset into the data block where it starts
    uint8_t  width;          // The width of the glyph (in pixels)
} painter_font_unicode_glyph_offset_t;
#endif  // UNICODE_ENABLE

// Uncompressed raw font descriptor
typedef struct painter_raw_font_descriptor_t {
    const painter_font_descriptor_t                base;
    const uint8_t *const                           image_palette;            // pointer to the image palette
    const uint8_t *const                           image_data;               // pointer to the image data
    const uint32_t                                 byte_count;               // number of bytes in the image
    const painter_font_ascii_glyph_offset_t *const ascii_glyph_definitions;  // list of offsets/widths for ASCII, range 0x20..0x7E
#ifdef UNICODE_ENABLE
    const painter_font_unicode_glyph_offset_t *const unicode_glyph_definitions;  // Unicode glyph descriptors for unicode rendering
    const uint16_t                                   unicode_glyph_count;        // Number of unicode glyphs defined
#endif                                                                           // UNICODE_ENABLE
} painter_raw_font_descriptor_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter driver callbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool (*painter_driver_init_func)(painter_device_t device, painter_rotation_t rotation);
typedef bool (*painter_driver_power_func)(painter_device_t device, bool power_on);
typedef bool (*painter_driver_clear_func)(painter_device_t device);
typedef bool (*painter_driver_viewport_func)(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
typedef bool (*painter_driver_pixdata_func)(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
typedef void (*painter_driver_convert_palette_func)(painter_device_t device, int16_t palette_size, qp_pixel_color_t *palette);
typedef void (*painter_driver_append_pixels)(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices);

typedef bool (*painter_driver_comms_init_func)(painter_device_t device);
typedef bool (*painter_driver_comms_start_func)(painter_device_t device);
typedef void (*painter_driver_comms_stop_func)(painter_device_t device);
typedef uint32_t (*painter_driver_comms_send_func)(painter_device_t device, const void *data, uint32_t byte_count);

typedef bool (*painter_driver_drawimage_func)(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val);
typedef int16_t (*painter_driver_drawtext_func)(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg);

// Driver vtable definition
struct painter_driver_vtable_t {
    painter_driver_init_func            init;
    painter_driver_power_func           power;
    painter_driver_clear_func           clear;
    painter_driver_viewport_func        viewport;
    painter_driver_pixdata_func         pixdata;
    painter_driver_convert_palette_func palette_convert;
    painter_driver_append_pixels        append_pixels;
};

struct painter_comms_vtable_t {
    painter_driver_comms_init_func  comms_init;
    painter_driver_comms_start_func comms_start;
    painter_driver_comms_stop_func  comms_stop;
    painter_driver_comms_send_func  comms_send;
};

// Temporary vtable definition for APIs currently in transition
struct painter_driver_TEMP_FUNC_vtable_t {
    painter_driver_drawimage_func drawimage;
    painter_driver_drawtext_func  drawtext;
};

// Driver base definition
struct painter_driver_t {
    const struct painter_driver_vtable_t QP_RESIDENT_FLASH *driver_vtable;
    const struct painter_comms_vtable_t QP_RESIDENT_FLASH *comms_vtable;
    const struct painter_driver_TEMP_FUNC_vtable_t QP_RESIDENT_FLASH *TEMP_vtable;

    uint8_t native_bits_per_pixel;
    void *  comms_config;
};
