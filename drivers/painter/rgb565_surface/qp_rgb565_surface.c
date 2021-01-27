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
#include <stdlib.h>
#include <qp.h>
#include <qp_rgb565_surface.h>
#include <qp_internal.h>
#include <qp_fallback.h>
#include <qp_utils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device definition
typedef struct qmk_rgb565_surface_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type

    // Geometry and buffer
    uint16_t  width;
    uint16_t  height;
    uint16_t *buffer;

    // Manually manage the viewport for streaming pixel data to the display
    uint16_t viewport_l;
    uint16_t viewport_t;
    uint16_t viewport_r;
    uint16_t viewport_b;

    // Current write location to the display when streaming pixel data
    uint16_t pixdata_x;
    uint16_t pixdata_y;
} qmk_rgb565_surface_device_t;

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

// Static buffer to contain a generated color palette
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
HSV             surf_hsv_lookup_table[256];
static uint16_t surf_rgb565_palette[256];
#else
HSV             surf_hsv_lookup_table[16];
static uint16_t surf_rgb565_palette[16];
#endif

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))

// Color conversion to RGB565
static inline uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t rgb565 = (((uint16_t)r) >> 3) << 11 | (((uint16_t)g) >> 2) << 5 | (((uint16_t)b) >> 3);
    return BYTE_SWAP(rgb565);
}

// Color conversion to RGB565
static inline uint16_t hsv_to_rgb565(uint8_t hue, uint8_t sat, uint8_t val) {
    RGB rgb = hsv_to_rgb_nocie((HSV){hue, sat, val});
    return rgb_to_rgb565(rgb.r, rgb.g, rgb.b);
}

