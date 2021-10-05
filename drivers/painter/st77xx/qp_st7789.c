// Copyright 2021 Paul Cotter (@gr1mr3aver)
// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

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

    // Perform a software reset
    qp_comms_command(device, ST77XX_CMD_RESET);
    wait_ms(120);

    // Disable sleep mode
    qp_comms_command(device, ST77XX_CMD_SLEEP_OFF);
    wait_ms(5);

    // Set the pixel format
    qp_comms_command_databyte(device, ST77XX_SET_PIX_FMT, 0x55);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        default:
        case QP_ROTATION_0:
            qp_comms_command_databyte(device, ST77XX_SET_MADCTL, ST77XX_MADCTL_RGB);
            if (lcd->qp_driver.screen_width == 240 && lcd->qp_driver.screen_height == 240) {
                lcd->x_offset = 0;
                lcd->y_offset = 0;
            }
            break;
        case QP_ROTATION_90:
            qp_comms_command_databyte(device, ST77XX_SET_MADCTL, ST77XX_MADCTL_RGB | ST77XX_MADCTL_MX | ST77XX_MADCTL_MV);
            if (lcd->qp_driver.screen_width == 240 && lcd->qp_driver.screen_height == 240) {
                lcd->x_offset = 0;
                lcd->y_offset = 0;
            }
            break;
        case QP_ROTATION_180:
            qp_comms_command_databyte(device, ST77XX_SET_MADCTL, ST77XX_MADCTL_RGB | ST77XX_MADCTL_MX | ST77XX_MADCTL_MY);
            if (lcd->qp_driver.screen_width == 240 && lcd->qp_driver.screen_height == 240) {
                lcd->x_offset = 0;
                lcd->y_offset = 80;
            }
            break;
        case QP_ROTATION_270:
            qp_comms_command_databyte(device, ST77XX_SET_MADCTL, ST77XX_MADCTL_RGB | ST77XX_MADCTL_MV | ST77XX_MADCTL_MY);
            if (lcd->qp_driver.screen_width == 240 && lcd->qp_driver.screen_height == 240) {
                lcd->x_offset = 80;
                lcd->y_offset = 0;
            }
            break;
    }

    // Invert the screen (apparently the pixel format is negated?)
    qp_comms_command(device, ST77XX_CMD_INVERT_ON);
    wait_ms(20);

    // Disable sleep mode
    qp_comms_command(device, ST77XX_CMD_NORMAL_ON);
    wait_ms(20);

    // Turn on display
    qp_comms_command(device, ST77XX_CMD_DISPLAY_ON);
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

// Factory function for creating a handle to the ST7789 device
painter_device_t qp_st7789_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode) {
    for (uint32_t i = 0; i < ST7789_NUM_DEVICES; ++i) {
        st77xx_painter_device_t *driver = &st7789_drivers[i];
        if (!driver->qp_driver.driver_vtable) {
            driver->qp_driver.driver_vtable         = &st7789_driver_vtable;
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

#endif  // QUANTUM_PAINTER_ST7789_SPI_ENABLE
