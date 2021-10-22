// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <qp_internal.h>
#include <qp_draw.h>
#include <qp_comms.h>
#include <qgf.h>

#ifndef NUM_QUANTUM_PAINTER_IMAGES
#    define NUM_QUANTUM_PAINTER_IMAGES 8
#endif  // NUM_QUANTUM_PAINTER_IMAGES

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QGF image handles

typedef struct QP_PACKED qgf_image_handle_t {
    painter_image_desc_t base;
    bool                 validate_okay;
    union {
        qp_stream_t        stream;
        qp_memory_stream_t mem_stream;
#ifdef QP_STREAM_HAS_FILE_IO
        qp_file_stream_t file_stream;
#endif  // QP_STREAM_HAS_FILE_IO
    };
} qgf_image_handle_t;

static qgf_image_handle_t image_descriptors[NUM_QUANTUM_PAINTER_IMAGES] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_load_image_mem

painter_image_handle_t qp_load_image_mem(const void QP_RESIDENT_FLASH_OR_RAM* buffer) {
    qp_dprintf("qp_load_image_mem: entry\n");
    qgf_image_handle_t* image = NULL;

    // Find a free slot
    for (int i = 0; i < NUM_QUANTUM_PAINTER_IMAGES; ++i) {
        if (!image_descriptors[i].validate_okay) {
            image = &image_descriptors[i];
            break;
        }
    }

    // Drop out if not found
    if (!image) {
        qp_dprintf("qp_load_image_mem: fail (no free slot)\n");
        return NULL;
    }

    // Assume we can read the graphics descriptor
    image->mem_stream = qp_make_memory_stream((void QP_RESIDENT_FLASH_OR_RAM*)buffer, sizeof(qgf_graphics_descriptor_v1_t));

    // Update the length of the stream to match, and rewind to the start
    image->mem_stream.length   = qgf_get_total_size(&image->stream);
    image->mem_stream.position = 0;

    // Now that we know the length, validate the input data
    if (!qgf_validate_stream(&image->stream)) {
        qp_dprintf("qp_load_image_mem: fail (failed validation)\n");
        return NULL;
    }

    // Fill out the QP image descriptor
    qgf_read_graphics_descriptor(&image->stream, &image->base.width, &image->base.height, &image->base.frame_count);

    // Validation success, we can return the handle
    image->validate_okay = true;
    qp_dprintf("qp_load_image_mem: ok\n");
    return (painter_image_handle_t)image;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawimage

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_handle_t image) { return qp_drawimage_recolor(device, x, y, image, 0, 0, 255, 0, 0, 0); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawimage_recolor

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_handle_t image, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    qp_dprintf("qp_drawimage_recolor: entry\n");
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_drawimage_recolor: fail (validation_ok == false)\n");
        return false;
    }

    qgf_image_handle_t* qgf_image = (qgf_image_handle_t*)image;
    if (!qgf_image->validate_okay) {
        qp_dprintf("qp_drawimage_recolor: fail (invalid image)\n");
        qp_comms_stop(device);
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

    // Ensure we aren't reusing any palette
    qp_internal_invalidate_palette();

    // Read the frame info // TODO: animations and deferred exec
    qgf_frame_info_t frame_info = {0};
    if (!qgf_prepare_frame_for_stream_read(&qgf_image->stream, 0, &frame_info, qp_internal_global_pixel_lookup_table)) {
        qp_dprintf("qp_drawimage_recolor: fail (could not read frame)\n");
        qp_comms_stop(device);
        return false;
    }

    // Set up the input/output states
    struct qp_internal_byte_input_state input_state    = {.device = device, .src_stream = &qgf_image->stream};
    qp_internal_byte_input_callback     input_callback = qp_internal_prepare_input_state(&input_state, frame_info.compression_scheme);
    if (input_callback == NULL) {
        qp_dprintf("qp_drawimage_recolor: fail (invalid image compression scheme)\n");
        qp_comms_stop(device);
        return false;
    }

    struct qp_internal_pixel_output_state output_state = {.device = device, .pixel_write_pos = 0, .max_pixels = qp_internal_num_pixels_in_buffer(device)};

    // Stream data to the LCD
    bool ret = false;

    // If we have no palette, then it's a greyscale image -- interpolate the palette given the supplied arguments
    int16_t palette_entries = 1 << frame_info.bpp;
    if (!frame_info.has_palette) {
        // Specify the fg/bg
        qp_pixel_t fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
        qp_pixel_t bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};
        qp_internal_interpolate_palette(fg_hsv888, bg_hsv888, palette_entries);
    }

    // Convert the palette to native format
    if (!driver->driver_vtable->palette_convert(device, palette_entries, qp_internal_global_pixel_lookup_table)) {
        qp_dprintf("qp_drawimage_recolor: fail (could not convert pixels to native)\n");
        qp_comms_stop(device);
        return false;
    }

    // Decode the pixel data
    ret = qp_internal_decode_palette(device, ((uint32_t)image->width) * image->height, frame_info.bpp, input_callback, &input_state, qp_internal_global_pixel_lookup_table, qp_internal_pixel_appender, &output_state);

    // Any leftovers need transmission as well.
    if (ret && output_state.pixel_write_pos > 0) {
        ret &= driver->driver_vtable->pixdata(device, qp_internal_global_pixdata_buffer, output_state.pixel_write_pos);
    }

    qp_dprintf("qp_drawimage_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}
