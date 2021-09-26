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

#include <quantum.h>
#include <utf8.h>
#include <qp_internal.h>
#include <qp_utils.h>

_Static_assert((QP_PIXDATA_BUFFER_SIZE > 0) && (QP_PIXDATA_BUFFER_SIZE % 16) == 0, "QP_PIXDATA_BUFFER_SIZE needs to be a non-zero multiple of 16");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_setpixel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    qp_fill_pixdata(device, 1, hue, sat, val);
    return qp_setpixel_impl(device, x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_line
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    if (x0 == x1) {
        return qp_rect(device, x0, y0, x0, y1, hue, sat, val, true);
    } else if (y0 == y1) {
        return qp_rect(device, x0, y0, x1, y0, hue, sat, val, true);
    } else {
        qp_fill_pixdata(device, 1, hue, sat, val);

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
            if (!qp_setpixel_impl(device, x, y)) {
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
        if (!qp_setpixel_impl(device, x, y)) {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_rect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_fillrect_helper_impl(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    uint32_t pixels_in_pixdata = qp_num_pixels_in_buffer(device);

    uint16_t l = QP_MIN(left, right);
    uint16_t r = QP_MAX(left, right);
    uint16_t t = QP_MIN(top, bottom);
    uint16_t b = QP_MAX(top, bottom);
    uint16_t w = r - l + 1;
    uint16_t h = b - t + 1;

    uint32_t remaining = w * h;
    qp_viewport(device, l, t, r, b);
    while (remaining > 0) {
        uint32_t transmit = QP_MIN(remaining, pixels_in_pixdata);
        if (!qp_pixdata(device, qp_global_pixdata_buffer, transmit)) {
            return false;
        }
        remaining -= transmit;
    }
    return true;
}

bool qp_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    // Cater for cases where people have submitted the coordinates backwards
    uint16_t l = QP_MIN(left, right);
    uint16_t r = QP_MAX(left, right);
    uint16_t t = QP_MIN(top, bottom);
    uint16_t b = QP_MAX(top, bottom);
    uint16_t w = r - l + 1;
    uint16_t h = b - t + 1;

    if (filled) {
        // Fill up the pixdata buffer with the required number of native pixels
        qp_fill_pixdata(device, w * h, hue, sat, val);

        // Perform the draw
        return qp_fillrect_helper_impl(device, l, t, r, b);
    } else {
        // Fill up the pixdata buffer with the required number of native pixels
        qp_fill_pixdata(device, QP_MAX(w, h), hue, sat, val);

        // Draw 4x filled rects to create an outline
        if (!qp_fillrect_helper_impl(device, l, t, r, t)) return false;
        if (!qp_fillrect_helper_impl(device, l, b, r, b)) return false;
        if (!qp_fillrect_helper_impl(device, l, t + 1, l, b - 1)) return false;
        if (!qp_fillrect_helper_impl(device, r, t + 1, r, b - 1)) return false;
    }

    return true;
}
