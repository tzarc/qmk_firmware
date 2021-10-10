// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

// See https://docs.qmk.fm/#/qgf for more information.

#include <qp_stream.h>
#include <qgf.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QGF API

// Ensure
static bool qgf_validate_header(qgf_block_header_v1_t *desc, uint8_t expected_typeid, int32_t expected_length) {
    if (desc->type_id != expected_typeid || desc->neg_type_id != ~expected_typeid) {
        return false;
    }

    if (expected_length >= 0 && desc->length != expected_length) {
        return false;
    }

    return true;
}

static void qgf_seek_to_frame(qp_stream_t *stream, uint16_t frame_number) {
    // Seek to the position in the frame offset block to read the frame offset
    qp_stream_setpos(stream, sizeof(qgf_graphics_descriptor_v1_t) + sizeof(qgf_frame_offsets_v1_t) + (frame_number * sizeof(uint32_t)));

    // Read the offset
    uint32_t offset = 0;
    qp_stream_read(&offset, sizeof(uint32_t), 1, stream);

    // Move to the offset
    qp_stream_setpos(stream, offset);
}

static bool qgf_validate_stream(qp_stream_t *stream) {
    // Validate the graphics descriptor
    qgf_graphics_descriptor_v1_t graphics_descriptor;
    if (qp_stream_read(&graphics_descriptor, sizeof(qgf_graphics_descriptor_v1_t), 1, stream) != sizeof(qgf_graphics_descriptor_v1_t)) {
        return false;
    }

    if (!qgf_validate_header(&graphics_descriptor.header, QGF_GRAPHICS_DESCRIPTOR_TYPEID, (sizeof(qgf_graphics_descriptor_v1_t) - sizeof(qgf_block_header_v1_t)))) {
        return false;
    }

    if (graphics_descriptor.magic != QGF_MAGIC || graphics_descriptor.qgf_version != 0x01) {
        return false;
    }

    // Read and validate the frame offset block
    {
        qgf_frame_offsets_v1_t frame_offsets;
        if (qp_stream_read(&frame_offsets, sizeof(qgf_frame_offsets_v1_t), 1, stream) != sizeof(qgf_frame_offsets_v1_t)) {
            return false;
        }

        if (!qgf_validate_header(&frame_offsets.header, QGF_FRAME_OFFSET_DESCRIPTOR_TYPEID, (graphics_descriptor.frame_count * sizeof(uint32_t)))) {
            return false;
        }
    }

    // Read and validate all the frames
    for (uint16_t i = 0; i < graphics_descriptor.frame_count; ++i) {
        // Seek to the correct location
        qgf_seek_to_frame(stream, i);

        // Read and validate the frame descriptor
        qgf_frame_v1_t frame_descriptor;
        if (qp_stream_read(&frame_descriptor, sizeof(qgf_frame_v1_t), 1, stream) != sizeof(qgf_frame_v1_t)) {
            return false;
        }

        if (!qgf_validate_header(&frame_descriptor.header, QGF_FRAME_DESCRIPTOR_TYPEID, (sizeof(qgf_frame_v1_t) - sizeof(qgf_block_header_v1_t)))) {
            return false;
        }

        // Work out if we expect a delta block
        bool has_delta_block = (frame_descriptor.flags & QGF_FRAME_FLAG_DELTA) == QGF_FRAME_FLAG_DELTA;

        // Work out if we expect a palette block
        uint8_t bpp;
        bool    has_palette_block;
        switch (frame_descriptor.format) {
            case GRAYSCALE_1BPP:
                bpp               = 1;
                has_palette_block = false;
                break;
            case GRAYSCALE_2BPP:
                bpp               = 2;
                has_palette_block = false;
                break;
            case GRAYSCALE_4BPP:
                bpp               = 4;
                has_palette_block = false;
                break;
            case GRAYSCALE_8BPP:
                bpp               = 8;
                has_palette_block = false;
                break;
            case PALETTE_1BPP:
                bpp               = 1;
                has_palette_block = true;
                break;
            case PALETTE_2BPP:
                bpp               = 2;
                has_palette_block = true;
                break;
            case PALETTE_4BPP:
                bpp               = 4;
                has_palette_block = true;
                break;
            case PALETTE_8BPP:
                bpp               = 8;
                has_palette_block = true;
                break;
            default:
                return false;
        }

        // Read and validate the delta block if required
        if (has_delta_block) {
            qgf_delta_v1_t delta_descriptor;
            if (qp_stream_read(&delta_descriptor, sizeof(qgf_delta_v1_t), 1, stream) != sizeof(qgf_delta_v1_t)) {
                return false;
            }

            if (!qgf_validate_header(&delta_descriptor.header, QGF_FRAME_DELTA_DESCRIPTOR_TYPEID, (sizeof(qgf_delta_v1_t) - sizeof(qgf_block_header_v1_t)))) {
                return false;
            }
        }

        // Read and validate the palette block if required
        if (has_palette_block) {
            qgf_palette_v1_t palette_descriptor;
            if (qp_stream_read(&palette_descriptor, sizeof(qgf_palette_v1_t), 1, stream) != sizeof(qgf_palette_v1_t)) {
                return false;
            }

            // BPP determines the number of palette entries, each entry is a HSV888 triplet.
            uint32_t expected_length = (1 << bpp) * (sizeof(uint8_t) * 3);

            if (!qgf_validate_header(&palette_descriptor.header, QGF_FRAME_PALETTE_DESCRIPTOR_TYPEID, expected_length)) {
                return false;
            }
        }

        // Read and validate the data block
        qgf_data_v1_t data_descriptor;
        if (qp_stream_read(&data_descriptor, sizeof(qgf_data_v1_t), 1, stream) != sizeof(qgf_data_v1_t)) {
            return false;
        }

        if (!qgf_validate_header(&data_descriptor.header, QGF_FRAME_DATA_DESCRIPTOR_TYPEID, -1)) {
            return false;
        }
    }

    return true;
}

// Validate the image data
bool qgf_validate_mem(const void QP_RESIDENT_FLASH_OR_RAM *buffer, uint32_t length) {
    qp_memory_stream_t stream = qp_make_memory_stream((void QP_RESIDENT_FLASH_OR_RAM *)buffer, length);
    return qgf_validate_stream((qp_stream_t *)&stream);
}
