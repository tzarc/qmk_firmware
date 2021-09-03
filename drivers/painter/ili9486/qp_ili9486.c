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
#include <wait.h>
#include <qp.h>
#include "qp_ili9486.h"
#include "qp_ili9486_extra_opcodes.h"
#include <qp_internal.h>
#include <qp_fallback.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool qp_ili9486_init(painter_device_t device, painter_rotation_t rotation) {
    static const uint8_t pgamma[15] = {0x1F, 0x25, 0x22, 0x0B, 0x06, 0x0A, 0x4E, 0xC6, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t ngamma[15] = {0x1F, 0x3F, 0x3F, 0x0F, 0x1F, 0x0F, 0x46, 0x49, 0x31, 0x05, 0x09, 0x03, 0x1C, 0x1A, 0x00 };

    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->rotation                 = rotation;

    //qp_ili9xxx_internal_lcd_init(lcd);
    // Set up pin directions and initial values
    setPinOutput(lcd->reset_pin);

    // Perform a reset
    writePinLow(lcd->reset_pin);
    wait_ms(50);
    writePinHigh(lcd->reset_pin);
    wait_ms(100);

    // Enable the comms to the LCD
    qp_ili9xxx_internal_lcd_start(lcd);
    wait_ms(20);

   // turn display off before we start doing anything
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_CMD_DISPLAY_OFF);
    wait_ms(20);

    // set 50% brightness
    qp_ili9xxx_internal_lcd_reg(lcd, ILI9XXX_SET_BRIGHTNESS, 0xFF);

    // Configure power control
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_POWER_CTL_1);
    qp_ili9xxx_internal_lcd_data(lcd, 0x19);
    qp_ili9xxx_internal_lcd_data(lcd, 0x1A);

    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_POWER_CTL_2);
    qp_ili9xxx_internal_lcd_data(lcd, 0x45);
    qp_ili9xxx_internal_lcd_data(lcd, 0x00);

    // Power on default
    qp_ili9xxx_internal_lcd_reg(lcd, ILI9486_SET_POWER_CTL_NORMAL, 0x33);

    // Configure VCOM control
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_VCOM_CTL_1);
    qp_ili9xxx_internal_lcd_data(lcd, 0x00);
    qp_ili9xxx_internal_lcd_data(lcd, 0x28);

    // Configure frame rate control
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_FRAME_CTL_NORMAL);
    qp_ili9xxx_internal_lcd_data(lcd, 0xA0);
    qp_ili9xxx_internal_lcd_data(lcd, 0x11);

    // Configure inversion
    qp_ili9xxx_internal_lcd_reg(lcd, ILI9XXX_SET_INVERSION_CTL, 0x02);

    // Control function
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_FUNCTION_CTL);
    qp_ili9xxx_internal_lcd_data(lcd, 0x00);
    qp_ili9xxx_internal_lcd_data(lcd, 0x42);
    qp_ili9xxx_internal_lcd_data(lcd, 0x3B);

    // Set positive gamma
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_PGAMMA);
    qp_ili9xxx_internal_lcd_sendbuf(lcd, pgamma, sizeof(pgamma));

    // Set negative gamma
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_NGAMMA);
    qp_ili9xxx_internal_lcd_sendbuf(lcd, ngamma, sizeof(ngamma));

    // Set the pixel format
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_PIX_FMT);
    qp_ili9xxx_internal_lcd_data(lcd, 0x55);


    //  From original driver, but register numbers don't make any sense.
    if (0)
    {
        qp_ili9xxx_internal_lcd_cmd(lcd, 0XF1);
        qp_ili9xxx_internal_lcd_data(lcd, 0x36);
        qp_ili9xxx_internal_lcd_data(lcd, 0x04);
        qp_ili9xxx_internal_lcd_data(lcd, 0x00);
        qp_ili9xxx_internal_lcd_data(lcd, 0x3C);
        qp_ili9xxx_internal_lcd_data(lcd, 0x0F);
        qp_ili9xxx_internal_lcd_data(lcd, 0x0F);
        qp_ili9xxx_internal_lcd_data(lcd, 0xA4);
        qp_ili9xxx_internal_lcd_data(lcd, 0x02);

        qp_ili9xxx_internal_lcd_cmd(lcd, 0XF2);
        qp_ili9xxx_internal_lcd_data(lcd, 0x18);
        qp_ili9xxx_internal_lcd_data(lcd, 0xA3);
        qp_ili9xxx_internal_lcd_data(lcd, 0x12);
        qp_ili9xxx_internal_lcd_data(lcd, 0x02);
        qp_ili9xxx_internal_lcd_data(lcd, 0x32);
        qp_ili9xxx_internal_lcd_data(lcd, 0x12);
        qp_ili9xxx_internal_lcd_data(lcd, 0xFF);
        qp_ili9xxx_internal_lcd_data(lcd, 0x32);
        qp_ili9xxx_internal_lcd_data(lcd, 0x00);

        qp_ili9xxx_internal_lcd_cmd(lcd, 0XF4);
        qp_ili9xxx_internal_lcd_data(lcd, 0x40);
        qp_ili9xxx_internal_lcd_data(lcd, 0x00);
        qp_ili9xxx_internal_lcd_data(lcd, 0x08);
        qp_ili9xxx_internal_lcd_data(lcd, 0x91);
        qp_ili9xxx_internal_lcd_data(lcd, 0x04);

        qp_ili9xxx_internal_lcd_cmd(lcd, 0XF8);
        qp_ili9xxx_internal_lcd_data(lcd, 0x21);
        qp_ili9xxx_internal_lcd_data(lcd, 0x04);
    }

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        case QP_ROTATION_0:
            qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_MEM_ACS_CTL);
            qp_ili9xxx_internal_lcd_data(lcd, 0b00001000);
            break;
        case QP_ROTATION_90:
            qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_MEM_ACS_CTL);
            qp_ili9xxx_internal_lcd_data(lcd, 0b10101000);
            break;
        case QP_ROTATION_180:
            qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_MEM_ACS_CTL);
            qp_ili9xxx_internal_lcd_data(lcd, 0b11001000);
            break;
        case QP_ROTATION_270:
            qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_MEM_ACS_CTL);
            qp_ili9xxx_internal_lcd_data(lcd, 0b01101000);
            break;
    }

    // Set the default viewport to be fullscreen
    qp_ili9xxx_internal_lcd_viewport(lcd, 0, 0, lcd->qp_driver.screen_width - 1 , lcd->qp_driver.screen_height - 1);

    // Disable sleep mode
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_CMD_SLEEP_OFF);
    wait_ms(20);

    // turn off inversion
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9486_CMD_INVERT_OFF);
    wait_ms(20);


    // Control function
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_SET_FUNCTION_CTL);
    qp_ili9xxx_internal_lcd_data(lcd, 0x00);
    qp_ili9xxx_internal_lcd_data(lcd, 0x22);

     // turn display on
    qp_ili9xxx_internal_lcd_cmd(lcd, ILI9XXX_CMD_DISPLAY_ON);
    wait_ms(20);


    // Disable the comms to the LCD
    qp_ili9xxx_internal_lcd_stop();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
