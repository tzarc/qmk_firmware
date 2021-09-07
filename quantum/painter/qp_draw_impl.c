/* Copyright 2020 Paul Cotter (@gr1mr3aver)
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

#include <qp.h>
#include <color.h>

#ifndef QP_PIXDATA_BUFSIZE
#   define QP_PIXDATA_BUFSIZE = 32
#endif

#if QP_PIXDATA_BUFSIZE < 16
#    error Pixel buffer size too small -- QP_PIXDATA_BUFSIZE must be >= 16
#endif

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

// Stream pixel data to the current viewport
bool qp_internal_impl_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    qp_start(lcd);

    // Stream data to the device
    qp_send(lcd, pixel_data, native_pixel_count * sizeof(uint16_t));

    qp_stop(lcd);

    return true;
}

// Manually set a single pixel's color (in rgb565)
bool qp_internal_impl_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    qp_start(lcd);

    // Configure where we're going to be rendering to
    qp_viewport(lcd, x, y, x, y);

    // Convert the color to RGB565 and transmit to the device
    rgb565_t buf = hsv_to_rgb565(hue, sat, val);
    qp_send(lcd, &buf, sizeof(buf));

    qp_stop(lcd);

    return true;
}

// Fallback implementation for drawing lines
bool qp_internal_impl_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    if (x0 == x1) {
        // Vertical line
        uint16_t ys = y0 < y1 ? y0 : y1;
        uint16_t ye = y0 > y1 ? y0 : y1;
        for (uint16_t y = ys; y <= ye; ++y) {
            if (!qp_setpixel(device, x0, y, hue, sat, val)) {
                return false;
            }
        }
    } else if (y0 == y1) {
        // Horizontal line
        uint16_t xs = x0 < x1 ? x0 : x1;
        uint16_t xe = x0 > x1 ? x0 : x1;
        for (uint16_t x = xs; x <= xe; ++x) {
            if (!qp_setpixel(device, x, y0, hue, sat, val)) {
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
            if (!qp_setpixel(device, x, y, hue, sat, val)) {
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
        if (!qp_setpixel(device, x, y, hue, sat, val)) {
            return false;
        }
    }

    return true;
}

// Fallback implementation for drawing rectangles
bool qp_internal_impl_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    painter_driver_t *lcd = (painter_driver_t *)device;

    // Cater for cases where people have submitted the coordinates backwards
    uint16_t l = left < right ? left : right;
    uint16_t r = left > right ? left : right;
    uint16_t t = top < bottom ? top : bottom;
    uint16_t b = top > bottom ? top : bottom;

    if (filled) {
        // Convert the color to RGB565
        rgb565_t clr = hsv_to_rgb565(hue, sat, val);

        // Build a larger buffer so we can stream to the LCD in larger chunks, for speed
        rgb565_t buf[QP_PIXDATA_BUFSIZE];
        for (uint32_t i = 0; i < QP_PIXDATA_BUFSIZE; ++i) buf[i] = clr;

        qp_start(lcd);

        // Configure where we're going to be rendering to
        qp_viewport(lcd, l, t, r, b);

        // Transmit the data to the LCD in chunks
        uint32_t remaining = (r - l + 1) * (b - t + 1);
        while (remaining > 0) {
            uint32_t transmit = (remaining < QP_PIXDATA_BUFSIZE ? remaining : QP_PIXDATA_BUFSIZE);
            uint32_t bytes    = transmit * sizeof(rgb565_t);
            qp_send(lcd, buf, bytes);
            remaining -= transmit;
        }

        qp_stop(lcd);
    } else {
        if (!qp_internal_impl_rect(device, l, t, r, t, hue, sat, val, true)) return false;
        if (!qp_internal_impl_rect(device, l, b, r, b, hue, sat, val, true)) return false;
        if (!qp_internal_impl_rect(device, l, t + 1, l, b - 1, hue, sat, val, true)) return false;
        if (!qp_internal_impl_rect(device, r, t + 1, r, b - 1, hue, sat, val, true)) return false;
    }

    return true;
}

// Fallback implementation for drawing circles
bool qp_internal_impl_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    // plot the initial set of points for x, y and r
    int16_t xcalc = 0;
    int16_t ycalc = (int16_t)radius;
    int16_t err   = ((5 - (radius >> 2)) >> 2);

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
bool qp_internal_impl_filled_rect(painter_device_t *lcd, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, rgb565_t fillbuffer[], uint16_t fillbufferlen) {

        // rectify coordinates
        if (left > right)
        {
            uint16_t tmpleft = left;
            left = right;
            right = tmpleft;
        }
        if (top > bottom)
        {
            uint16_t tmptop = top;
            top = bottom;
            bottom = tmptop;
        }

        // Configure where we're going to be rendering to
        qp_viewport(lcd, left, top, right, bottom);

        // Transmit the data to the LCD in chunks
        uint32_t remaining = (right - left + 1) * (bottom - top + 1);

        while (remaining > 0) {
            uint32_t transmit = (remaining < fillbufferlen ? remaining : fillbufferlen);
            uint32_t bytes    = transmit * sizeof(rgb565_t);
            qp_send(lcd, fillbuffer, bytes);
            remaining -= transmit;
        }
        return true;
}


// Utilize 8-way symmetry to draw circle
bool qp_internal_impl_circle_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
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

    painter_driver_t *lcd = (painter_driver_t *)device;

    // Convert the color to RGB565
    rgb565_t clr = hsv_to_rgb565(hue, sat, val);

    uint16_t bufsize = (offsety << 1) < QP_PIXDATA_BUFSIZE ? (offsety << 1) : QP_PIXDATA_BUFSIZE;
    rgb565_t buf[bufsize];
    if (filled) {
        for (uint32_t i = 0; i < bufsize; ++i) buf[i] = clr;
    }

    uint16_t yplusy = LIMIT(centery + offsety, 0, lcd->qp_driver.screen_height);
    uint16_t yminusy = LIMIT(centery - offsety, 0, lcd->qp_driver.screen_height);
    uint16_t yplusx = LIMIT(centery + offsetx, 0, lcd->qp_driver.screen_height);
    uint16_t yminusx = LIMIT(centery - offsetx, 0, lcd->qp_driver.screen_height);
    uint16_t xplusx = LIMIT(centerx + offsetx, 0, lcd->qp_driver.screen_width);
    uint16_t xminusx = LIMIT(centerx - offsetx, 0, lcd->qp_driver.screen_width);
    uint16_t xplusy = LIMIT(centerx + offsety, 0, lcd->qp_driver.screen_width);
    uint16_t xminusy = LIMIT(centerx - offsety, 0, lcd->qp_driver.screen_width);

    centerx = LIMIT(centerx, 0, lcd->qp_driver.screen_width);
    centery = LIMIT(centery, 0, lcd->qp_driver.screen_height);

    bool retval = true;
    if (offsetx == 0) {
        if (!qp_internal_impl_setpixel(lcd,centerx, yplusy, hue, sat, val)) {
            retval = false;
        }
        if (!qp_internal_impl_setpixel(lcd,centerx, yminusy, hue, sat, val)) {
            retval = false;
        }
        if (filled) {
            if (!qp_internal_impl_filled_rect(lcd, xplusy, centery, xminusy, centery, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
        } else {
            if (!qp_internal_impl_setpixel(lcd,xplusy, centery, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusy, centery, hue, sat, val)) {
                retval = false;
            }
        }
    } else if (offsetx == offsety) {
        if (filled) {
            if (!qp_internal_impl_filled_rect(lcd, xplusy, yplusy, xminusy, yplusy, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
            if (!qp_internal_impl_filled_rect(lcd, xplusy, yminusy, xminusy, yminusy, hue, sat, val, buf, bufsize)) {
                retval = false;
            }

        } else {
            if (!qp_internal_impl_setpixel(lcd,xplusy, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusy, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xplusy, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusy, yminusy, hue, sat, val)) {
                retval = false;
            }
        }
    } else {
        if (filled) {
            // Build a larger buffer so we can stream to the LCD in larger chunks, for speed
            // rgb565_t buf[ST77XX_PIXDATA_BUFSIZE];
            // for (uint32_t i = 0; i < ST77XX_PIXDATA_BUFSIZE; ++i) buf[i] = clr;

            if (!qp_internal_impl_filled_rect(lcd, xplusx, yplusy, xminusx, yplusy, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
            if (!qp_internal_impl_filled_rect(lcd, xplusx, yminusy, xminusx, yminusy, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
            if (!qp_internal_impl_filled_rect(lcd, xplusy, yplusx, xminusy, yplusx, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
            if (!qp_internal_impl_filled_rect(lcd, xplusy, yminusx, xminusy, yminusx, hue, sat, val, buf, bufsize)) {
                retval = false;
            }
        } else {
             if (!qp_internal_impl_setpixel(lcd,xplusx, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusx, yplusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xplusx, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusx, yminusy, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xplusy, yplusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusy, yplusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xplusy, yminusx, hue, sat, val)) {
                retval = false;
            }
            if (!qp_internal_impl_setpixel(lcd,xminusy, yminusx, hue, sat, val)) {
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
        if (!qp_line(device, xx, yy, xl, yy, hue, sat, val)) {
            retval = false;
        }
        if (offsety > 0 && !qp_line(device, xx, yl, xl, yl, hue, sat, val)) {
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

    qp_start(lcd);

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

                    qp_viewport(lcd, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1);
                    qp_internal_impl_drawimage_uncompressed(lcd, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
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
                            qp_viewport(lcd, x, y, x + glyph_desc->width - 1, y + font->glyph_height - 1);
                            qp_internal_drawimage_uncompressed_impl(lcd, font->image_format, font->image_bpp, &fdesc->image_data[glyph_desc->offset], byte_count, glyph_desc->width, font->glyph_height, fdesc->image_palette, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
                            x += glyph_desc->width;
                        }
                    }
                }
            }
#endif  // UNICODE_ENABLE
        }
    }

    qp_stop(lcd);
    return (int16_t)x;
}
