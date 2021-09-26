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

#ifdef QUANTUM_PAINTER_SPI_ENABLE

#    include <stddef.h>
#    include <stdint.h>
#    include <gpio.h>

#    include <qp_internal.h>

struct qp_comms_spi_config_t {
    pin_t    chip_select_pin;
    uint16_t divisor;
    bool     lsb_first;
    int8_t   mode;
};

bool   qp_comms_spi_init(painter_device_t device);
bool   qp_comms_spi_start(painter_device_t device);
size_t qp_comms_spi_send_data(painter_device_t device, const void *data, size_t byte_count);
void   qp_comms_spi_stop(painter_device_t device);

#endif  // QUANTUM_PAINTER_SPI_ENABLE
