/* Copyright 2021 Paul Cotter (@gr1mr3aver)
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
// Quantum Painter ILI9486 configurables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The number of ILI9486 devices we're going to be talking to
#ifndef ILI9486_NUM_DEVICES
#    define ILI9486_NUM_DEVICES 1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9486 device factory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Factory method for an ILI9486 device
painter_device_t qp_ili9486_make_device_spi(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight);
painter_device_t qp_ili9486_make_device_parallel(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, pin_t write_pin, pin_t read_pin, const pin_t* data_pin_map, uint8_t data_pin_count, bool uses_backlight);
