// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

// See https://docs.qmk.fm/#/quantum_painter_qgf for more information.

#include <qgf.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QGF API

// Validate the specifics of each block header
static bool qgf_validate_block_header(qgf_block_header_v1_t *desc, uint8_t expected_typeid, int32_t expected_length) {
    if (desc->type_id != expected_typeid || desc->neg_type_id != ((~expected_typeid) & 0xFF)) {
        qp_dprintf("Failed to validate header, expected typeid 0x%02X, was 0x%02X, expected negated typeid 0x%02X, was 0x%02X\n", (int)expected_typeid, (int)desc->type_id, (int)((~desc->type_id) & 0xFF), (int)desc->neg_type_id);
        return false;
    }

    if (expected_length >= 0 && desc->length != expected_length) {
        qp_dprintf("Failed to validate header (typeid 0x%02X), expected length %d, was %d\n", (int)desc->type_id, (int)expected_length, (int)desc->length);
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

static bool qgf_parse_frame_descriptor(qgf_frame_v1_t *frame_descriptor, uint8_t *bpp, bool *has_palette, bool *is_delta) {
    // Work out if this is a delta frame
    *is_delta = (frame_descriptor->flags & QGF_FRAME_FLAG_DELTA) == QGF_FRAME_FLAG_DELTA;

    // Work out the BPP and if we have a palette
    switch (frame_descriptor->format) {
        case GRAYSCALE_1BPP:
            *bpp         = 1;
            *has_palette = false;
            break;
        case GRAYSCALE_2BPP:
            *bpp         = 2;
            *has_palette = false;
            break;
        case GRAYSCALE_4BPP:
            *bpp         = 4;
            *has_palette = false;
            break;
        case GRAYSCALE_8BPP:
            *bpp         = 8;
            *has_palette = false;
            break;
        case PALETTE_1BPP:
            *bpp         = 1;
            *has_palette = true;
            break;
        case PALETTE_2BPP:
            *bpp         = 2;
            *has_palette = true;
            break;
        case PALETTE_4BPP:
            *bpp         = 4;
            *has_palette = true;
            break;
        case PALETTE_8BPP:
            *bpp         = 8;
            *has_palette = true;
            break;
        default:
            qp_dprintf("Failed to parse frame_descriptor, invalid format 0x%02X\n", (int)frame_descriptor->format);
            return false;
    }

    return true;
}

static bool qgf_validate_stream(qp_stream_t *stream) {
    // Validate the graphics descriptor
    qgf_graphics_descriptor_v1_t graphics_descriptor;
    if (qp_stream_read(&graphics_descriptor, sizeof(qgf_graphics_descriptor_v1_t), 1, stream) != sizeof(qgf_graphics_descriptor_v1_t)) {
        qp_dprintf("Failed to read graphics_descriptor, expected length was not %d\n", (int)sizeof(qgf_graphics_descriptor_v1_t));
        return false;
    }

    if (!qgf_validate_block_header(&graphics_descriptor.header, QGF_GRAPHICS_DESCRIPTOR_TYPEID, (sizeof(qgf_graphics_descriptor_v1_t) - sizeof(qgf_block_header_v1_t)))) {
        return false;
    }

    if (graphics_descriptor.magic != QGF_MAGIC || graphics_descriptor.qgf_version != 0x01) {
        qp_dprintf("Failed to validate graphics_descriptor, expected magic 0x%06X was 0x%06X, expected version = 0x%02X was 0x%02X\n", (int)QGF_MAGIC, (int)graphics_descriptor.magic, (int)0x01, (int)graphics_descriptor.qgf_version);
        return false;
    }

    if (graphics_descriptor.neg_total_file_size != ~graphics_descriptor.total_file_size) {
        qp_dprintf("Failed to validate graphics_descriptor, expected negated length 0x%08X was 0x%08X\n", (int)(~graphics_descriptor.total_file_size), (int)graphics_descriptor.neg_total_file_size);
        return false;
    }

    // Read and validate the frame offset block
    {
        qgf_frame_offsets_v1_t frame_offsets;
        if (qp_stream_read(&frame_offsets, sizeof(qgf_frame_offsets_v1_t), 1, stream) != sizeof(qgf_frame_offsets_v1_t)) {
            qp_dprintf("Failed to read frame_offsets, expected length was not %d\n", (int)sizeof(qgf_frame_offsets_v1_t));
            return false;
        }

        if (!qgf_validate_block_header(&frame_offsets.header, QGF_FRAME_OFFSET_DESCRIPTOR_TYPEID, (graphics_descriptor.frame_count * sizeof(uint32_t)))) {
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
            qp_dprintf("Failed to read frame_descriptor, expected length was not %d\n", (int)sizeof(qgf_frame_v1_t));
            return false;
        }

        if (!qgf_validate_block_header(&frame_descriptor.header, QGF_FRAME_DESCRIPTOR_TYPEID, (sizeof(qgf_frame_v1_t) - sizeof(qgf_block_header_v1_t)))) {
            return false;
        }

        uint8_t bpp;
        bool    has_palette_block;
        bool    has_delta_block;
        if (!qgf_parse_frame_descriptor(&frame_descriptor, &bpp, &has_palette_block, &has_delta_block)) {
            return false;
        } else {
            qp_dprintf("Frame info: format 0x%02X, flags 0x%02X, compression 0x%02X, %d bpp, %s, %s\n", (int)frame_descriptor.format, (int)frame_descriptor.flags, (int)frame_descriptor.compression_scheme, (int)bpp, has_palette_block ? "has palette" : "no palette", has_delta_block ? "uses deltas" : "no deltas");
        }

        // Read and validate the delta block if required
        if (has_delta_block) {
            qgf_delta_v1_t delta_descriptor;
            if (qp_stream_read(&delta_descriptor, sizeof(qgf_delta_v1_t), 1, stream) != sizeof(qgf_delta_v1_t)) {
                qp_dprintf("Failed to read delta_descriptor, expected length was not %d\n", (int)sizeof(qgf_delta_v1_t));
                return false;
            }

            if (!qgf_validate_block_header(&delta_descriptor.header, QGF_FRAME_DELTA_DESCRIPTOR_TYPEID, (sizeof(qgf_delta_v1_t) - sizeof(qgf_block_header_v1_t)))) {
                return false;
            }
        }

        // Read and validate the palette block if required
        if (has_palette_block) {
            qgf_palette_v1_t palette_descriptor;
            if (qp_stream_read(&palette_descriptor, sizeof(qgf_palette_v1_t), 1, stream) != sizeof(qgf_palette_v1_t)) {
                qp_dprintf("Failed to read palette_descriptor, expected length was not %d\n", (int)sizeof(qgf_palette_v1_t));
                return false;
            }

            // BPP determines the number of palette entries, each entry is a HSV888 triplet.
            uint32_t expected_length = (1 << bpp) * (sizeof(uint8_t) * 3);

            if (!qgf_validate_block_header(&palette_descriptor.header, QGF_FRAME_PALETTE_DESCRIPTOR_TYPEID, expected_length)) {
                return false;
            }
        }

        // Read and validate the data block
        qgf_data_v1_t data_descriptor;
        if (qp_stream_read(&data_descriptor, sizeof(qgf_data_v1_t), 1, stream) != sizeof(qgf_data_v1_t)) {
            qp_dprintf("Failed to read data_descriptor, expected length was not %d\n", (int)sizeof(qgf_data_v1_t));
            return false;
        }

        if (!qgf_validate_block_header(&data_descriptor.header, QGF_FRAME_DATA_DESCRIPTOR_TYPEID, -1)) {
            return false;
        }
    }

    return true;
}

// Work out the total size of an image definition, assuming we can read far enough into the file
uint32_t qgf_get_total_size(qp_stream_t *stream) {
    // Get the original location
    uint32_t oldpos = qp_stream_tell(stream);

    qgf_graphics_descriptor_v1_t gd;
    qp_stream_read(&gd, sizeof(qgf_graphics_descriptor_v1_t), 1, stream);

    // Ensure the lengths match
    if (gd.neg_total_file_size != (~gd.total_file_size)) {
        qp_dprintf("Failed to validate graphics descriptor, expected negated length 0x%08X was 0x%08X\n", (int)(~gd.total_file_size), (int)gd.neg_total_file_size);
        return 0;
    }

    qp_stream_setpos(stream, oldpos);
    return gd.total_file_size;
}

// Validate the image data
bool qgf_validate_mem(const void QP_RESIDENT_FLASH_OR_RAM *buffer) {
    // Assume we can read the graphics descriptor
    qp_memory_stream_t stream = qp_make_memory_stream((void QP_RESIDENT_FLASH_OR_RAM *)buffer, sizeof(qgf_graphics_descriptor_v1_t));

    // Update the length of the stream to match, and rewind to the start
    stream.length   = qgf_get_total_size((qp_stream_t *)&stream);
    stream.position = 0;

    // Now that we know the length, validate the input data
    return qgf_validate_stream((qp_stream_t *)&stream);
}
