/* Copyright 2021 Paul Cotter (@gr1mr3aver)
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

#include <color.h>
#include "qp_internal.h"
#include "qp.h"

#ifndef QP_PIXDATA_BUFSIZE
#    define QP_PIXDATA_BUFSIZE 32
#endif

#if QP_PIXDATA_BUFSIZE < 16
#    error Pixel buffer size too small -- QP_PIXDATA_BUFSIZE must be >= 16
#endif

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))
#define LIMIT(x, limit_min, limit_max) (x > limit_max ? limit_max : (x < limit_min ? limit_min : x))

// Static buffer to contain a generated color palette
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
static HSV      hsv_lookup_table[256];
static rgb565_t rgb565_palette[256];
#else
static HSV      hsv_lookup_table[16];
static rgb565_t rgb565_palette[16];
#endif

// Static buffer used for transmitting image data
static rgb565_t pixdata_transmit_buf[QP_PIXDATA_BUFSIZE];

// Color conversion to LCD-native
static inline rgb565_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    rgb565_t rgb565 = (((rgb565_t)r) >> 3) << 11 | (((rgb565_t)g) >> 2) << 5 | (((rgb565_t)b) >> 3);
    return BYTE_SWAP(rgb565);
}

// Color conversion to LCD-native
static inline rgb565_t hsv_to_rgb565(uint8_t hue, uint8_t sat, uint8_t val) {
    RGB rgb = hsv_to_rgb_nocie((HSV){hue, sat, val});
    return rgb_to_rgb565(rgb.r, rgb.g, rgb.b);
}

/* internal function declarations */
bool qp_internal_impl_circle_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_internal_impl_ellipse_drawpixels(painter_device_t device, uint16_t x, uint16_t y, uint16_t dx, uint16_t dy, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format image rendering
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Palette renderer
static inline void qp_internal_impl_send_palette_pixdata(painter_device_t device, const rgb565_t *const rgb565_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    const uint8_t  pixel_bitmask       = (1 << bits_per_pixel) - 1;
    const uint8_t  pixels_per_byte     = 8 / bits_per_pixel;
    const uint16_t max_transmit_pixels = ((ILI9XXX_PIXDATA_BUFSIZE / pixels_per_byte) * pixels_per_byte);  // the number of rgb565 pixels that we can completely fit in the buffer
    const uint8_t *pixdata             = (const uint8_t *)pixel_data;
    uint32_t       remaining_pixels    = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte

    // Transmit each block of pixels
    while (remaining_pixels > 0) {
        uint16_t  transmit_pixels = remaining_pixels < max_transmit_pixels ? remaining_pixels : max_transmit_pixels;
        rgb565_t *target16        = (rgb565_t *)pixdata_transmit_buf;
        for (uint16_t p = 0; p < transmit_pixels; p += pixels_per_byte) {
            uint8_t pixval      = *pixdata;
            uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
            for (uint8_t q = 0; q < loop_pixels; ++q) {
                *target16++ = rgb565_palette[pixval & pixel_bitmask];
                pixval >>= bits_per_pixel;
            }
            ++pixdata;
        }
        qp_send(device, (uint8_t *)pixdata_transmit_buf, transmit_pixels * sizeof(rgb565_t));
        remaining_pixels -= transmit_pixels;
    }
}

// Recolored renderer
static inline void qp_internal_impl_send_palette_pixdata_rgb(painter_device_t device, const uint8_t *const rgb_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    for (uint16_t i = 0; i < items; ++i) {
        rgb565_palette[i] = rgb_to_rgb565(rgb_palette[i * 3 + 0], rgb_palette[i * 3 + 1], rgb_palette[i * 3 + 2]);
    }

    // Transmit each block of pixels
    qp_internal_impl_send_palette_pixdata(device, rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

// Recolored renderer
static inline void qp_internal_impl_send_mono_pixdata_recolor(painter_device_t device, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg) {
    // Memoize the last batch of colors so we're not regenerating the palette if we're not changing anything
    static uint8_t last_bits_per_pixel = UINT8_MAX;
    static int16_t last_hue_fg         = INT16_MIN;
    static int16_t last_sat_fg         = INT16_MIN;
    static int16_t last_val_fg         = INT16_MIN;
    static int16_t last_hue_bg         = INT16_MIN;
    static int16_t last_sat_bg         = INT16_MIN;
    static int16_t last_val_bg         = INT16_MIN;

    // Regenerate the palette only if the inputs have changed
    if (last_bits_per_pixel != bits_per_pixel || last_hue_fg != hue_fg || last_sat_fg != sat_fg || last_val_fg != val_fg || last_hue_bg != hue_bg || last_sat_bg != sat_bg || last_val_bg != val_bg) {
        last_bits_per_pixel = bits_per_pixel;
        last_hue_fg         = hue_fg;
        last_sat_fg         = sat_fg;
        last_val_fg         = val_fg;
        last_hue_bg         = hue_bg;
        last_sat_bg         = sat_bg;
        last_val_bg         = val_bg;

        // Generate the color lookup table
        uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
        qp_generate_palette(hsv_lookup_table, items, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
        for (uint16_t i = 0; i < items; ++i) {
            rgb565_palette[i] = hsv_to_rgb565(hsv_lookup_table[i].h, hsv_lookup_table[i].s, hsv_lookup_table[i].v);
        }
    }

    // Transmit each block of pixels
    qp_internal_impl_send_palette_pixdata(device, rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

// Uncompressed image drawing helper
static bool qp_internal_impl_drawimage_uncompressed(painter_device_t device, painter_image_format_t image_format, uint8_t image_bpp, const uint8_t *pixel_data, uint32_t byte_count, int32_t width, int32_t height, const uint8_t *palette_data, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    // Stream data to the LCD
    if (image_format == IMAGE_FORMAT_RAW || image_format == IMAGE_FORMAT_RGB565) {
        // The pixel data is in the correct format already -- send it directly to the device
        qp_send(device, pixel_data, width * height);
    } else if (image_format == IMAGE_FORMAT_GRAYSCALE) {
        // Supplied pixel data is in 4bpp monochrome -- decode it to the equivalent pixel data
        qp_internal_impl_send_mono_pixdata_recolor(device, image_bpp, width * height, pixel_data, byte_count, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    } else if (image_format == IMAGE_FORMAT_PALETTE) {
        // Supplied pixel data is in 1bpp monochrome -- decode it to the equivalent pixel data
        qp_internal_impl_send_palette_pixdata(device, (rgb565_t *)palette_data, image_bpp, width * height, pixel_data, byte_count);
    }

    return true;
}

// Draw an image
bool qp_internal_impl_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    bool ret = false;
    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        ret                                                  = qp_internal_impl_drawimage_uncompressed(device, image->image_format, image->image_bpp, raw_image_desc->image_data, raw_image_desc->byte_count, image->width, image->height, raw_image_desc->image_palette, hue, sat, val, hue, sat, 0);
    }

    return ret;
}

// Stream pixel data to the current viewport
bool qp_internal_impl_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    // Stream data to the device
    return qp_send(device, pixel_data, native_pixel_count * sizeof(uint16_t));
}

// Manually set a single pixel's color (in rgb565)
bool qp_internal_impl_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    // Configure where we're going to be rendering to
    qp_viewport(device, x, y, x, y, true);

    // Convert the color to RGB565 and transmit to the device
    rgb565_t buf = hsv_to_rgb565(hue, sat, val);
    return qp_send(device, (uint8_t *)&buf, sizeof(buf));
}

// Default implementation for drawing lines
bool qp_internal_impl_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    if (x0 == x1) {
        // Vertical line
        uint16_t ys = y0 < y1 ? y0 : y1;
        uint16_t ye = y0 > y1 ? y0 : y1;

        for (uint16_t y = ys; y <= ye; ++y) {
            if (!qp_internal_impl_setpixel(device, x0, y, hue, sat, val)) {
                return false;
            }
        }
    } else if (y0 == y1) {
        // Horizontal line
        uint16_t xs = x0 < x1 ? x0 : x1;
        uint16_t xe = x0 > x1 ? x0 : x1;
        for (uint16_t x = xs; x <= xe; ++x) {
            if (!qp_internal_impl_setpixel(device, x, y0, hue, sat, val)) {
                return false;
            }
        }
    } else {
        // draw angled line using Bresenham's algo
        int16_t x      = ((int16_t)x0);
        int16_t y      = ((int16_t)y0);
        int16_t slopex = ((int16_t)x0) < ((int16_t)x1) ? 1 : -1;
        int16_t slopey = ((int16_t)y0) < ((int16_t)y1) ? 1 : -1;
        int16_t dx     = abs(((int16_t)x1) - ((int16_t)x0));
        int16_t dy     = -abs(((int16_t)y1) - ((int16_t)y0));

        int16_t e  = dx + dy;
        int16_t e2 = 2 * e;

        while (x != x1 || y != y1) {
            if (!qp_internal_impl_setpixel(device, x, y, hue, sat, val)) {
                return false;
            }
            e2 = 2 * e;
            if (e2 >= dy) {
                e += dy;
                x += slopex;
            }
            if (e2 <= dx) {
                e += dx;
                y += slopey;
            }
        }
        // draw the last pixel
        if (!qp_internal_impl_setpixel(device, x, y, hue, sat, val)) {
            return false;
        }
    }

    return true;
}

// Default implementation for drawing circles
bool qp_internal_impl_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    int16_t xcalc = 0;
    int16_t ycalc = (int16_t)radius;
    int16_t err   = ((5 - (radius >> 2)) >> 2);

    // plot the initial set of points for x, y and r
    qp_internal_impl_circle_drawpixels(device, x, y, xcalc, ycalc, hue, sat, val, filled);
    while (xcalc < ycalc) {
        xcalc++;
        if (err < 0) {
            err += (xcalc << 1) + 1;
        } else {
            ycalc--;
            err += ((xcalc - ycalc) << 1) + 1;
        }
        qp_internal_impl_circle_drawpixels(device, x, y, xcalc, ycalc, hue, sat, val, filled);
    }
    return true;
}

bool qp_internal_impl_filled_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, rgb565_t fillbuffer[], uint16_t fillbufferlen) {
    // Configure where we're going to be rendering to
    qp_viewport(device, left, top, right, bottom, true);

    // Transmit the data to the LCD in chunks
    uint32_t remaining = (right - left + 1) * (bottom - top + 1);
    while (remaining > 0) {
        uint32_t transmit = (remaining < fillbufferlen ? remaining : fillbufferlen);
        uint32_t bytes    = transmit * sizeof(rgb565_t);
        qp_send(device, (uint8_t *)fillbuffer, bytes);
        remaining -= transmit;
    }
    return true;
}

