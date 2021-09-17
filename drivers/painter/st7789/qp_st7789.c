/* Copyright 2021 Paul Cotter (@gr1mr3aver)
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
#include "print.h"

#include <qp.h>
#include <qp_internal.h>
#include <qp_utils.h>
#include <qp_fallback.h>
#include "qp_st7789.h"
#include "qp_st7789_opcodes.h"
#include <qp_st77xx.h>
#include <qp_st77xx_internal.h>
#include <qp_st77xx_opcodes.h>

// Driver storage
static st77xx_painter_device_t drivers[ST7789_NUM_DEVICES] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool qp_st7789_init(painter_device_t device, painter_rotation_t rotation) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;
    lcd->rotation                 = rotation;

    qp_st77xx_internal_lcd_init(lcd);

    // Perform a HW reset
    writePinLow(lcd->reset_pin);
    wait_ms(20);
    writePinHigh(lcd->reset_pin);
    wait_ms(100);

    // Enable the SPI comms to the LCD
    qp_st77xx_internal_lcd_start(lcd);

    // Set rgb565 color format
    qp_st77xx_internal_lcd_reg(lcd, ST77XX_SET_PIX_FMT, 0x55);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        case QP_ROTATION_0:
            qp_st77xx_internal_lcd_reg(lcd, ST77XX_SET_MADCTL, 0b11000000);
            break;
        case QP_ROTATION_90:
            qp_st77xx_internal_lcd_reg(lcd, ST77XX_SET_MADCTL, 0b10100000);
            break;
        case QP_ROTATION_180:
            qp_st77xx_internal_lcd_reg(lcd, ST77XX_SET_MADCTL, 0b00000000);
            break;
        case QP_ROTATION_270:
            qp_st77xx_internal_lcd_reg(lcd, ST77XX_SET_MADCTL, 0b01100000);
            break;
    }

    // Turn on Inversion - THIS IS A HACK
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_CMD_INVERT_ON);

    // Turn on Normal mode
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_CMD_NORMAL_ON);
    wait_ms(20);

    // Turn off Sleep mode
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_CMD_SLEEP_OFF);
    wait_ms(20);

    // Turn on display
    qp_st77xx_internal_lcd_cmd(lcd, ST77XX_CMD_DISPLAY_ON);
    wait_ms(20);

    // Disable the SPI comms to the LCD
    qp_st77xx_internal_lcd_stop(lcd);

    return true;
}

bool qp_st7789_brightness(painter_device_t device, uint8_t val) {
    st77xx_painter_device_t *lcd = (st77xx_painter_device_t *)device;
    qp_st77xx_internal_lcd_start(lcd);

    qp_st77xx_internal_lcd_reg(lcd, ST7789_SET_BRIGHTNESS, val);

    qp_st77xx_internal_lcd_stop(lcd);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Factory function for creating a handle to the ST7789 device using SPI interface
painter_device_t qp_st7789_make_device_spi(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t data_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight) {

    for (uint32_t i = 0; i < ST7789_NUM_DEVICES; ++i) {
        st77xx_painter_device_t *driver = &drivers[i];
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->dc_pin              = data_pin;
            driver->reset_pin           = reset_pin;
            driver->qp_driver.init      = qp_st7789_init;
            driver->qp_driver.clear     = qp_st77xx_clear;
            driver->qp_driver.power     = qp_st77xx_power;
            driver->qp_driver.viewport  = qp_st77xx_viewport;
            driver->qp_driver.brightness    = qp_st7789_brightness;
            driver->qp_driver.screen_height       = screen_height;
            driver->qp_driver.screen_width        = screen_width;
            driver->qp_driver.comms_interface     = SPI;
            driver->qp_driver.spi.chip_select_pin = chip_select_pin;
            driver->qp_driver.spi.clock_divisor   = spi_divisor;
            driver->qp_driver.spi.spi_mode        = LCD_SPI_MODE;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight = uses_backlight;
#endif
            return (painter_device_t)driver;
        }
    }

    return NULL;
}

painter_device_t qp_st7789_make_device_parallel(uint16_t screen_height, uint16_t screen_width, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, pin_t write_pin, pin_t read_pin, bool uses_backlight) {
    for (uint32_t i = 0; i < ST7789_NUM_DEVICES; ++i) {
        st77xx_painter_device_t *driver = &drivers[i];
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->reset_pin           = reset_pin;
            driver->dc_pin              = dc_pin;
            driver->qp_driver.init      = qp_st7789_init;
            driver->qp_driver.power     = qp_st77xx_power;
            driver->qp_driver.viewport  = qp_st77xx_viewport;
            driver->qp_driver.brightness    = qp_st7789_brightness;
            driver->qp_driver.screen_height             = screen_height;
            driver->qp_driver.screen_width              = screen_width;
            driver->qp_driver.comms_interface           = PARALLEL;
            driver->qp_driver.parallel.chip_select_pin  = chip_select_pin;
            driver->qp_driver.parallel.write_pin        = write_pin;
            driver->qp_driver.parallel.read_pin         = read_pin;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight          = uses_backlight;
#endif
            return (painter_device_t)driver;
        }
    }
    return NULL;
}





