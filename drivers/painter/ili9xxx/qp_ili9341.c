// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <wait.h>
#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_ili9341.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

#ifdef QUANTUM_PAINTER_ILI9341_SPI_ENABLE
#    include <qp_comms_spi.h>
#endif  // QUANTUM_PAINTER_ILI9341_SPI_ENABLE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
ili9xxx_painter_device_t ili9341_drivers[ILI9341_NUM_DEVICES] = {0};

// Initialization
bool qp_ili9341_init(painter_device_t device, painter_rotation_t rotation) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->rotation                 = rotation;

    // Configure power control
    static const uint8_t power_ctl_a[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    qp_comms_command_databuf(device, ILI9XXX_POWER_CTL_A, power_ctl_a, sizeof(power_ctl_a));
    static const uint8_t power_ctl_b[] = {0x00, 0xD9, 0x30};
    qp_comms_command_databuf(device, ILI9XXX_POWER_CTL_B, power_ctl_b, sizeof(power_ctl_b));
    static const uint8_t power_on_seq[] = {0x64, 0x03, 0x12, 0x81};
    qp_comms_command_databuf(device, ILI9XXX_POWER_ON_SEQ_CTL, power_on_seq, sizeof(power_on_seq));
    qp_comms_command_databyte(device, ILI9XXX_SET_PUMP_RATIO_CTL, 0x20);
    qp_comms_command_databyte(device, ILI9XXX_SET_POWER_CTL_1, 0x26);
    qp_comms_command_databyte(device, ILI9XXX_SET_POWER_CTL_2, 0x11);
    static const uint8_t vcom_ctl_1[] = {0x35, 0x3E};
    qp_comms_command_databuf(device, ILI9XXX_SET_VCOM_CTL_1, vcom_ctl_1, sizeof(vcom_ctl_1));
    qp_comms_command_databyte(device, ILI9XXX_SET_VCOM_CTL_2, 0xBE);

    // Configure timing control
    static const uint8_t drv_timing_ctl_a[] = {0x85, 0x10, 0x7A};
    qp_comms_command_databuf(device, ILI9XXX_DRV_TIMING_CTL_A, drv_timing_ctl_a, sizeof(drv_timing_ctl_a));
    static const uint8_t drv_timing_ctl_b[] = {0x00, 0x00};
    qp_comms_command_databuf(device, ILI9XXX_DRV_TIMING_CTL_B, drv_timing_ctl_b, sizeof(drv_timing_ctl_b));

    // Configure brightness / gamma
    qp_comms_command_databyte(device, ILI9XXX_SET_BRIGHTNESS, 0xFF);
    qp_comms_command_databyte(device, ILI9XXX_ENABLE_3_GAMMA, 0x00);
    qp_comms_command_databyte(device, ILI9XXX_SET_GAMMA, 0x01);
    static const uint8_t pgamma[] = {0x0F, 0x29, 0x24, 0x0C, 0x0E, 0x09, 0x4E, 0x78, 0x3C, 0x09, 0x13, 0x05, 0x17, 0x11, 0x00};
    qp_comms_command_databuf(device, ILI9XXX_SET_PGAMMA, pgamma, sizeof(pgamma));
    static const uint8_t ngamma[] = {0x00, 0x16, 0x1B, 0x04, 0x11, 0x07, 0x31, 0x33, 0x42, 0x05, 0x0C, 0x0A, 0x28, 0x2F, 0x0F};
    qp_comms_command_databuf(device, ILI9XXX_SET_NGAMMA, ngamma, sizeof(ngamma));

    // Set the pixel format
    qp_comms_command_databyte(device, ILI9XXX_SET_PIX_FMT, 0x05);

    // Frame and function control
    static const uint8_t frame_ctl_normal[] = {0x00, 0x1B};
    qp_comms_command_databuf(device, ILI9XXX_SET_FRAME_CTL_NORMAL, frame_ctl_normal, sizeof(frame_ctl_normal));
    static const uint8_t function_ctl[] = {0x0A, 0xA2};
    qp_comms_command_databuf(device, ILI9XXX_SET_FUNCTION_CTL, function_ctl, sizeof(function_ctl));

    // Set the default viewport to be fullscreen
    qp_ili9xxx_viewport(lcd, 0, 0, 239, 319);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        default:
        case QP_ROTATION_0:
            qp_comms_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, ILI9XXX_MADCTL_BGR);
            break;
        case QP_ROTATION_90:
            qp_comms_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MX | ILI9XXX_MADCTL_MV);
            break;
        case QP_ROTATION_180:
            qp_comms_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MX | ILI9XXX_MADCTL_MY);
            break;
        case QP_ROTATION_270:
            qp_comms_command_databyte(device, ILI9XXX_SET_MEM_ACS_CTL, ILI9XXX_MADCTL_BGR | ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MY);
            break;
    }

    // Disable sleep mode
    qp_comms_command(device, ILI9XXX_CMD_SLEEP_OFF);
    wait_ms(20);

    return true;
}

// Driver vtable
const struct painter_driver_vtable_t QP_RESIDENT_FLASH ili9341_driver_vtable = {
    .init            = qp_ili9341_init,
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

#ifdef QUANTUM_PAINTER_ILI9341_SPI_ENABLE

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_ili9341_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode) {
    for (uint32_t i = 0; i < ILI9341_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &ili9341_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &ili9341_driver_vtable;
            driver->qp_driver.comms_vtable          = (struct painter_comms_vtable_t QP_RESIDENT_FLASH *)&spi_comms_with_dc_vtable;
            driver->qp_driver.screen_width          = screen_width;
            driver->qp_driver.screen_height         = screen_height;
            driver->qp_driver.native_bits_per_pixel = 16;  // RGB565

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

#endif  // QUANTUM_PAINTER_ILI9341_SPI_ENABLE
