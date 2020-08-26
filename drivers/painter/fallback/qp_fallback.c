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

#include "qp_fallback.h"

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
        // TODO: Angled lines
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
