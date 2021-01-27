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

#pragma once

#include <qp.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter RGB565 surface device
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Factory method for creating a new surface
painter_device_t qp_rgb565_surface_make_device(uint16_t width, uint16_t height);

// Get access to the buffer and its length
const void* const qp_rgb565_surface_get_buffer_ptr(painter_device_t device);
uint32_t          qp_rgb565_surface_get_pixel_count(painter_device_t device);