// Default implementation for drawing rectangles
bool qp_internal_impl_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    // Cater for cases where people have submitted the coordinates backwards
    uint16_t l = left < right ? left : right;
    uint16_t r = left > right ? left : right;
    uint16_t t = top < bottom ? top : bottom;
    uint16_t b = top > bottom ? top : bottom;

    // Convert the color to RGB565
    rgb565_t clr = hsv_to_rgb565(hue, sat, val);

    // Build a larger buffer so we can stream to the LCD in larger chunks, for speed
    uint32_t bufsize = (r - l + 1) * (b - t + 1);
    bufsize          = QP_PIXDATA_BUFSIZE < bufsize ? QP_PIXDATA_BUFSIZE : bufsize;
    rgb565_t buf[bufsize];
    for (uint32_t i = 0; i < bufsize; ++i) buf[i] = clr;

    if (filled) {
        qp_internal_impl_filled_rect(device, l, t, r, b, buf, bufsize);

    } else {
        if (!qp_internal_impl_filled_rect(device, l, t, r, t, buf, bufsize)) return false;
        if (t < b) {
            if (!qp_internal_impl_filled_rect(device, l, b, r, b, buf, bufsize)) return false;
        }
        if (t + 1 <= b - 1) {
            if (!qp_internal_impl_filled_rect(device, l, t + 1, l, b - 1, buf, bufsize)) return false;
            if (!qp_internal_impl_filled_rect(device, r, t + 1, r, b - 1, buf, bufsize)) return false;
        }
    }

    return true;
}

