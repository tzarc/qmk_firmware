/* Copyright 2021 Paul Cotter (@gr1mr3aver), Nick Brassel (@tzarc)
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
#include <qp_internal.h>
#include <qp_utils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_circle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Utilize 8-way symmetry to draw circle
bool qp_circle_helper_impl(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, bool filled) {
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

    if (offsetx == 0) {
        if (!qp_setpixel_impl(device, centerx, centery + offsety)) {
            return false;
        }
        if (!qp_setpixel_impl(device, centerx, centery - offsety)) {
            return false;
        }
        if (filled) {
            if (!qp_rect_helper_impl(device, centerx + offsety, centery, centerx - offsety, centery)) {
                return false;
            }
        } else {
            if (!qp_setpixel_impl(device, centerx + offsety, centery)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsety, centery)) {
                return false;
            }
        }
    } else if (offsetx == offsety) {
        if (filled) {
            if (!qp_rect_helper_impl(device, centerx + offsety, centery + offsety, centerx - offsety, centery + offsety)) {
                return false;
            }
            if (!qp_rect_helper_impl(device, centerx + offsety, centery - offsety, centerx - offsety, centery - offsety)) {
                return false;
            }
        } else {
            if (!qp_setpixel_impl(device, centerx + offsety, centery + offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsety, centery + offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx + offsety, centery - offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsety, centery - offsety)) {
                return false;
            }
        }

    } else {
        if (filled) {
            if (!qp_rect_helper_impl(device, centerx + offsetx, centery + offsety, centerx - offsetx, centery + offsety)) {
                return false;
            }
            if (!qp_rect_helper_impl(device, centerx + offsetx, centery - offsety, centerx - offsetx, centery - offsety)) {
                return false;
            }
            if (!qp_rect_helper_impl(device, centerx + offsety, centery + offsetx, centerx - offsety, centery + offsetx)) {
                return false;
            }
            if (!qp_rect_helper_impl(device, centerx + offsety, centery - offsetx, centerx - offsety, centery - offsetx)) {
                return false;
            }
        } else {
            if (!qp_setpixel_impl(device, centerx + offsetx, centery + offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsetx, centery + offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx + offsetx, centery - offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsetx, centery - offsety)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx + offsety, centery + offsetx)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsety, centery + offsetx)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx + offsety, centery - offsetx)) {
                return false;
            }
            if (!qp_setpixel_impl(device, centerx - offsety, centery - offsetx)) {
                return false;
            }
        }
    }

    return true;
}

bool qp_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    // plot the initial set of points for x, y and r
    int16_t xcalc = 0;
    int16_t ycalc = (int16_t)radius;
    int16_t err   = ((5 - (radius >> 2)) >> 2);

    uint32_t required_pixels = QP_MIN((radius * 2) + 1, qp_num_pixels_in_buffer(device));
    qp_fill_pixdata(device, required_pixels, hue, sat, val);

    qp_circle_helper_impl(device, x, y, xcalc, ycalc, filled);
    while (xcalc < ycalc) {
        xcalc++;
        if (err < 0) {
            err += (xcalc << 1) + 1;
        } else {
            ycalc--;
            err += ((xcalc - ycalc) << 1) + 1;
        }
        qp_circle_helper_impl(device, x, y, xcalc, ycalc, filled);
    }

    return true;
}
