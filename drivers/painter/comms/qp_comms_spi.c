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

#include <gpio.h>
#include <spi_master.h>

#include <qp.h>
#include <qp_internal.h>
#include "qp_comms_spi.h"

bool qp_comms_spi_init(painter_device_t device) {
    struct painter_driver_t *     driver     = (struct painter_driver_t *)device;
    struct qp_comms_spi_config_t *spi_config = (struct qp_comms_spi_config_t *)driver->comms_config;

    // Initialize the SPI peripheral
    spi_init();

    // Set up CS as output high
    setPinOutput(spi_config->chip_select_pin);
    writePinHigh(spi_config->chip_select_pin);

    return true;
}

bool qp_comms_spi_start(painter_device_t device) {
    struct painter_driver_t *     driver     = (struct painter_driver_t *)device;
    struct qp_comms_spi_config_t *spi_config = (struct qp_comms_spi_config_t *)driver->comms_config;

    return spi_start(spi_config->chip_select_pin, spi_config->lsb_first, spi_config->mode, spi_config->divisor);
}

size_t qp_comms_spi_send_data(painter_device_t device, const void *data, size_t byte_count) {
    uint32_t       bytes_remaining = byte_count;
    const uint8_t *p               = (const uint8_t *)data;
    while (bytes_remaining > 0) {
        uint32_t bytes_this_loop = bytes_remaining < 1024 ? bytes_remaining : 1024;
        spi_transmit(p, bytes_this_loop);
        p += bytes_this_loop;
        bytes_remaining -= bytes_this_loop;
    }

    return byte_count - bytes_remaining;
}

void qp_comms_spi_stop(painter_device_t device) { spi_stop(); }