// Utilize 8-way symmetry to draw circle
bool qp_internal_impl_circle_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    /*
    Circles have the property of 8-way symmetry, so eight pixels can be drawn
    for each computed [offsetx,offsety] given the center coordinates
    represented by [centerx,centery].

    For filled circles, we can draw horizontal lines between each pair of
    pixels with the same final value of y.

    Two special cases exist and have been optimized:
    1) offsetx == offsety (the final point), makes half the coordinates
    equivalent, so we can omit them (and the corresponding fill lines)
    2) offsetx == 0 (the starting point) means that some horizontal lines
    would be a single pixel in length, so we write individual pixels instead.
    This also makes half the symmetrical points identical to their twins,
    so we only need four points or two points and one line
    */

    uint16_t yplusy  = LIMIT(centery + offsety, 0, driver->screen_height);
    uint16_t yminusy = LIMIT(centery - offsety, 0, driver->screen_height);
    uint16_t yplusx  = LIMIT(centery + offsetx, 0, driver->screen_height);
    uint16_t yminusx = LIMIT(centery - offsetx, 0, driver->screen_height);
    uint16_t xplusx  = LIMIT(centerx + offsetx, 0, driver->screen_width);
    uint16_t xminusx = LIMIT(centerx - offsetx, 0, driver->screen_width);
    uint16_t xplusy  = LIMIT(centerx + offsety, 0, driver->screen_width);
    uint16_t xminusy = LIMIT(centerx - offsety, 0, driver->screen_width);

    centerx = LIMIT(centerx, 0, driver->screen_width);
    centery = LIMIT(centery, 0, driver->screen_height);

    bool retval = true;
    if (offsetx == 0) {
        if (!qp_internal_impl_setpixel(device, centerx, yplusy, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(device, centerx, yminusy, hue, sat, val)) {
            retval = false;
        }
        if (filled) {
            if (!qp_internal_impl_rect(device, xplusy, centery, xminusy, centery, hue, sat, val, true)) {
                retval = false;
            }
        } else {
            if (!qp_internal_impl_setpixel(device, xplusy, centery, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusy, centery, hue, sat, val)) {
                retval = false;
            }
        }
    } else if (offsetx == offsety) {
        if (filled) {
            if (!qp_internal_impl_rect(device, xplusy, yplusy, xminusy, yplusy, hue, sat, val, true)) {
                retval = false;
            }
            if (!qp_internal_impl_rect(device, xplusy, yminusy, xminusy, yminusy, hue, sat, val, true)) {
                retval = false;
            }

        } else {
            if (!qp_internal_impl_setpixel(device, xplusy, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusy, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xplusy, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusy, yminusy, hue, sat, val)) {
                retval = false;
            }
        }
    } else {
        if (filled) {
            // Build a larger buffer so we can stream to the LCD in larger chunks, for speed
            // rgb565_t buf[ST77XX_PIXDATA_BUFSIZE];
            // for (uint32_t i = 0; i < ST77XX_PIXDATA_BUFSIZE; ++i) buf[i] = clr;

            if (!qp_internal_impl_rect(device, xplusx, yplusy, xminusx, yplusy, hue, sat, val, true)) {
                retval = false;
            }
            if (!qp_internal_impl_rect(device, xplusx, yminusy, xminusx, yminusy, hue, sat, val, true)) {
                retval = false;
            }
            if (!qp_internal_impl_rect(device, xplusy, yplusx, xminusy, yplusx, hue, sat, val, true)) {
                retval = false;
            }
            if (!qp_internal_impl_rect(device, xplusy, yminusx, xminusy, yminusx, hue, sat, val, true)) {
                retval = false;
            }
        } else {
            if (!qp_internal_impl_setpixel(device, xplusx, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusx, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xplusx, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusx, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xplusy, yplusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusy, yplusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xplusy, yminusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(device, xminusy, yminusx, hue, sat, val)) {
                retval = false;
            }
        }
    }

    return retval;
}

// Implementation for drawing ellipses
bool qp_internal_impl_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    int16_t aa = ((int16_t)sizex) * ((int16_t)sizex);
    int16_t bb = ((int16_t)sizey) * ((int16_t)sizey);
    int16_t fa = 4 * ((int16_t)aa);
    int16_t fb = 4 * ((int16_t)bb);

    int16_t dx = 0;
    int16_t dy = ((int16_t)sizey);

    bool retval = true;
    for (int16_t delta = (2 * bb) + (aa * (1 - (2 * sizey))); bb * dx <= aa * dy; dx++) {
        if (!qp_internal_impl_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled)) {
            retval = false;
        }
        if (delta >= 0) {
            delta += fa * (1 - dy);
            dy--;
        }
        delta += bb * (4 * dx + 6);
    }

    dx = sizex;
    dy = 0;

    for (int16_t delta = (2 * aa) + (bb * (1 - (2 * sizex))); aa * dy <= bb * dx; dy++) {
        if (!qp_internal_impl_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled)) {
            retval = false;
        }
        if (delta >= 0) {
            delta += fb * (1 - dx);
            dx--;
        }
        delta += aa * (4 * dy + 6);
    }

    return retval;
}

