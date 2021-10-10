// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <qp_internal.h>
#include <qp_draw.h>
#include <qp_comms.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawimage

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image) { return qp_drawimage_recolor(device, x, y, image, 0, 0, 255, 0, 0, 0); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_drawimage_recolor

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    qp_dprintf("qp_drawimage_recolor: entry\n");
    struct painter_driver_t* driver = (struct painter_driver_t*)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_drawimage_recolor: fail (validation_ok == false)\n");
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

    const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM* raw_image_desc = (const painter_raw_image_descriptor_t QP_RESIDENT_FLASH_OR_RAM*)image;

    // Set up the input/output states
    struct qp_internal_byte_input_state input_state    = {.device = device, .src_data = raw_image_desc->image_data};
    qp_internal_byte_input_callback     input_callback = qp_internal_prepare_input_state(&input_state, image->compression);
    if (input_callback == NULL) {
        qp_dprintf("qp_drawimage_recolor: fail (invalid image compression scheme)\n");
        qp_comms_stop(device);
        return false;
    }

    struct qp_internal_pixel_output_state output_state = {.device = device, .pixel_write_pos = 0, .max_pixels = qp_internal_num_pixels_in_buffer(device)};

    // Stream data to the LCD
    bool ret = false;
    if (image->image_format == IMAGE_FORMAT_RAW) {
        // Transmit the raw data.
        ret = driver->driver_vtable->pixdata(device, raw_image_desc->image_data, ((uint32_t)image->width) * image->height);
    } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
        // Specify the fg/bg
        qp_pixel_t fg_hsv888 = {.hsv888 = {.h = hue_fg, .s = sat_fg, .v = val_fg}};
        qp_pixel_t bg_hsv888 = {.hsv888 = {.h = hue_bg, .s = sat_bg, .v = val_bg}};

        // Decode the pixel data
        ret = qp_internal_decode_recolor(device, ((uint32_t)image->width) * image->height, image->image_bpp, input_callback, &input_state, fg_hsv888, bg_hsv888, qp_internal_pixel_appender, &output_state);

        // Any leftovers need transmission as well.
        if (ret && output_state.pixel_write_pos > 0) {
            ret &= driver->driver_vtable->pixdata(device, qp_internal_global_pixdata_buffer, output_state.pixel_write_pos);
        }
    } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
        // Read the palette entries
        const uint8_t QP_RESIDENT_FLASH_OR_RAM* hsv_palette     = raw_image_desc->image_palette;
        uint16_t                                palette_entries = 1 << image->image_bpp;
        qp_internal_invalidate_palette();
        for (uint16_t i = 0; i < palette_entries; ++i) {
            qp_internal_global_pixel_lookup_table[i] = (qp_pixel_t){.hsv888 = {.h = hsv_palette[i * 3 + 0], .s = hsv_palette[i * 3 + 1], .v = hsv_palette[i * 3 + 2]}};
        }

        // Convert the palette to native format
        if (!driver->driver_vtable->palette_convert(device, palette_entries, qp_internal_global_pixel_lookup_table)) {
            qp_dprintf("qp_drawimage_recolor: fail (could not convert pixels to native)\n");
            qp_comms_stop(device);
            return false;
        }

        // Decode the pixel data
        ret = qp_internal_decode_palette(device, ((uint32_t)image->width) * image->height, image->image_bpp, input_callback, &input_state, qp_internal_global_pixel_lookup_table, qp_internal_pixel_appender, &output_state);

        // Any leftovers need transmission as well.
        if (ret && output_state.pixel_write_pos > 0) {
            ret &= driver->driver_vtable->pixdata(device, qp_internal_global_pixdata_buffer, output_state.pixel_write_pos);
        }
    }

    qp_dprintf("qp_drawimage_recolor: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}
