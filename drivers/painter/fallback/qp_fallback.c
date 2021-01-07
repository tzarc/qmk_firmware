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

#include <stdlib.h>
#include "qp_fallback.h"

/* internal function declarations */
bool qp_fallback_circle_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
bool qp_fallback_ellipse_drawpixels(painter_device_t device, uint16_t x, uint16_t y, uint16_t dx, uint16_t dy, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

// Fallback implementation for drawing lines
bool qp_fallback_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    if (x0 == x1) {
        // Vertical line
        for (uint16_t y = y0; y <= y1; ++y) {
            if (!qp_setpixel(device, x0, y, hue, sat, val)) {
                return false;
            }
        }
    } else if (y0 == y1) {
        // Horizontal line
        for (uint16_t x = x0; x <= x1; ++x) {
            if (!qp_setpixel(device, x, y0, hue, sat, val)) {
                return false;
            }
        }
    } else {
        // draw angled line using Bresenham's algo
        uint16_t x = x0;
        uint16_t y = y0;
        uint16_t slopex = x0 < x1 ? 1 : -1;
        uint16_t slopey = y0 < y1 ? 1 : -1;
        uint16_t dx = abs(x1 - x0);
        uint16_t dy = -abs(y1 - y0);

        uint16_t e = dx + dy;
        uint16_t e2 = 2 * e;

        while (x != x1 && y != y1) {
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

// Fallback implementation for drawing circles
bool qp_fallback_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {

// plot the initial set of points for x, y and r
    uint16_t xcalc, ycalc, err;
    xcalc = 0;
    ycalc = radius;
    err = ((5 - (radius >> 2)) >> 2);

    qp_fallback_circle_drawpixels(device, x, y, xcalc, ycalc, hue, sat, val, filled);
    while (xcalc < ycalc) {
        xcalc++;
        if (err < 0) {
            err += (xcalc << 1) + 1;
        } else {
            ycalc--;
            err += ((xcalc - ycalc) << 1) + 1;
        }
        qp_fallback_circle_drawpixels(device, x, y, xcalc, ycalc, hue, sat, val, filled);
    }

    return true;
}

// Utilize 8-way symmetry to draw circle
bool qp_fallback_circle_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {

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

    if(offsetx == 0) {
        if (!qp_setpixel(device, centerx, centery + offsety, hue, sat, val)) {
            return false;
        }
        if (!qp_setpixel(device, centerx, centery - offsety, hue, sat, val)) {
            return false;
        }
        if (filled) {
            if (!qp_line(device, centerx + offsety, centery, centerx - offsety, centery, hue, sat, val)) {
                return false;
            }
        } else {
            if (!qp_setpixel(device, centerx + offsety, centery, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsety, centery, hue, sat, val)) {
                return false;
            }
        }
    }
    else if(offsetx == offsety)
    {
        if (filled) {
            if (!qp_line(device, centerx + offsety, centery + offsety, centerx - offsety, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_line(device, centerx + offsety, centery - offsety, centerx - offsety, centery - offsety, hue, sat, val)) {
                return false;
            }
        }
        else
        {
            if (!qp_setpixel(device, centerx + offsety, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsety, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx + offsety, centery - offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsety, centery - offsety, hue, sat, val)) {
                return false;
            }
        }

    }else{
        if (filled) {
            if (!qp_line(device, centerx + offsetx, centery + offsety, centerx - offsetx, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_line(device, centerx + offsetx, centery - offsety, centerx - offsetx, centery - offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_line(device, centerx + offsety, centery + offsetx, centerx - offsety, centery + offsetx, hue, sat, val)) {
            return false;
            }
            if (!qp_line(device, centerx + offsety, centery - offsetx, centerx - offsety, centery - offsetx, hue, sat, val)) {
                return false;
            }
        }
        else
        {
            if (!qp_setpixel(device, centerx + offsetx, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsetx, centery + offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx + offsetx, centery - offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsetx, centery - offsety, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx + offsety, centery + offsetx, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsety, centery + offsetx, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx + offsety, centery - offsetx, hue, sat, val)) {
                return false;
            }
            if (!qp_setpixel(device, centerx - offsety, centery - offsetx, hue, sat, val)) {
                return false;
            }
        }
    }

    return true;
}

// Fallback implementation for drawing ellipses
bool qp_fallback_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled)
{
    uint16_t aa = sizex * sizex;
    uint16_t bb = sizey * sizey;
    uint16_t fa = 4 * aa;
    uint16_t fb = 4 * bb;

    uint16_t dx = 0;
    uint16_t dy = sizey;

    for (uint16_t delta = (2 * bb) + (aa * (1 - (2 * sizey))); bb * dx <= aa * dy; dx++)
    {
        if (!qp_fallback_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled))
        {
            return false;
        }
        if(delta >= 0 )
        {
            delta += fa * (1 - dy) ;
            dy--;
        }
        delta += bb * (4 * dx + 6);
    }

    dx = sizex;
    dy = 0;

    for (uint16_t delta = (2 * aa) + (bb * (1 - (2 * sizex))); aa * dy <= bb * dx; dy++)
    {
        if (!qp_fallback_ellipse_drawpixels(device, x, y, dx, dy, hue, sat, val, filled))
        {
            return false;
        }
        if (delta >= 0)
        {
            delta += fb * (1 - dx);
            dx--;
        }
        delta += aa * (4 * dy + 6);
    }

    return true;
}

// Utilize 4-way symmetry to draw an ellipse
bool qp_fallback_ellipse_drawpixels(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offestx, uint16_t offsety, uint8_t hue, uint8_t sat, uint8_t val, bool filled)
{
    /*
    Ellipses have the property of 4-way symmetry, so four pixels can be drawn
    for each computed [offsetx,offsety] given the center coordinates
    represented by [centerx,centery].

    For filled ellipses, we can draw horizontal lines between each pair of
    pixels with the same final value of y.

    When offsetx == 0 only two pixels can be drawn for filled or unfilled ellipses
    */

    uint16_t xx = centerx + offestx;
    uint16_t xl = centerx - offestx;
    uint16_t yy = centery + offsety;
    uint16_t yl = centery - offsety;

    if (offestx == 0)
    {
        if (!qp_setpixel(device, xx, yy, hue, sat, val))
        {
            return false;
        }
        if (!qp_setpixel(device, xx, yl, hue, sat, val))
        {
            return false;
        }
    }
    else if (filled)
    {

        if (!qp_line(device, xx, yy, xl, yy, hue, sat, val))
        {
            return false;
        }
        if (offsety > 0 && !qp_line(device, xx, yl, xl, yl, hue, sat, val))
        {
            return false;
        }
    }
    else
    {
        if (!qp_setpixel(device, xx, yy, hue, sat, val))
        {
            return false;
        }
        if (!qp_setpixel(device, xx, yl, hue, sat, val))
        {
            return false;
        }
        if (!qp_setpixel(device, xl, yy, hue, sat, val))
        {
            return false;
        }
        if (!qp_setpixel(device, xl, yl, hue, sat, val))
        {
            return false;
        }
    }

    return true;
}
