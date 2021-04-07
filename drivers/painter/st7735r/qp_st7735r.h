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

#include "qp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ST7735R configurables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The number of ST7735R devices we're going to be talking to
#ifndef ST7735R_NUM_DEVICES
#    define ST7735R_NUM_DEVICES 1
#endif

// The buffer size to use when rendering chunks of data, allows limiting of RAM allocation when rendering images
#ifndef ST7735R_PIXDATA_BUFSIZE
#    define ST7735R_PIXDATA_BUFSIZE 32
#endif

#if ST7735R_PIXDATA_BUFSIZE < 16
#    error ST7735R pixel buffer size too small -- ST7735R_PIXDATA_BUFSIZE must be >= 16
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ST7735R device factory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Factory method for an ST7735R device
painter_device_t qp_st7735r_make_device(pin_t chip_select_pin, pin_t data_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight);
