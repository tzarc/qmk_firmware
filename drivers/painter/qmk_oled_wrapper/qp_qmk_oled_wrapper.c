/* Copyright 2020 Nick Brassel (@tzarc)
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

#include <string.h>

#include "spi_master.h"

#include "qp.h"
#include "qp_qmk_oled_wrapper.h"
#include "qp_internal.h"
#include "qp_fallback.h"
#include "qp_utils.h"
#include "oled_driver.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct qmk_oled_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    painter_rotation_t      rotation;

    // Manually manage the viewport for streaming pixel data to the display
    uint16_t viewport_l;
    uint16_t viewport_t;
    uint16_t viewport_r;
    uint16_t viewport_b;

    // Current write location to the display when streaming pixel data
    uint16_t pixdata_x;
    uint16_t pixdata_y;
} qmk_oled_painter_device_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//
// NOTE: The variables in this section are intentionally outside a stack frame. They are able to be defined with larger
//       sizes than the normal stack frames would allow, and as such need to be external.
//
//       **** DO NOT refactor this and decide to place the variables inside the function calling them -- you will ****
//       **** very likely get artifacts rendered to the screen as a result.                                       ****
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QUANTUM_PAINTER_COMPRESSION_ENABLE
// Static buffer used for decompression
static uint8_t decompressed_buf[QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE];
#endif  // QUANTUM_PAINTER_COMPRESSION_ENABLE

void translate_pixel_location(qmk_oled_painter_device_t *oled, uint16_t *x, uint16_t *y) {
    switch (oled->rotation) {
        case QP_ROTATION_0:
            // No change
            break;
        case QP_ROTATION_90: {
            int new_x = *y;
            int new_y = *x;
            *x        = new_x;
            *y        = new_y;
        } break;
        case QP_ROTATION_180: {
            int new_x = (OLED_DISPLAY_HEIGHT) - *x - 1;
            int new_y = (OLED_DISPLAY_WIDTH) - *y - 1;
            *x        = new_x;
            *y        = new_y;
        } break;
        case QP_ROTATION_270: {
            int new_x = *y;
            int new_y = *x;
            *x        = new_x;
            *y        = new_y;
        } break;
    }
}

void increment_pixdata_location(qmk_oled_painter_device_t *oled) {
    // Increment the X-position
    oled->pixdata_x++;

    // If the x-coord has gone past the right-side edge, loop it back around and increment the y-coord
    if (oled->pixdata_x > oled->viewport_r) {
        oled->pixdata_x = oled->viewport_l;
        oled->pixdata_y++;
    }

    // If the y-coord has gone past the bottom, loop it back to the top
    if (oled->pixdata_y > oled->viewport_b) {
        oled->pixdata_y = oled->viewport_t;
    }
}

void setpixel(qmk_oled_painter_device_t *oled, uint16_t x, uint16_t y, bool on) {
    translate_pixel_location(oled, &x, &y);
    oled_write_pixel(x, y, on);
}

void stream_pixdata(qmk_oled_painter_device_t *oled, const uint8_t *data, uint32_t native_pixel_count) {
    for (uint32_t pixel_counter = 0; pixel_counter < native_pixel_count; ++pixel_counter) {
        uint16_t byte_offset = (uint16_t)(pixel_counter / 8);
        uint8_t  bit_offset  = (uint8_t)(pixel_counter % 8);
        setpixel(oled, oled->pixdata_x, oled->pixdata_y, (data[byte_offset] & (1 << bit_offset) ? true : false));
        increment_pixdata_location(oled);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_qmk_oled_wrapper_init(painter_device_t device, painter_rotation_t rotation) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;
    oled->rotation                  = rotation;
    oled_clear();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_qmk_oled_wrapper_clear(painter_device_t device) {
    oled_clear();
    return true;
}

bool qp_qmk_oled_wrapper_power(painter_device_t device, bool power_on) {
    if (power_on)
        oled_on();
    else
        oled_off();
    return true;
}

bool qp_qmk_oled_wrapper_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;
    const uint8_t *            data = (const uint8_t *)pixel_data;
    stream_pixdata(oled, data, native_pixel_count);
    return true;
}

bool qp_qmk_oled_wrapper_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;

    // Set the viewport locations
    oled->viewport_l = left;
    oled->viewport_t = top;
    oled->viewport_r = right;
    oled->viewport_b = bottom;

    // Reset the write location to the top left
    oled->pixdata_x = left;
    oled->pixdata_y = top;
    return true;
}

bool qp_qmk_oled_wrapper_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;
    setpixel(oled, x, y, (val >= 128 ? true : false));
    return true;
}

// Draw an image
bool qp_qmk_oled_wrapper_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;

    // Can only render grayscale images if they're 1bpp
    if (image->image_format != IMAGE_FORMAT_GRAYSCALE || image->image_bpp != 1) return false;

    // Configure where we're rendering to
    qp_qmk_oled_wrapper_viewport(device, x, y, x + image->width - 1, y + image->height - 1);
    uint32_t pixel_count = (((uint32_t)image->width) * image->height);

    if (image->compression == IMAGE_COMPRESSED_LZF) {
#ifdef QUANTUM_PAINTER_COMPRESSION_ENABLE
        const painter_compressed_image_descriptor_t *comp_image_desc = (const painter_compressed_image_descriptor_t *)image;
        for (uint16_t i = 0; i < comp_image_desc->chunk_count; ++i) {
            // Check if we're the last chunk
            bool last_chunk = (i == (comp_image_desc->chunk_count - 1));
            // Work out the current chunk size
            uint32_t compressed_size = last_chunk ? (comp_image_desc->compressed_size - comp_image_desc->chunk_offsets[i])        // last chunk
                                                  : (comp_image_desc->chunk_offsets[i + 1] - comp_image_desc->chunk_offsets[i]);  // any other chunk
            // Decode the image data
            uint32_t decompressed_size = qp_decode(&comp_image_desc->compressed_data[comp_image_desc->chunk_offsets[i]], compressed_size, decompressed_buf, sizeof(decompressed_buf));
            uint32_t pixels_this_loop  = last_chunk ? pixel_count : (decompressed_size / 8);
            stream_pixdata(oled, decompressed_buf, pixels_this_loop);
            pixel_count -= pixels_this_loop;
        }
#else
        return false;
#endif  // QUANTUM_PAINTER_COMPRESSION_ENABLE
    } else if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        stream_pixdata(oled, raw_image_desc->image_data, pixel_count);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
static qmk_oled_painter_device_t driver;

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_qmk_oled_wrapper_make_device(void) {
    memset(&driver, 0, sizeof(qmk_oled_painter_device_t));
    driver.qp_driver.init      = qp_qmk_oled_wrapper_init;
    driver.qp_driver.clear     = qp_qmk_oled_wrapper_clear;
    driver.qp_driver.power     = qp_qmk_oled_wrapper_power;
    driver.qp_driver.pixdata   = qp_qmk_oled_wrapper_pixdata;
    driver.qp_driver.viewport  = qp_qmk_oled_wrapper_viewport;
    driver.qp_driver.setpixel  = qp_qmk_oled_wrapper_setpixel;
    driver.qp_driver.line      = qp_fallback_line;
    driver.qp_driver.rect      = qp_fallback_rect;
    driver.qp_driver.circle    = qp_fallback_circle;
    driver.qp_driver.ellipse   = qp_fallback_ellipse;
    driver.qp_driver.drawimage = qp_qmk_oled_wrapper_drawimage;
    return (painter_device_t)&driver;
}