static ili9xxx_painter_device_t drivers[ILI9486_NUM_DEVICES] = {0};

// Factory function for creating a handle to the ILI9486 device
painter_device_t qp_ili9486_make_device_spi(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight) {
    for (uint32_t i = 0; i < ILI9486_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &drivers[i];
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->qp_driver.init      = qp_ili9486_init;
            driver->qp_driver.clear     = qp_ili9xxx_clear;
            driver->qp_driver.power     = qp_ili9xxx_power;
            driver->qp_driver.pixdata   = qp_ili9xxx_pixdata;
            driver->qp_driver.viewport  = qp_ili9xxx_viewport;
            driver->qp_driver.setpixel  = qp_ili9xxx_setpixel;
            driver->qp_driver.line      = qp_ili9xxx_line;
            driver->qp_driver.rect      = qp_ili9xxx_rect;
            driver->qp_driver.circle    = qp_fallback_circle;
            driver->qp_driver.ellipse   = qp_fallback_ellipse;
            driver->qp_driver.drawimage = qp_ili9xxx_drawimage;
            driver->qp_driver.drawtext  = qp_ili9xxx_drawtext;
            driver->qp_driver.brightness    = qp_ili9xxx_brightness;
            driver->reset_pin               = reset_pin;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight          = uses_backlight;
#endif
            driver->qp_driver.comms_interface       = SPI;
            driver->qp_driver.screen_height         = height;
            driver->qp_driver.screen_width          = width;
            driver->qp_driver.spi.chip_select_pin   = chip_select_pin;
            driver->qp_driver.spi.dc_pin            = dc_pin;
            driver->qp_driver.spi.clock_divisor     = spi_divisor;
            return (painter_device_t)driver;
        }
    }
    return NULL;
}

painter_device_t qp_ili9486_make_device_parallel(pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, pin_t write_pin, pin_t read_pin, const pin_t* data_pin_map, uint8_t data_pin_count, bool uses_backlight) {
    for (uint32_t i = 0; i < ILI9486_NUM_DEVICES; ++i) {
        ili9xxx_painter_device_t *driver = &drivers[i];
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->qp_driver.init      = qp_ili9486_init;
            driver->qp_driver.clear     = qp_ili9xxx_clear;
            driver->qp_driver.power     = qp_ili9xxx_power;
            driver->qp_driver.pixdata   = qp_ili9xxx_pixdata;
            driver->qp_driver.viewport  = qp_ili9xxx_viewport;
            driver->qp_driver.setpixel  = qp_ili9xxx_setpixel;
            driver->qp_driver.line      = qp_ili9xxx_line;
            driver->qp_driver.rect      = qp_ili9xxx_rect;
            driver->qp_driver.circle    = qp_fallback_circle;
            driver->qp_driver.ellipse   = qp_fallback_ellipse;
            driver->qp_driver.drawimage = qp_ili9xxx_drawimage;
            driver->qp_driver.drawtext  = qp_ili9xxx_drawtext;
            driver->qp_driver.brightness    = qp_ili9xxx_brightness;
            driver->reset_pin               = reset_pin;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight          = uses_backlight;
#endif
            driver->qp_driver.comms_interface           = PARALLEL;
            driver->qp_driver.screen_height             = height;
            driver->qp_driver.screen_width              = width;
            driver->qp_driver.parallel.chip_select_pin  = chip_select_pin;
            driver->qp_driver.parallel.write_pin        = write_pin;
            driver->qp_driver.parallel.read_pin         = read_pin;
            driver->qp_driver.parallel.dc_pin           = dc_pin;
            driver->qp_driver.parallel.data_pin_map     = data_pin_map;
            driver->qp_driver.parallel.data_pin_count   = data_pin_count;
            return (painter_device_t)driver;
        }
    }
    return NULL;
}
