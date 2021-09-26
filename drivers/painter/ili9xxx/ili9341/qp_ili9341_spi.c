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

#include <qp_internal.h>
#include <qp_comms_spi.h>
#include <qp_ili9341.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9341_internal.h>

#ifdef QUANTUM_PAINTER_ILI9341_SPI_ENABLE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const struct painter_comms_vtable_t QP_RESIDENT_FLASH spi_comms_vtable = {
    .comms_init  = qp_comms_spi_init,
    .comms_start = qp_comms_spi_start,
    .comms_send  = qp_ili9xxx_spi_send_data_dc_pin,
    .comms_stop  = qp_comms_spi_stop,
};

static const struct painter_driver_TEMP_FUNC_vtable_t QP_RESIDENT_FLASH TEMP_vtable = {
    .drawimage = qp_ili9xxx_drawimage,
    .drawtext  = qp_ili9xxx_drawtext,
};

static const struct ili9xxx_painter_device_vtable_t QP_RESIDENT_FLASH spi_ili9xxx_vtable = {
    .send_cmd8 = qp_ili9xxx_spi_cmd8,
};

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_ili9341_make_spi_device(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor) {
    for (uint32_t i = 0; i < ILI9341_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &ili9341_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &ili9341_driver_vtable;
            driver->qp_driver.comms_vtable          = &spi_comms_vtable;
            driver->qp_driver.TEMP_vtable           = &TEMP_vtable;
            driver->qp_driver.native_bits_per_pixel = 16;  // RGB565
            driver->ili9xxx_vtable                  = &spi_ili9xxx_vtable;

            // SPI and other pin configuration
            driver->qp_driver.comms_config     = &driver->spi_config;
            driver->spi_config.chip_select_pin = chip_select_pin;
            driver->spi_config.divisor         = spi_divisor;
            driver->spi_config.lsb_first       = false;
            driver->spi_config.mode            = 0;
            driver->dc_pin                     = dc_pin;
            driver->reset_pin                  = reset_pin;
            return (painter_device_t)driver;
        }
    }
    return NULL;
}

#endif  // QUANTUM_PAINTER_ILI9341_SPI_ENABLE
