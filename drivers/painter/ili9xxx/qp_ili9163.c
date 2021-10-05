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
#    include <qp_comms_spi.h>
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

    // clang-format off
    const uint8_t ili9163_init_sequence[] QP_RESIDENT_FLASH = {
        // Command,                 Delay,  N, Data[N]
        ILI9XXX_CMD_RESET,            120,  0,
        ILI9XXX_CMD_SLEEP_OFF,          5,  0,
        ILI9XXX_SET_PIX_FMT,            0,  1, 0x55,
        ILI9XXX_SET_GAMMA,              0,  1, 0x04,
        ILI9XXX_ENABLE_3_GAMMA,         0,  1, 0x01,
        ILI9XXX_SET_FUNCTION_CTL,       0,  2, 0b11111111, 0b00000110,
        ILI9XXX_SET_PGAMMA,             0, 15, 0x36, 0x29, 0x12, 0x22, 0x1C, 0x15, 0x42, 0xB7, 0x2F, 0x13, 0x12, 0x0A, 0x11, 0x0B, 0x06,
        ILI9XXX_SET_NGAMMA,             0, 15, 0x09, 0x16, 0x2D, 0x0D, 0x13, 0x15, 0x40, 0x48, 0x53, 0x0C, 0x1D, 0x25, 0x2E, 0x34, 0x39,
        ILI9XXX_SET_FRAME_CTL_NORMAL,   0,  2, 0x08, 0x02,
        ILI9XXX_SET_POWER_CTL_1,        0,  2, 0x0A, 0x02,
        ILI9XXX_SET_POWER_CTL_2,        0,  1, 0x02,
        ILI9XXX_SET_VCOM_CTL_1,         0,  2, 0x50, 0x63,
        ILI9XXX_SET_VCOM_CTL_2,         0,  1, 0x00,
        ILI9XXX_CMD_PARTIAL_OFF,        0,  0,
        ILI9XXX_CMD_DISPLAY_ON,        20,  0
    };
    // clang-format on

    qp_ili9xxx_bulk_command(device, ili9163_init_sequence, sizeof(ili9163_init_sequence));

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    uint8_t madctl[] QP_RESIDENT_FLASH = {
        [QP_ROTATION_0]   = ILI9XXX_MADCTL_BGR,
        [QP_ROTATION_90]  = ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MX | ILI9XXX_MADCTL_MV,
        [QP_ROTATION_180] = ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MX | ILI9XXX_MADCTL_MY,
        [QP_ROTATION_270] = ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MY,
    };

    qp_ili9xxx_command_databuf(device, ILI9XXX_SET_MEM_ACS_CTL, &madctl[rotation], sizeof(uint8_t));

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
painter_device_t qp_ili9163_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode) {
    for (uint32_t i = 0; i < ILI9163_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &ili9163_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &ili9163_driver_vtable;
            driver->qp_driver.comms_vtable          = &spi_comms_with_dc_vtable;
            driver->qp_driver.screen_width          = screen_width;
            driver->qp_driver.screen_height         = screen_height;
            driver->qp_driver.native_bits_per_pixel = 16;  // RGB565
            driver->ili9xxx_vtable                  = &spi_ili9xxx_vtable;

            // SPI and other pin configuration
            driver->qp_driver.comms_config                         = &driver->spi_dc_reset_config;
            driver->spi_dc_reset_config.spi_config.chip_select_pin = chip_select_pin;
            driver->spi_dc_reset_config.spi_config.divisor         = spi_divisor;
            driver->spi_dc_reset_config.spi_config.lsb_first       = false;
            driver->spi_dc_reset_config.spi_config.mode            = spi_mode;
            driver->spi_dc_reset_config.dc_pin                     = dc_pin;
            driver->spi_dc_reset_config.reset_pin                  = reset_pin;
            return (painter_device_t)driver;
        }
    }
    return NULL;
}

#endif  // QUANTUM_PAINTER_ILI9163_SPI_ENABLE
