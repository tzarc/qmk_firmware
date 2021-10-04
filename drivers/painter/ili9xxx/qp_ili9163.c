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

#include <wait.h>
#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_ili9163.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

#ifdef QUANTUM_PAINTER_ILI9163_SPI_ENABLE
#include <qp_comms_spi.h>
#endif  // QUANTUM_PAINTER_ILI9163_SPI_ENABLE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
ili9xxx_painter_device_t ili9163_drivers[ILI9163_NUM_DEVICES] = {0};

// Initialization
bool qp_ili9163_init(painter_device_t device, painter_rotation_t rotation) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->rotation                 = rotation;

    qp_ili9xxx_command(device, ILI9XXX_CMD_SLEEP_OFF);
    qp_ili9xxx_command_databyte(device, ILI9XXX_SET_PIX_FMT, 0x05);
    qp_ili9xxx_command_databyte(device, ILI9XXX_SET_GAMMA, 0x04);
    qp_ili9xxx_command_databyte(device, ILI9XXX_ENABLE_3_GAMMA, 0x01);
    qp_ili9xxx_command(device, ILI9XXX_CMD_PARTIAL_OFF);

    static const uint8_t function_ctl[] = {0b11111111, 0b00000110};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_FUNCTION_CTL, function_ctl, sizeof(function_ctl));

    static const uint8_t pgamma[] = {0x36,0x29,0x12,0x22,0x1C,0x15,0x42,0xB7,0x2F,0x13,0x12,0x0A,0x11,0x0B,0x06};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_PGAMMA, pgamma, sizeof(pgamma));
    static const uint8_t ngamma[] = {0x09,0x16,0x2D,0x0D,0x13,0x15,0x40,0x48,0x53,0x0C,0x1D,0x25,0x2E,0x34,0x39};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_NGAMMA, ngamma, sizeof(ngamma));

    static const uint8_t frame_ctl_normal[] = {0x08, 0x02};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_FRAME_CTL_NORMAL, frame_ctl_normal, sizeof(frame_ctl_normal));

    static const uint8_t power_ctl_1[] = {0x0A, 0x02};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_POWER_CTL_1, power_ctl_1, sizeof(power_ctl_1));
    qp_ili9xxx_command_databyte(device, ILI9XXX_SET_POWER_CTL_2, 0x02);
    static const uint8_t vcom_ctl_1[] = {0x50, 0x63};
    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_VCOM_CTL_1, vcom_ctl_1, sizeof(vcom_ctl_1));
    qp_ili9xxx_command_databyte(device, ILI9XXX_SET_VCOM_CTL_2, 0);

    qp_ili9xxx_command(device, ILI9XXX_CMD_DISPLAY_ON);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        default:
        case QP_ROTATION_0:
            qp_ili9xxx_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, 0b00001000);
            break;
        case QP_ROTATION_90:
            qp_ili9xxx_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, 0b10101000);
            break;
        case QP_ROTATION_180:
            qp_ili9xxx_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, 0b11001000);
            break;
        case QP_ROTATION_270:
            qp_ili9xxx_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, 0b01101000);
            break;
    }

    // Disable sleep mode
    qp_ili9xxx_command(device, ILI9XXX_CMD_SLEEP_OFF);
    wait_ms(20);

    return true;
}

// Driver vtable
const struct painter_driver_vtable_t QP_RESIDENT_FLASH ili9163_driver_vtable = {
    .init            = qp_ili9163_init,
    .power           = qp_ili9xxx_power,
    .clear           = qp_ili9xxx_clear,
    .flush           = qp_ili9xxx_flush,
    .pixdata         = qp_ili9xxx_pixdata,
    .viewport        = qp_ili9xxx_viewport,
    .palette_convert = qp_ili9xxx_palette_convert,
    .append_pixels   = qp_ili9xxx_append_pixels,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QUANTUM_PAINTER_ILI9163_SPI_ENABLE

static const struct ili9xxx_painter_device_vtable_t QP_RESIDENT_FLASH spi_ili9xxx_vtable = {
    .send_cmd8 = qp_comms_spi_dc_reset_send_command,
};

// Factory function for creating a handle to the ILI9163 device
painter_device_t qp_ili9163_make_spi_device(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor) {
    for (uint32_t i = 0; i < ILI9163_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &ili9163_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &ili9163_driver_vtable;
            driver->qp_driver.comms_vtable          = &spi_comms_with_dc_vtable;
            driver->qp_driver.native_bits_per_pixel = 16;  // RGB565
            driver->ili9xxx_vtable                  = &spi_ili9xxx_vtable;

            // SPI and other pin configuration
            driver->qp_driver.comms_config                         = &driver->spi_dc_reset_config;
            driver->spi_dc_reset_config.spi_config.chip_select_pin = chip_select_pin;
            driver->spi_dc_reset_config.spi_config.divisor         = spi_divisor;
            driver->spi_dc_reset_config.spi_config.lsb_first       = false;
            driver->spi_dc_reset_config.spi_config.mode            = 0;
            driver->spi_dc_reset_config.dc_pin                     = dc_pin;
            driver->spi_dc_reset_config.reset_pin                  = reset_pin;
            return (painter_device_t)driver;
        }
    }
    return NULL;
}

#endif  // QUANTUM_PAINTER_ILI9163_SPI_ENABLE
