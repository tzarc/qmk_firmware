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

#include "spi_master.h"

#include "qp.h"
#include "qp_st7735r.h"
#include "qp_internal.h"
#include "qp_st7735r_internal.h"
#include "qp_st7735r_opcodes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool qp_st7735r_init(painter_device_t device, painter_rotation_t rotation) {
    static const uint8_t pgamma[16] = {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    static const uint8_t ngamma[16] = {0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d, 0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10};

    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    lcd->rotation                 = rotation;

    // Initialize the SPI peripheral
    spi_init();

    // Set up pin directions and initial values
    setPinOutput(lcd->chip_select_pin);
    writePinHigh(lcd->chip_select_pin);

    setPinOutput(lcd->data_pin);
    writePinLow(lcd->data_pin);

    // Perform a reset
    //// TODO: Saving for future use - maybe an IFDEF to account for SW vs. HW reset?
    // setPinOutput(lcd->reset_pin);
    // writePinLow(lcd->reset_pin);
    // wait_ms(20);
    // writePinHigh(lcd->reset_pin);
    // wait_ms(20);

    // Enable the SPI comms to the LCD
    qp_st7735r_internal_lcd_start(lcd);

    // Perform a software reset
    qp_st7735r_internal_lcd_cmd(ST7735R_CMD_RESET);
    wait_ms(100);
    qp_st7735r_internal_lcd_cmd(ST7735R_CMD_SLEEP_OFF);
    wait_ms(100);

    // Set frame rate (Normal mode)
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_FRAME_CTL_NORMAL);
    qp_st7735r_internal_lcd_data(lcd, 0x01);
    qp_st7735r_internal_lcd_data(lcd, 0x2C);
    qp_st7735r_internal_lcd_data(lcd, 0x2D);

   // Set frame rate (Idle mode)
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_FRAME_CTL_IDLE);
    qp_st7735r_internal_lcd_data(lcd, 0x01);
    qp_st7735r_internal_lcd_data(lcd, 0x2C);
    qp_st7735r_internal_lcd_data(lcd, 0x2D);

    // Set frame rate (Partial mode)
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_FRAME_CTL_PARTIAL);
    qp_st7735r_internal_lcd_data(lcd, 0x01);
    qp_st7735r_internal_lcd_data(lcd, 0x2C);
    qp_st7735r_internal_lcd_data(lcd, 0x2D);
    qp_st7735r_internal_lcd_data(lcd, 0x01);
    qp_st7735r_internal_lcd_data(lcd, 0x2C);
    qp_st7735r_internal_lcd_data(lcd, 0x2D);

    // Configure inversion
    // frame for all modes
    qp_st7735r_internal_lcd_reg(lcd, ST7735R_SET_INVERSION_CTL, 0x07);
    // line for all modes
    // qp_st7735r_internal_lcd_reg(lcd, ST7735R_SET_INVERSION_CTL, 0x00);


    // Configure power control
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_POWER_CTL_1);
    qp_st7735r_internal_lcd_data(lcd, 0xa2);
    qp_st7735r_internal_lcd_data(lcd, 0x02);
    qp_st7735r_internal_lcd_data(lcd, 0x84);

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_POWER_CTL_2);
    qp_st7735r_internal_lcd_data(lcd, 0xc5);

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_POWER_CTL_NORMAL);
    qp_st7735r_internal_lcd_data(lcd, 0x0a);
    qp_st7735r_internal_lcd_data(lcd, 0x00);

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_POWER_CTL_IDLE);
    qp_st7735r_internal_lcd_data(lcd, 0x8a);
    qp_st7735r_internal_lcd_data(lcd, 0x2a);

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_POWER_CTL_PARTIAL);
    qp_st7735r_internal_lcd_data(lcd, 0x8a);
    qp_st7735r_internal_lcd_data(lcd, 0xee);

    qp_st7735r_internal_lcd_reg(lcd, ST7735R_SET_VCOM_CTL_1, 0x0E);

    // Turn off inversion
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_CMD_INVERT_OFF);

    // Use full color mode
    qp_st7735r_internal_lcd_reg(lcd, ST7735R_SET_PIX_FMT, 0x05);

    // Configure gamma settings
    qp_st7735r_internal_lcd_reg(lcd, ST7735R_SET_GAMMA, 0x01);

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_PGAMMA);
    qp_st7735r_internal_lcd_sendbuf(lcd, pgamma, sizeof(pgamma));

    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_NGAMMA);
    qp_st7735r_internal_lcd_sendbuf(lcd, ngamma, sizeof(ngamma));

    // Set refresh bottom->top
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM_ACS_CTL);
    wait_ms(20);

    // Configure the rotation (i.e. the ordering and direction of memory writes in GRAM)
    switch (rotation) {
        case QP_ROTATION_0:
            qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM_ACS_CTL);
            qp_st7735r_internal_lcd_data(lcd, 0b00001000);
            break;
        case QP_ROTATION_90:
            qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM_ACS_CTL);
            qp_st7735r_internal_lcd_data(lcd, 0b10101000);
            break;
        case QP_ROTATION_180:
            qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM_ACS_CTL);
            qp_st7735r_internal_lcd_data(lcd, 0b11001000);
            break;
        case QP_ROTATION_270:
            qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM_ACS_CTL);
            qp_st7735r_internal_lcd_data(lcd, 0b01101000);
            break;
    }

    // Set the default viewport to be fullscreen
    qp_st7735r_internal_lcd_viewport(lcd, 0, 0, 239, 319);

    // Turn on Normal mode
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_CMD_NORMAL_ON);
    wait_ms(20);

    // Turn on display
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_CMD_DISPLAY_ON);
    wait_ms(20);

    // Disable the SPI comms to the LCD
    qp_st7735r_internal_lcd_stop();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
static st7735r_painter_device_t drivers[ST7735R_NUM_DEVICES] = {0};

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_st7735r_make_device(pin_t chip_select_pin, pin_t data_pin, pin_t reset_pin, uint16_t spi_divisor, bool uses_backlight) {
    for (uint32_t i = 0; i < ST7735R_NUM_DEVICES; ++i) {
        st7735r_painter_device_t *driver = &drivers[i];
        memset(driver, 0, sizeof(st7735r_painter_device_t));
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->qp_driver.init      = qp_st7735r_init;
            driver->qp_driver.clear     = qp_st7735r_clear;
            driver->qp_driver.power     = qp_st7735r_power;
            driver->qp_driver.pixdata   = qp_st7735r_pixdata;
            driver->qp_driver.viewport  = qp_st7735r_viewport;
            driver->qp_driver.setpixel  = qp_st7735r_setpixel;
            driver->qp_driver.line      = qp_st7735r_line;
            driver->qp_driver.rect      = qp_st7735r_rect;
            driver->qp_driver.drawimage = qp_st7735r_drawimage;
            driver->chip_select_pin     = chip_select_pin;
            driver->data_pin            = data_pin;
            driver->reset_pin           = reset_pin;
            driver->spi_divisor         = spi_divisor;
#ifdef BACKLIGHT_ENABLE
            driver->uses_backlight = uses_backlight;
#endif
            return (painter_device_t)driver;
        }
    }
    return NULL;
}
