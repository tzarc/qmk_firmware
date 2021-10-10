// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter pixel formats

// Datatype containing a pixel's colour
typedef union QP_PACKED qp_pixel_t {
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
} qp_pixel_t;
_Static_assert(sizeof(qp_pixel_t) == 4, "Invalid size for qp_pixel_t");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter image format

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

// Uncompressed raw image descriptor
typedef struct painter_raw_image_descriptor_t {
    const painter_image_descriptor_t base;
    const uint32_t                   byte_count;     // number of bytes in the image
    const uint8_t *const             image_palette;  // pointer to the image palette
    const uint8_t *const             image_data;     // pointer to the image data
} painter_raw_image_descriptor_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter font format

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
