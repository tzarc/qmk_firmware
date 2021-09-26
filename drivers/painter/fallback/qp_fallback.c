/* Copyright 2021 Nick Brassel (@tzarc)
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

#include <stdlib.h>
#include <qp_fallback.h>

/* internal function declarations */
bool qp_fallback_ellipse_drawpixels(painter_device_t device, uint16_t x, uint16_t y, uint16_t dx, uint16_t dy, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

// Fallback implementation for drawing lines
bool qp_fallback_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
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
bool qp_fallback_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    if (filled) {
        // Draw a filled rectangle by drawing horizontal lines for the width/height requested
        for (uint16_t y = top; y <= bottom; ++y) {
            if (!qp_line(device, left, y, right, y, hue, sat, val)) {
                return false;
            }
        }
    } else {
        // Draw a non-filled rectangle by drawing horizontal and vertical lines along each edge
        if (!qp_line(device, left, top, right, top, hue, sat, val)) {
            return false;
        }
        if (!qp_line(device, left, bottom, right, bottom, hue, sat, val)) {
            return false;
        }
        if (!qp_line(device, left, top + 1, left, bottom - 1, hue, sat, val)) {
            return false;
        }
        if (!qp_line(device, right, top + 1, right, bottom - 1, hue, sat, val)) {
            return false;
        }
    }

    return true;
}

// Fallback implementation for drawing ellipses
bool qp_fallback_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    int16_t aa = ((int16_t)sizex) * ((int16_t)sizex);
    int16_t bb = ((int16_t)sizey) * ((int16_t)sizey);
    int16_t fa = 4 * ((int16_t)aa);
    int16_t fb = 4 * ((int16_t)bb);

    int16_t dx = 0;
    int16_t dy = ((int16_t)sizey);

    for (int16_t delta = (2 * bb) + (aa * (1 - (2 * sizey))); bb * dx <= aa * dy; dx++) {
        if (!qp_fallback_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled)) {
            return false;
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
        if (!qp_fallback_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled)) {
            return false;
        }
        if (delta >= 0) {
            delta += fb * (1 - dx);
            dx--;
        }
        delta += aa * (4 * dy + 6);
    }

    return true;
}

// Utilize 4-way symmetry to draw an ellipse
bool qp_fallback_ellipse_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
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

    if (offsetx == 0) {
        if (!qp_setpixel(device, xx, yy, hue, sat, val)) {
            return false;
        }
        if (!qp_setpixel(device, xx, yl, hue, sat, val)) {
            return false;
        }
    } else if (filled) {
        if (!qp_line(device, xx, yy, xl, yy, hue, sat, val)) {
            return false;
        }
        if (offsety > 0 && !qp_line(device, xx, yl, xl, yl, hue, sat, val)) {
            return false;
        }
    } else {
        if (!qp_setpixel(device, xx, yy, hue, sat, val)) {
            return false;
        }
        if (!qp_setpixel(device, xx, yl, hue, sat, val)) {
            return false;
        }
        if (!qp_setpixel(device, xl, yy, hue, sat, val)) {
            return false;
        }
        if (!qp_setpixel(device, xl, yl, hue, sat, val)) {
            return false;
        }
    }

    return true;
}
