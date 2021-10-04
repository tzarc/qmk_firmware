/* Copyright 2021 Paul Cotter (@gr1mr3aver), Nick Brassel (@tzarc)
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
#include <qp_st7789.h>
#include <qp_st77xx_internal.h>
#include <qp_st77xx_opcodes.h>
#include <qp_st7789_opcodes.h>

#ifdef QUANTUM_PAINTER_ST7789_SPI_ENABLE
#    include <qp_comms_spi.h>
#endif  // QUANTUM_PAINTER_ST7789_SPI_ENABLE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
st77xx_painter_device_t st7789_drivers[ST7789_NUM_DEVICES] = {0};

// Initialization
bool qp_st7789_init(painter_device_t device, painter_rotation_t rotation) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;
    lcd->rotation                = rotation;

    // Set the pixel format
    qp_st77xx_command_databyte(device, ST77XX_SET_PIX_FMT, 0x55);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        default:
        case QP_ROTATION_0:
            qp_st77xx_command_databyte(device, ST77XX_SET_MADCTL, 0b11000000);
            break;
        case QP_ROTATION_90:
            qp_st77xx_command_databyte(device, ST77XX_SET_MADCTL, 0b10100000);
            break;
        case QP_ROTATION_180:
            qp_st77xx_command_databyte(device, ST77XX_SET_MADCTL, 0b00000000);
            break;
        case QP_ROTATION_270:
            qp_st77xx_command_databyte(device, ST77XX_SET_MADCTL, 0b01100000);
            break;
    }

    static const uint8_t porch_ctl[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    qp_st77xx_command_databuf(device, ST7789_SET_PORCH_CTL, porch_ctl, sizeof(porch_ctl));
    qp_st77xx_command_databyte(device, ST7789_SET_GATE_CTL, 0x35);
    qp_st77xx_command_databyte(device, ST7789_SET_VCOM, 0x28);
    qp_st77xx_command_databyte(device, ST7789_SET_LCM_CTL, 0x0C);
    static const uint8_t vdv_vrh_on[] = {0x01, 0xFF};
    qp_st77xx_command_databuf(device, ST7789_SET_VDV_VRH_ON, vdv_vrh_on, sizeof(vdv_vrh_on));
    qp_st77xx_command_databyte(device, ST7789_SET_VRH, 0x10);
    qp_st77xx_command_databyte(device, ST7789_SET_VDV, 0x20);
    qp_st77xx_command_databyte(device, ST7789_SET_FRAME_RATE_CTL_2, 0x0F);
    static const uint8_t power_ctl_1[] = {0xA4, 0xA1};
    qp_st77xx_command_databuf(device, ST7789_SET_POWER_CTL_1, power_ctl_1, sizeof(power_ctl_1));
    static const uint8_t pgamma[] = {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17};
    qp_st77xx_command_databuf(device, ST7789_SET_PGAMMA, pgamma, sizeof(pgamma));
    static const uint8_t ngamma[] = {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E};
    qp_st77xx_command_databuf(device, ST7789_SET_NGAMMA, ngamma, sizeof(ngamma));

    // Invert the screen (apparently the pixel format is negated?)
    qp_st77xx_command(device, ST77XX_CMD_INVERT_ON);

    // Disable sleep mode
    qp_st77xx_command(device, ST77XX_CMD_NORMAL_ON);

    // Disable sleep mode
    qp_st77xx_command(device, ST77XX_CMD_SLEEP_OFF);

    // Turn on display
    qp_st77xx_command(device, ST77XX_CMD_DISPLAY_ON);
    wait_ms(20);

    return true;
}

// Driver vtable
const struct painter_driver_vtable_t QP_RESIDENT_FLASH st7789_driver_vtable = {
    .init            = qp_st7789_init,
    .power           = qp_st77xx_power,
    .clear           = qp_st77xx_clear,
    .flush           = qp_st77xx_flush,
    .pixdata         = qp_st77xx_pixdata,
    .viewport        = qp_st77xx_viewport,
    .palette_convert = qp_st77xx_palette_convert,
    .append_pixels   = qp_st77xx_append_pixels,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QUANTUM_PAINTER_ST7789_SPI_ENABLE

static const struct st77xx_painter_device_vtable_t QP_RESIDENT_FLASH spi_st77xx_vtable = {
    .send_cmd8 = qp_comms_spi_dc_reset_send_command,
};

// Factory function for creating a handle to the ST7789 device
painter_device_t qp_st7789_make_spi_device(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor) {
    for (uint32_t i = 0; i < ST7789_NUM_DEVICES; ++i) {
        st77xx_painter_device_t *driver = &st7789_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &st7789_driver_vtable;
            driver->qp_driver.comms_vtable          = &spi_comms_with_dc_vtable;
            driver->qp_driver.native_bits_per_pixel = 16;  // RGB565
            driver->st77xx_vtable                   = &spi_st77xx_vtable;

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

#endif  // QUANTUM_PAINTER_ST7789_SPI_ENABLE