// Utilize 4-way symmetry to draw an ellipse
bool qp_internal_impl_ellipse_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    /*
    Ellipses have the property of 4-way symmetry, so four pixels can be drawn
    for each computed [offsetx,offsety] given the center coordinates
    represented by [centerx,centery].

    For filled ellipses, we can draw horizontal lines between each pair of
    pixels with the same final value of y.

    When offsetx == 0 only two pixels can be drawn for filled or unfilled ellipses
    */

    int16_t xx = ((int16_t)centerx) + ((int16_t)offsetx);
    int16_t xl = ((int16_t)centerx) - ((int16_t)offsetx);
    int16_t yy = ((int16_t)centery) + ((int16_t)offsety);
    int16_t yl = ((int16_t)centery) - ((int16_t)offsety);

    bool retval = true;

    if (offsetx == 0) {
        if (!qp_internal_impl_setpixel(device, xx, yy, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(device, xx, yl, hue, sat, val)) {
            retval = false;
        }
    } else if (filled) {
        if (!qp_internal_impl_rect(device, xx, yy, xl, yy, hue, sat, val, true)) {
            retval = false;
        }
        if (offsety > 0 && !qp_internal_impl_rect(device, xx, yl, xl, yl, hue, sat, val, true)) {
            retval = false;
        }
    } else {
        if (!qp_internal_impl_setpixel(device, xx, yy, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(device, xx, yl, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(device, xl, yy, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(device, xl, yl, hue, sat, val)) {
            retval = false;
        }
    }

    return retval;
}

int16_t qp_internal_impl_drawtext_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    const painter_raw_font_descriptor_t *fdesc = (const painter_raw_font_descriptor_t *)font;

    const char *c = str;
    while (*c) {
        int32_t code_point = 0;
        c                  = decode_utf8(c, &code_point);

        if (code_point >= 0) {
            if (code_point >= 0x20 && code_point < 0x7F) {
                if (fdesc->ascii_glyph_definitions != NULL) {
                    // Search the font's ascii table
                    uint8_t                                  index      = code_point - 0x20;
                    const painter_font_ascii_glyph_offset_t *glyph_desc = &fdesc->ascii_glyph_definitions[index];
                    uint16_t                                 byte_count = 0;
                    if (code_point < 0x7E) {
                        byte_count = (glyph_desc + 1)->offset - glyph_desc->offset;
                    } else if (code_point == 0x7E) {
#ifdef UNICODE_ENABLE
                        // Unicode glyphs directly follow ascii glyphs, so we take the first's offset
                        if (fdesc->unicode_glyph_count > 0) {
                            byte_count = fdesc->unicode_glyph_definitions[0].offset - glyph_desc->offset;
                        } else {
                            byte_count = fdesc->byte_count - glyph_desc->offset;
                        }
#else   // UNICODE_ENABLE
                        byte_count = fdesc->byte_count - glyph_desc->offset;
#endif  // UNICODE_ENABLE
                    }

                    qp_viewport(device, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1, true);
                    qp_internal_impl_drawimage_uncompressed(device, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
                    x += glyph_desc->width;
                }
            }
#ifdef UNICODE_ENABLE
            else {
                // Search the font's unicode table
                if (fdesc->unicode_glyph_definitions != NULL) {
                    for (uint16_t index = 0; index < fdesc->unicode_glyph_count; ++index) {
                        const painter_font_unicode_glyph_offset_t *glyph_desc = &fdesc->unicode_glyph_definitions[index];
                        if (glyph_desc->unicode_glyph == code_point) {
                            uint16_t byte_count = (index == fdesc->unicode_glyph_count - 1) ? (fdesc->byte_count - glyph_desc->offset) : ((glyph_desc + 1)->offset - glyph_desc->offset);
                            qp_viewport(device, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1, true);
                            qp_internal_impl_drawimage_uncompressed(device, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
                            x += glyph_desc->width;
                        }
                    }
                }
            }
#endif  // UNICODE_ENABLE
        }
    }
    return (int16_t)x;
}
