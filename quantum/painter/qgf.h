// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// See https://docs.qmk.fm/#/qgf for more information.

#include <stdint.h>
#include <stdbool.h>

#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QGF API

// Validate the image data
bool qgf_validate(const void QP_RESIDENT_FLASH_OR_RAM *buffer, uint32_t length);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QGF structures

/////////////////////////////////////////
// Common block header

typedef struct QP_PACKED qgf_block_header_v1_t {
    uint8_t  type_id;      // See each respective block type below.
    uint8_t  neg_type_id;  // Negated type ID, used for detecting parsing errors.
    uint32_t length : 24;  // 24-bit blob length, allowing for block sizes of a maximum of 16MB.
} qgf_block_header_v1_t;

_Static_assert(sizeof(qgf_block_header_v1_t) == 5, "qgf_block_header_v1_t must be 5 bytes in v1 of QGF");

/////////////////////////////////////////
// Graphics descriptor

#define QGF_GRAPHICS_DESCRIPTOR_TYPEID 0x00

typedef struct QP_PACKED qgf_graphics_descriptor_v1_t {
    qgf_block_header_v1_t header;        // = { .type_id = 0x00, .neg_type_id = (~0x00), .length = 10 }
    uint32_t              magic : 24;    // constant, equal to 0x464751 ("QGF")
    uint8_t               qgf_version;   // constant, equal to 0x01
    uint16_t              image_width;   // in pixels
    uint16_t              image_height;  // in pixels
    uint16_t              frame_count;   // minimum of 1
} qgf_graphics_descriptor_v1_t;

_Static_assert(sizeof(qgf_graphics_descriptor_v1_t) == 15, "qgf_graphics_descriptor_v1_t must be 15 bytes in v1 of QGF");

#define QGF_MAGIC 0x464751

/////////////////////////////////////////
// Frame offset descriptor

#define QGF_FRAME_OFFSET_DESCRIPTOR_TYPEID 0x01

typedef struct QP_PACKED qgf_frame_offsets_v1_t {
    qgf_block_header_v1_t header;     // = { .type_id = 0x01, .neg_type_id = (~0x01), .length = (N * sizeof(uint32_t)) }
    uint32_t              offset[0];  // '0' signifies that this struct is immediately followed by the frame offsets
} qgf_frame_offsets_v1_t;

_Static_assert(sizeof(qgf_frame_offsets_v1_t) == sizeof(qgf_block_header_v1_t), "qgf_frame_offsets_v1_t must only contain qgf_block_header_v1_t in v1 of QGF");

/////////////////////////////////////////
// Frame descriptor

#define QGF_FRAME_DESCRIPTOR_TYPEID 0x02

typedef struct QP_PACKED qgf_frame_v1_t {
    qgf_block_header_v1_t header;              // = { .type_id = 0x02, .neg_type_id = (~0x02), .length = 5 }
    qp_image_format_t     format;              // Frame format
    uint8_t               flags;               // Frame flags, see qp.h.
    painter_compression_t compression_scheme;  // Compression scheme, see qp.h.
    uint8_t               transparency_index;  // palette index used for transparent pixels
    uint16_t              delay;               // frame delay time for animations (in units of milliseconds)
} qgf_frame_v1_t;

_Static_assert(sizeof(qgf_frame_v1_t) == 11, "qgf_frame_v1_t must be 11 bytes in v1 of QGF");

#define QGF_FRAME_FLAG_DELTA 0x02
#define QGF_FRAME_FLAG_TRANSPARENT 0x01

/////////////////////////////////////////
// Frame palette descriptor

#define QGF_FRAME_PALETTE_DESCRIPTOR_TYPEID 0x03

typedef struct QP_PACKED qgf_palette_v1_t {
    qgf_block_header_v1_t header;  // = { .type_id = 0x03, .neg_type_id = (~0x03), .length = (N * 3 * sizeof(uint8_t)) }
    struct QP_PACKED {             // container for a single HSV palette entry
        uint8_t h;                 // hue component: `[0,360)` degrees is mapped to `[0,255]` uint8_t.
        uint8_t s;                 // saturation component: `[0,1]` is mapped to `[0,255]` uint8_t.
        uint8_t v;                 // value component: `[0,1]` is mapped to `[0,255]` uint8_t.
    } hsv[0];                      // N * hsv, where N is the number of palette entries depending on the frame format in the descriptor
} qgf_palette_v1_t;

_Static_assert(sizeof(qgf_palette_v1_t) == sizeof(qgf_block_header_v1_t), "qgf_palette_v1_t must only contain qgf_block_header_v1_t in v1 of QGF");

/////////////////////////////////////////
// Frame delta descriptor

#define QGF_FRAME_DELTA_DESCRIPTOR_TYPEID 0x04

typedef struct QP_PACKED qgf_delta_v1_t {
    qgf_block_header_v1_t header;  // = { .type_id = 0x04, .neg_type_id = (~0x04), .length = 8 }
    uint16_t              left;    // The left pixel location to draw the delta image
    uint16_t              top;     // The top pixel location to draw the delta image
    uint16_t              right;   // The right pixel location to to draw the delta image
    uint16_t              bottom;  // The bottom pixel location to to draw the delta image
} qgf_delta_v1_t;

_Static_assert(sizeof(qgf_delta_v1_t) == 13, "qgf_delta_v1_t must be 13 bytes in v1 of QGF");

/////////////////////////////////////////
// Frame data descriptor

#define QGF_FRAME_DATA_DESCRIPTOR_TYPEID 0x05

typedef struct QP_PACKED qgf_data_v1_t {
    qgf_block_header_v1_t header;   // = { .type_id = 0x05, .neg_type_id = (~0x05), .length = N }
    uint8_t               data[0];  // 0 signifies that this struct is immediately followed by the length of data specified in the header
} qgf_data_v1_t;

_Static_assert(sizeof(qgf_data_v1_t) == sizeof(qgf_block_header_v1_t), "qgf_data_v1_t must only contain qgf_block_header_v1_t in v1 of QGF");
