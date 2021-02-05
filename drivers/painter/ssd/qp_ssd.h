/* Copyright 2021 Michael Spradling <mike@mspradling.com>
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

/*
 * Quantum Painter SSD configurables
 */

// The number of SH1106  devices we're going to be talking to
#ifndef SSD_NUM_DEVICES
#    define SSD_NUM_DEVICES 1
#endif

// Resolution
#ifndef SSD_RESOLUTION_X
#    define SSD_RESOLUTION_X 128
#endif

// Resolution
#ifndef SSD_RESOLUTION_Y
#    define SSD_RESOLUTION_Y 32
#endif

// Some SSDs are manufactored with a smaller screen than controller.  In this
// case you need to offset the starting column. (e.g. SH1106 is usually 2)
#ifndef SSD_COLUMN_OFFSET
#    define SSD_COLUMN_OFFSET 0
#endif

// Number of addresses page regions in LCD
#ifndef SSD_PAGE_COUNT
#    define SSD_PAGE_COUNT 8
#endif

#ifndef SSD_SPI_MODE
#    define SSD_SPI_MODE 3
#endif

#ifndef SSD_SPI_LSB_FIRST
#    define SSD_SPI_LSB_FIRST false
#endif

#ifndef QUANTUM_PAINTER_MONO2_SURFACE_ENABLE
#    error "mono2_surface driver required for SSD driver"
#endif

/*
 * Quantum Painter SSD device factory
 */

#ifdef QUANTUM_PAINTER_BUS_SPI
/* Factory function for creating a handle to the SSD Driver with SPI Bus */
painter_device_t qp_ssd_spi_make_device(pin_t chip_select_pin, pin_t data_pin, pin_t reset_pin, uint16_t spi_divisor);
#endif

/* Factory function for creating a handle to the SSD Driver with I2C Bus */
#ifdef QUANTUM_PAINTER_BUS_I2C
painter_device_t qp_ssd_i2c_make_device(pin_t reset_pin, uint8_t i2c_addr);
#endif
