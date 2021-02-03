/* Copyright 2021 Michael Spradling <mike@mspradling.com>
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
#include <stdlib.h>
#include <qp.h>
#include <qp_fallback.h>
#include <qp_internal.h>
#include <qp_mono2_surface.h>
#include <qp_utils.h>

#ifdef PROTOCOL_CHIBIOS
#    include <ch.h>
#    if !defined(CH_CFG_USE_MEMCORE) || CH_CFG_USE_MEMCORE == FALSE
#        error ChibiOS is configured without a memory allocator. Your keyboard may have set `#define CH_CFG_USE_MEMCORE FALSE`, which is incompatible with this debounce algorithm.
#    endif
#endif

/*
 * Definitions
 */

/* Device definition */
typedef struct qmk_mono2_surface_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    bool allocated;

    // Geometry and buffer
    uint16_t  width;
    uint16_t  height;
    uint8_t *buffer;

    // Manually manage the viewport for streaming pixel data to the display
    uint16_t viewport_l;
    uint16_t viewport_t;
    uint16_t viewport_r;
    uint16_t viewport_b;

    // Current write location to the display when streaming pixel data
    uint16_t pixdata_x;
    uint16_t pixdata_y;
} qmk_mono2_surface_device_t;


/*
 * Forward declaractions
 */
bool qp_mono2_surface_clear(painter_device_t device);

/*
 * Helpers
 */

static inline void increment_pixdata_location(qmk_mono2_surface_device_t *surf) {
    // Increment the X-position
    surf->pixdata_x++;

    // If the x-coord has gone past the right-side edge, loop it back around and increment the y-coord
    if (surf->pixdata_x > surf->viewport_r) {
        surf->pixdata_x = surf->viewport_l;
        surf->pixdata_y++;
    }

    // If the y-coord has gone past the bottom, loop it back to the top
    if (surf->pixdata_y > surf->viewport_b) {
        surf->pixdata_y = surf->viewport_t;
    }
}

static inline void setpixel(qmk_mono2_surface_device_t *surf, uint16_t x, uint16_t y, uint8_t pixel) {
    /* calculate the pixel location and preserve other pixels data */
    uint8_t page = y / 8; // TODO: is this variable or notsurf->page_cnt;
    uint16_t location = x + page * surf->width;

    if (pixel) {
        surf->buffer[location] |= (1 << (y & 7));
    } else {
        surf->buffer[location] &= ~(1 << (y & 7));
    }
}

static inline void append_pixel(qmk_mono2_surface_device_t *surf, uint8_t pixel) {
    setpixel(surf, surf->pixdata_x, surf->pixdata_y, pixel);
    increment_pixdata_location(surf);
}

static inline void stream_pixdata(qmk_mono2_surface_device_t *surf, const uint8_t *data, uint32_t native_pixel_count) {
    for (uint32_t pixel_counter = 0; pixel_counter < native_pixel_count / 8; pixel_counter++) {
        uint8_t pixel = *data;
        for (int i = 0; i < 8; i++) {
            append_pixel(surf, pixel & 0x01);
            pixel >>= 1;
        }
        data++;
    }
}

/*
 * Initialization
 */
bool qp_mono2_surface_init(painter_device_t device, painter_rotation_t rotation) {
    (void)rotation;  // no rotation supported.
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (surf->buffer) {
        return true;
    }

    uint8_t *buffer = (uint8_t *)malloc(surf->width * surf->height / sizeof(uint8_t));
    if (!buffer) {
        return false;
    }
    surf->buffer = buffer;
    qp_mono2_surface_clear(device);
    return true;
}


/*
 * Operations
 */

bool qp_mono2_surface_clear(painter_device_t device) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (!surf->buffer) {
        return false;
    }

    memset(surf->buffer, 0, surf->width * surf->height / sizeof(uint8_t));
    return true;
}

bool qp_mono2_surface_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (!surf->buffer) {
        return false;
    }

    const uint8_t *data = (const uint8_t *)pixel_data;
    stream_pixdata(surf, data, native_pixel_count);
    return true;
}

bool qp_mono2_surface_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (!surf->buffer) {
        return false;
    }

    // Set the viewport locations
    surf->viewport_l = left;
    surf->viewport_t = top;
    surf->viewport_r = right;
    surf->viewport_b = bottom;

    // Reset the write location to the top left
    surf->pixdata_x = left;
    surf->pixdata_y = top;
    return true;
}

bool qp_mono2_surface_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (!surf->buffer) {
        return false;
    }

    setpixel(surf, x, y, (bool)val);
    return true;
}

/* Draw an image */
bool qp_mono2_surface_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    if (!surf->buffer) {
        return false;
    }

    // Configure where we're rendering to
    qp_mono2_surface_viewport(device, x, y, x + image->width - 1, y + image->height - 1);
    uint32_t pixel_count = (((uint32_t)image->width) * image->height);

    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        // Stream data to the LCD
        if (image->image_format == IMAGE_FORMAT_RAW || image->image_format == IMAGE_FORMAT_GRAYSCALE) {
            // The pixel data is in the correct format already -- send it directly to the device
            stream_pixdata(surf, (const uint8_t *)raw_image_desc->image_data, pixel_count);
        } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
            // Supplied pixel data is in 4bpp monochrome -- decode it to the equivalent pixel data
            stream_pixdata(surf, (const uint8_t *)raw_image_desc->image_data, pixel_count);
	}
    // TODO How should I handle images.  Should I take the weight of the pixel and convert that to black or white?
        /*} else if (image->image_format == IMAGE_FORMAT_PALETTE) {
            // Supplied pixel data is in 1bpp monochrome -- decode it to the equivalent pixel data
            stream_palette_pixdata(surf, raw_image_desc->image_palette, raw_image_desc->base.image_bpp, pixel_count, raw_image_desc->image_data, raw_image_desc->byte_count);
        }
	*/
    }

    return true;
}

/*
 * Device Creation
 */
// Driver storage
static qmk_mono2_surface_device_t drivers[MONO2_SURFACE_NUM] = {0};

    // Factory function for creating a handle to the mono2 surface
    painter_device_t qp_mono2_surface_make_device(uint16_t width, uint16_t height) {
    for (int i = 0; i < MONO2_SURFACE_NUM; i++) {
	qmk_mono2_surface_device_t *driver = &drivers[i];
	if (!driver->allocated) {
	    driver->allocated           = true;
	    driver->qp_driver.init      = qp_mono2_surface_init;
	    driver->qp_driver.clear     = qp_mono2_surface_clear;
	    driver->qp_driver.pixdata   = qp_mono2_surface_pixdata;
	    driver->qp_driver.viewport  = qp_mono2_surface_viewport;
	    driver->qp_driver.setpixel  = qp_mono2_surface_setpixel;
	    driver->qp_driver.line      = qp_fallback_line;
	    driver->qp_driver.rect      = qp_fallback_rect;
	    driver->qp_driver.circle    = qp_fallback_circle;
	    driver->qp_driver.ellipse   = qp_fallback_ellipse;
	    driver->qp_driver.drawimage = qp_mono2_surface_drawimage;
	    driver->width               = width;
	    driver->height              = height;
	    return (painter_device_t)driver;
	}
    }
    return NULL;
}

const void *const qp_mono2_surface_get_buffer_ptr(painter_device_t device) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    return surf->buffer;
}

uint32_t qp_mono2_surface_get_pixel_count(painter_device_t device) {
    qmk_mono2_surface_device_t *surf = (qmk_mono2_surface_device_t *)device;
    return ((uint32_t)surf->width) * surf->height;
}
