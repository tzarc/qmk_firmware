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

#include <string.h>
#include <spi_master.h>
#include <wait.h>
#include <qp.h>
#include <qp_ili9341.h>
#include <qp_internal.h>
#include <qp_fallback.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool qp_ili9341_init(painter_device_t device, painter_rotation_t rotation) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->rotation                 = rotation;

    // Initialize the SPI peripheral
    spi_init();

    // Set up pin directions and initial values
    setPinOutput(lcd->spi_config.chip_select_pin);
    writePinHigh(lcd->spi_config.chip_select_pin);

    setPinOutput(lcd->spi_config.dc_pin);
    writePinLow(lcd->spi_config.dc_pin);

    setPinOutput(lcd->spi_config.reset_pin);

    // Perform a reset
    writePinLow(lcd->spi_config.reset_pin);
    wait_ms(20);
    writePinHigh(lcd->spi_config.reset_pin);
    wait_ms(20);

    // Enable the SPI comms to the LCD
    qp_comms_spi_start(&lcd->spi_config);

    // Configure power control
    static const uint8_t power_ctl_a[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_POWER_CTL_A, power_ctl_a, sizeof(power_ctl_a));
    static const uint8_t power_ctl_b[] = {0x00, 0xD9, 0x30};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_POWER_CTL_B, power_ctl_b, sizeof(power_ctl_b));
    static const uint8_t power_on_seq[] = {0x64, 0x03, 0x12, 0x81};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_POWER_ON_SEQ_CTL, power_on_seq, sizeof(power_on_seq));
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_PUMP_RATIO_CTL, 0x20);
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_POWER_CTL_1, 0x26);
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_POWER_CTL_2, 0x11);
    static const uint8_t vcom_ctl_1[] = {0x35, 0x3E};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_SET_VCOM_CTL_1, vcom_ctl_1, sizeof(vcom_ctl_1));
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_VCOM_CTL_2, 0xBE);

    // Configure timing control
    static const uint8_t drv_timing_ctl_a[] = {0x85, 0x10, 0x7A};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_DRV_TIMING_CTL_A, drv_timing_ctl_a, sizeof(drv_timing_ctl_a));
    static const uint8_t drv_timing_ctl_b[] = {0x00, 0x00};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_DRV_TIMING_CTL_B, drv_timing_ctl_b, sizeof(drv_timing_ctl_b));

    // Configure brightness / gamma
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_BRIGHTNESS, 0xFF);
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_ENABLE_3_GAMMA, 0x00);
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_GAMMA, 0x01);
    static const uint8_t pgamma[] = {0x0F, 0x29, 0x24, 0x0C, 0x0E, 0x09, 0x4E, 0x78, 0x3C, 0x09, 0x13, 0x05, 0x17, 0x11, 0x00};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_SET_PGAMMA, pgamma, sizeof(pgamma));
    static const uint8_t ngamma[] = {0x00, 0x16, 0x1B, 0x04, 0x11, 0x07, 0x31, 0x33, 0x42, 0x05, 0x0C, 0x0A, 0x28, 0x2F, 0x0F};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_SET_NGAMMA, ngamma, sizeof(ngamma));

    // Set the pixel format
    qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_PIX_FMT, 0x55);

    // Frame and function control
    static const uint8_t frame_ctl_normal[] = {0x00, 0x1B};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_SET_FRAME_CTL_NORMAL, frame_ctl_normal, sizeof(frame_ctl_normal));
    static const uint8_t function_ctl[] = {0x0A, 0xA2};
    qp_comms_spi_cmd8_databuf(&lcd->spi_config, ILI9XXX_SET_FUNCTION_CTL, function_ctl, sizeof(function_ctl));

    // Set the default viewport to be fullscreen
    qp_ili9xxx_internal_lcd_viewport(lcd, 0, 0, 239, 319);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        case QP_ROTATION_0:
            qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_MEM_ACS_CTL, 0b00001000);
            break;
        case QP_ROTATION_90:
            qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_MEM_ACS_CTL, 0b10101000);
            break;
        case QP_ROTATION_180:
            qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_MEM_ACS_CTL, 0b11001000);
            break;
        case QP_ROTATION_270:
            qp_comms_spi_cmd8_data8(&lcd->spi_config, ILI9XXX_SET_MEM_ACS_CTL, 0b01101000);
            break;
    }

    // Disable sleep mode
    qp_comms_spi_cmd8(&lcd->spi_config, ILI9XXX_CMD_SLEEP_OFF);
    wait_ms(20);

    // Disable the SPI comms to the LCD
    qp_comms_spi_stop(&lcd->spi_config);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver vtable
static const struct painter_driver_vtable_t QP_RESIDENT_FLASH driver_vtable = {
    .init      = qp_ili9341_init,
    .clear     = qp_ili9xxx_clear,
    .power     = qp_ili9xxx_power,
    .pixdata   = qp_ili9xxx_pixdata,
    .viewport  = qp_ili9xxx_viewport,
    .setpixel  = qp_ili9xxx_setpixel,
    .line      = qp_ili9xxx_line,
    .rect      = qp_ili9xxx_rect,
    .circle    = qp_fallback_circle,
    .ellipse   = qp_fallback_ellipse,
    .drawimage = qp_ili9xxx_drawimage,
    .drawtext  = qp_ili9xxx_drawtext,
};

// Driver storage
static ili9xxx_painter_device_t drivers[ILI9341_NUM_DEVICES] = {0};

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_ili9341_make_device(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight) {
    for (uint32_t i = 0; i < ILI9341_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &drivers[i];
        if (!driver->qp_driver.vtable) {
            driver->qp_driver.vtable           = &driver_vtable;
            driver->spi_config.chip_select_pin = chip_select_pin;
            driver->spi_config.dc_pin          = dc_pin;
            driver->spi_config.reset_pin       = reset_pin;
            driver->spi_config.divisor         = spi_divisor;
            driver->spi_config.lsb_first       = false;
            driver->spi_config.mode            = 0;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight = uses_backlight;
#endif
            return (painter_device_t)driver;
        }
    }
    return NULL;
}
