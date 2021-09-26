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
// Quantum Painter External API: qp_ellipse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Utilize 4-way symmetry to draw an ellipse
bool qp_ellipse_helper_impl(painter_device_t device, uint16_t centerx, uint16_t centery, uint16_t offsetx, uint16_t offsety, bool filled) {
    /*
    Ellipses have the property of 4-way symmetry, so four pixels can be drawn
    for each computed [offsetx,offsety] given the center coordinates
    represented by [centerx,centery].

    For filled ellipses, we can draw horizontal lines between each pair of
    pixels with the same final value of y.

    When offsetx == 0 only two pixels can be drawn for filled or unfilled ellipses
    */

    int16_t xpx = ((int16_t)centerx) + ((int16_t)offsetx);
    int16_t xmx = ((int16_t)centerx) - ((int16_t)offsetx);
    int16_t ypy = ((int16_t)centery) + ((int16_t)offsety);
    int16_t ymy = ((int16_t)centery) - ((int16_t)offsety);

    if (offsetx == 0) {
        if (!qp_setpixel_impl(device, xpx, ypy)) {
            return false;
        }
        if (!qp_setpixel_impl(device, xpx, ymy)) {
            return false;
        }
    } else if (filled) {
        if (!qp_fillrect_helper_impl(device, xpx, ypy, xmx, ypy)) {
            return false;
        }
        if (offsety > 0 && !qp_fillrect_helper_impl(device, xpx, ymy, xmx, ymy)) {
            return false;
        }
    } else {
        if (!qp_setpixel_impl(device, xpx, ypy)) {
            return false;
        }
        if (!qp_setpixel_impl(device, xpx, ymy)) {
            return false;
        }
        if (!qp_setpixel_impl(device, xmx, ypy)) {
            return false;
        }
        if (!qp_setpixel_impl(device, xmx, ymy)) {
            return false;
        }
    }

    return true;
}

// Fallback implementation for drawing ellipses
bool qp_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    int16_t aa = ((int16_t)sizex) * ((int16_t)sizex);
    int16_t bb = ((int16_t)sizey) * ((int16_t)sizey);
    int16_t fa = 4 * ((int16_t)aa);
    int16_t fb = 4 * ((int16_t)bb);

    int16_t dx = 0;
    int16_t dy = ((int16_t)sizey);

    qp_fill_pixdata(device, QP_MAX(sizex, sizey), hue, sat, val);

    for (int16_t delta = (2 * bb) + (aa * (1 - (2 * sizey))); bb * dx <= aa * dy; dx++) {
        if (!qp_ellipse_helper_impl(device, x, y, dx, dy, filled)) {
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
        if (!qp_ellipse_helper_impl(device, x, y, dx, dy, filled)) {
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
