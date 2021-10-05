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

#pragma once

#include <gpio.h>
#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9341 configurables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The number of ILI9341 devices we're going to be talking to
#ifndef ILI9341_NUM_DEVICES
#    define ILI9341_NUM_DEVICES 1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9341 device factory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Factory method for an ILI9341 device
#ifdef QUANTUM_PAINTER_ILI9341_SPI_ENABLE
painter_device_t qp_ili9341_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode);
#endif  // QUANTUM_PAINTER_ILI9341_SPI_ENABLE