static inline void increment_pixdata_location(qmk_rgb565_surface_device_t *surf) {
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

static inline void setpixel(qmk_rgb565_surface_device_t *surf, uint16_t x, uint16_t y, uint16_t color) { surf->buffer[y * surf->width + x] = color; }

static inline void append_pixel(qmk_rgb565_surface_device_t *surf, uint16_t pixel) {
    setpixel(surf, surf->pixdata_x, surf->pixdata_y, pixel);
    increment_pixdata_location(surf);
}

static inline void stream_pixdata(qmk_rgb565_surface_device_t *surf, const uint16_t *data, uint32_t native_pixel_count) {
    for (uint32_t pixel_counter = 0; pixel_counter < native_pixel_count; ++pixel_counter) {
        append_pixel(surf, data[pixel_counter]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format image rendering
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Palette renderer
static inline void stream_palette_pixdata_impl(qmk_rgb565_surface_device_t *surf, const uint16_t *const rgb565_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    const uint8_t  pixel_bitmask    = (1 << bits_per_pixel) - 1;
    const uint8_t  pixels_per_byte  = 8 / bits_per_pixel;
    const uint8_t *pixdata          = (const uint8_t *)pixel_data;
    uint32_t       remaining_pixels = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte

    // Transmit each block of pixels
    while (remaining_pixels > 0) {
        for (uint16_t p = 0; p < pixel_count; p += pixels_per_byte) {
            uint8_t pixval      = *pixdata;
            uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
            for (uint8_t q = 0; q < loop_pixels; ++q) {
                append_pixel(surf, rgb565_palette[pixval & pixel_bitmask]);
            }
            ++pixdata;
            remaining_pixels -= loop_pixels;
        }
    }
}

// Recolored renderer
static inline void stream_palette_pixdata(qmk_rgb565_surface_device_t *surf, const uint8_t *const rgb_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    for (uint16_t i = 0; i < items; ++i) {
        surf_rgb565_palette[i] = hsv_to_rgb565(rgb_palette[i * 3 + 0], rgb_palette[i * 3 + 1], rgb_palette[i * 3 + 2]);
    }

    // Transmit each block of pixels
    stream_palette_pixdata_impl(surf, surf_rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

// Recolored renderer
static inline void stream_mono_pixdata_recolor(qmk_rgb565_surface_device_t *surf, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg) {
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    qp_generate_palette(surf_hsv_lookup_table, items, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    for (uint16_t i = 0; i < items; ++i) {
        surf_rgb565_palette[i] = hsv_to_rgb565(surf_hsv_lookup_table[i].h, surf_hsv_lookup_table[i].s, surf_hsv_lookup_table[i].v);
    }

    // Transmit each block of pixels
    stream_palette_pixdata_impl(surf, surf_rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_rgb565_surface_init(painter_device_t device, painter_rotation_t rotation) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    surf->buffer                      = (uint16_t *)malloc(surf->width * surf->height * sizeof(uint16_t));
    memset(surf->buffer, 0, sizeof(surf->width * surf->height * sizeof(uint16_t)));
    (void)rotation;  // no rotation supported.
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_rgb565_surface_clear(painter_device_t device) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    memset(surf->buffer, 0, sizeof(surf->width * surf->height * sizeof(uint16_t)));
    return true;
}

bool qp_rgb565_surface_power(painter_device_t device, bool power_on) { return true; }

bool qp_rgb565_surface_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    const uint16_t *             data = (const uint16_t *)pixel_data;
    stream_pixdata(surf, data, native_pixel_count);
    return true;
}

bool qp_rgb565_surface_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;

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

bool qp_rgb565_surface_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    setpixel(surf, x, y, hsv_to_rgb565(hue, sat, val));
    return true;
}

// Draw an image
bool qp_rgb565_surface_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;

    // Configure where we're rendering to
    qp_rgb565_surface_viewport(device, x, y, x + image->width - 1, y + image->height - 1);
    uint32_t pixel_count = (((uint32_t)image->width) * image->height);

    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        // Stream data to the LCD
        if (image->image_format == IMAGE_FORMAT_RAW || image->image_format == IMAGE_FORMAT_RGB565) {
            // The pixel data is in the correct format already -- send it directly to the device
            stream_pixdata(surf, (const uint16_t *)raw_image_desc->image_data, raw_image_desc->byte_count);
        } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
            // Supplied pixel data is in 4bpp monochrome -- decode it to the equivalent pixel data
            stream_mono_pixdata_recolor(surf, raw_image_desc->base.image_bpp, pixel_count, raw_image_desc->image_data, raw_image_desc->byte_count, hue, sat, val, hue, sat, 0);
        } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
            // Supplied pixel data is in 1bpp monochrome -- decode it to the equivalent pixel data
            stream_palette_pixdata(surf, raw_image_desc->image_palette, raw_image_desc->base.image_bpp, pixel_count, raw_image_desc->image_data, raw_image_desc->byte_count);
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
static qmk_rgb565_surface_device_t driver;

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_rgb565_surface_make_device(uint16_t width, uint16_t height) {
    memset(&driver, 0, sizeof(qmk_rgb565_surface_device_t));
    driver.qp_driver.init      = qp_rgb565_surface_init;
    driver.qp_driver.clear     = qp_rgb565_surface_clear;
    driver.qp_driver.power     = qp_rgb565_surface_power;
    driver.qp_driver.pixdata   = qp_rgb565_surface_pixdata;
    driver.qp_driver.viewport  = qp_rgb565_surface_viewport;
    driver.qp_driver.setpixel  = qp_rgb565_surface_setpixel;
    driver.qp_driver.line      = qp_fallback_line;
    driver.qp_driver.rect      = qp_fallback_rect;
    driver.qp_driver.circle    = qp_fallback_circle;
    driver.qp_driver.ellipse   = qp_fallback_ellipse;
    driver.qp_driver.drawimage = qp_rgb565_surface_drawimage;
    driver.width               = width;
    driver.height              = height;
    return (painter_device_t)&driver;
}

const void *const qp_rgb565_surface_get_buffer_ptr(painter_device_t device) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    return surf->buffer;
}

uint32_t qp_rgb565_surface_get_pixel_count(painter_device_t device) {
    qmk_rgb565_surface_device_t *surf = (qmk_rgb565_surface_device_t *)device;
    return ((uint32_t)surf->width) * surf->height;
}