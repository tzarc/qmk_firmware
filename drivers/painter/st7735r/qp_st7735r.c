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

#include "color.h"
#include "spi_master.h"

#ifdef BACKLIGHT_ENABLE
#    include "backlight/backlight.h"
#endif

#include <qp.h>
#include <qp_internal.h>
#include <qp_utils.h>
#include <qp_fallback.h>
#include "qp_st7735r.h"
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
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_CMD_RESET);
    wait_ms(100);
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_CMD_SLEEP_OFF);
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

    // Set the default viewport to be fullscreen
    qp_st7735r_internal_lcd_viewport(lcd, 0, 0, 239, 319);

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
        if (!driver->allocated) {
            driver->allocated           = true;
            driver->qp_driver.init      = qp_st7735r_init;
            driver->qp_driver.clear     = qp_st7735r_clear;
            driver->qp_driver.power     = qp_st7735r_power;
            driver->qp_driver.pixdata   = qp_st7735r_pixdata;
            driver->qp_driver.viewport  = qp_st7735r_viewport;
            driver->qp_driver.setpixel  = qp_st7735r_setpixel;
            driver->qp_driver.line      = qp_fallback_line;
            driver->qp_driver.rect      = qp_fallback_rect;
            driver->qp_driver.circle    = qp_fallback_circle;
            driver->qp_driver.ellipse   = qp_fallback_ellipse;
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

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level LCD control functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Enable SPI comms
void qp_st7735r_internal_lcd_start(st7735r_painter_device_t *lcd) {
    spi_start(lcd->chip_select_pin, false, 0, lcd->spi_divisor);
}

// Disable SPI comms
void qp_st7735r_internal_lcd_stop(void) {
    spi_stop();
}

// Send a command
void qp_st7735r_internal_lcd_cmd(st7735r_painter_device_t *lcd, uint8_t b) {
    writePinLow(lcd->data_pin);
    spi_write(b);
}

// Send data
void qp_st7735r_internal_lcd_sendbuf(st7735r_painter_device_t *lcd, const void *data, uint16_t len) {
    writePinHigh(lcd->data_pin);

    uint32_t       bytes_remaining = len;
    const uint8_t *p               = (const uint8_t *)data;
    while (bytes_remaining > 0) {
        uint32_t bytes_this_loop = bytes_remaining < 1024 ? bytes_remaining : 1024;
        spi_transmit(p, bytes_this_loop);
        p += bytes_this_loop;
        bytes_remaining -= bytes_this_loop;
    }
}

// Send data (single byte)
void qp_st7735r_internal_lcd_data(st7735r_painter_device_t *lcd, uint8_t b) { qp_st7735r_internal_lcd_sendbuf(lcd, &b, sizeof(b)); }

// Set a register value
void    qp_st7735r_internal_lcd_reg(st7735r_painter_device_t *lcd, uint8_t reg, uint8_t val) {
    qp_st7735r_internal_lcd_cmd(lcd, reg);
    qp_st7735r_internal_lcd_data(lcd, val);
}

// Set the drawing viewport area
void qp_st7735r_internal_lcd_viewport(st7735r_painter_device_t *lcd, uint16_t xbegin, uint16_t ybegin, uint16_t xend, uint16_t yend) {
    // Set up the x-window
    uint8_t xbuf[4] = {xbegin >> 8, xbegin & 0xFF, xend >> 8, xend & 0xFF};
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_COL_ADDR);  // column address set
    qp_st7735r_internal_lcd_sendbuf(lcd, xbuf, sizeof(xbuf));

    // Set up the y-window
    uint8_t ybuf[4] = {ybegin >> 8, ybegin & 0xFF, yend >> 8, yend & 0xFF};
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_ROW_ADDR);  // page (row) address set
    qp_st7735r_internal_lcd_sendbuf(lcd, ybuf, sizeof(ybuf));

    // Lock in the window
    qp_st7735r_internal_lcd_cmd(lcd, ST7735R_SET_MEM);  // enable memory writes
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//
// NOTE: The variables in this section are intentionally outside a stack frame. They are able to be defined with larger
//       sizes than the normal stack frames would allow, and as such need to be external.
//
//       **** DO NOT refactor this and decide to place the variables inside the function calling them -- you will ****
//       **** very likely get artifacts rendered to the LCD screen as a result.                                   ****
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Static buffer to contain a generated color palette
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
HSV             hsv_lookup_table[256];
static rgb565_t rgb565_palette[256];
#else
HSV             hsv_lookup_table[16];
static rgb565_t rgb565_palette[16];
#endif

// Static buffer used for transmitting image data
static rgb565_t pixdata_transmit_buf[ST7735R_PIXDATA_BUFSIZE];

// Color conversion to LCD-native
static inline rgb565_t rgb_to_st7735r(uint8_t r, uint8_t g, uint8_t b) {
    rgb565_t rgb565 = (((rgb565_t)r) >> 3) << 11 | (((rgb565_t)g) >> 2) << 5 | (((rgb565_t)b) >> 3);
    return BYTE_SWAP(rgb565);
}

// Color conversion to LCD-native
static inline rgb565_t hsv_to_st7735r(uint8_t hue, uint8_t sat, uint8_t val) {
    RGB rgb = hsv_to_rgb_nocie((HSV){hue, sat, val});
    return rgb_to_st7735r(rgb.r, rgb.g, rgb.b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Palette / Monochrome-format image rendering
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Palette renderer
static inline void lcd_send_palette_pixdata_impl(st7735r_painter_device_t *lcd, const rgb565_t *const rgb565_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    const uint8_t  pixel_bitmask       = (1 << bits_per_pixel) - 1;
    const uint8_t  pixels_per_byte     = 8 / bits_per_pixel;
    const uint16_t max_transmit_pixels = ((ST7735R_PIXDATA_BUFSIZE / pixels_per_byte) * pixels_per_byte);  // the number of rgb565 pixels that we can completely fit in the buffer
    const uint8_t *pixdata             = (const uint8_t *)pixel_data;
    uint32_t       remaining_pixels    = pixel_count;  // don't try to derive from byte_count, we may not use an entire byte

    // Transmit each block of pixels
    while (remaining_pixels > 0) {
        uint16_t  transmit_pixels = remaining_pixels < max_transmit_pixels ? remaining_pixels : max_transmit_pixels;
        rgb565_t *target16        = (rgb565_t *)pixdata_transmit_buf;
        for (uint16_t p = 0; p < transmit_pixels; p += pixels_per_byte) {
            uint8_t pixval      = *pixdata;
            uint8_t loop_pixels = remaining_pixels < pixels_per_byte ? remaining_pixels : pixels_per_byte;
            for (uint8_t q = 0; q < loop_pixels; ++q) {
                *target16++ = rgb565_palette[pixval & pixel_bitmask];
                pixval >>= bits_per_pixel;
            }
            ++pixdata;
        }
        qp_st7735r_internal_lcd_sendbuf(lcd, pixdata_transmit_buf, transmit_pixels * sizeof(rgb565_t));
        remaining_pixels -= transmit_pixels;
    }
}

// Recolored renderer
static inline void lcd_send_palette_pixdata(st7735r_painter_device_t *lcd, const uint8_t *const rgb_palette, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count) {
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    for (uint16_t i = 0; i < items; ++i) {
        rgb565_palette[i] = rgb_to_st7735r(rgb_palette[i * 3 + 0], rgb_palette[i * 3 + 1], rgb_palette[i * 3 + 2]);
    }

    // Transmit each block of pixels
    lcd_send_palette_pixdata_impl(lcd, rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

// Recolored renderer
static inline void lcd_send_mono_pixdata_recolor(st7735r_painter_device_t *lcd, uint8_t bits_per_pixel, uint32_t pixel_count, const void *const pixel_data, uint32_t byte_count, int16_t hue_fg, int16_t sat_fg, int16_t val_fg, int16_t hue_bg, int16_t sat_bg, int16_t val_bg) {
    // Generate the color lookup table
    uint16_t items = 1 << bits_per_pixel;  // number of items we need to interpolate
    qp_generate_palette(hsv_lookup_table, items, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    for (uint16_t i = 0; i < items; ++i) {
        rgb565_palette[i] = hsv_to_st7735r(hsv_lookup_table[i].h, hsv_lookup_table[i].s, hsv_lookup_table[i].v);
    }

    // Transmit each block of pixels
    lcd_send_palette_pixdata_impl(lcd, rgb565_palette, bits_per_pixel, pixel_count, pixel_data, byte_count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter API implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Screen clear
bool qp_st7735r_clear(painter_device_t device) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;

    // Re-init the LCD
    lcd->qp_driver.init(device, lcd->rotation);

    return true;
}

// Power control -- on/off (will also handle backlight if set to use the normal QMK backlight driver)
bool qp_st7735r_power(painter_device_t device, bool power_on) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    qp_st7735r_internal_lcd_start(lcd);

    // Turn on/off the display
    qp_st7735r_internal_lcd_cmd(lcd, power_on ? ST7735R_CMD_DISPLAY_ON : ST7735R_CMD_DISPLAY_OFF);

    qp_st7735r_internal_lcd_stop();

#ifdef BACKLIGHT_ENABLE
    // If we're using the backlight to control the display as well, toggle that too.
    if (lcd->uses_backlight) {
        if (power_on) {
            // There's a small amount of time for the LCD to get the display back on the screen -- it's all white beforehand.
            // Delay for a small amount of time and let the LCD catch up before turning the backlight on.
            wait_ms(20);
            backlight_set(get_backlight_level());
        } else {
            backlight_set(0);
        }
    }
#endif

    return true;
}

// Viewport to draw to
bool qp_st7735r_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    qp_st7735r_internal_lcd_start(lcd);

    // Configure where we're going to be rendering to
    qp_st7735r_internal_lcd_viewport(lcd, left, top, right, bottom);

    qp_st7735r_internal_lcd_stop();

    return true;
}

// Stream pixel data to the current write position in GRAM
bool qp_st7735r_pixdata(painter_device_t device, const void *pixel_data, uint32_t byte_count) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    qp_st7735r_internal_lcd_start(lcd);

    // Stream data to the LCD
    qp_st7735r_internal_lcd_sendbuf(lcd, pixel_data, byte_count);

    qp_st7735r_internal_lcd_stop();

    return true;
}

// Manually set a single pixel's color
bool qp_st7735r_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    qp_st7735r_internal_lcd_start(lcd);

    // Configure where we're going to be rendering to
    qp_st7735r_internal_lcd_viewport(lcd, x, y, x, y);

    // Convert the color to RGB565 and transmit to the device
    rgb565_t buf = hsv_to_st7735r(hue, sat, val);
    qp_st7735r_internal_lcd_sendbuf(lcd, &buf, sizeof(buf));

    qp_st7735r_internal_lcd_stop();

    return true;
}

// Draw a line
bool qp_st7735r_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    // If we're not doing horizontal or vertical, fallback to the base implementation
    if (x0 != x1 && y0 != y1) {
        return qp_fallback_line(device, x0, y0, x1, y1, hue, sat, val);
    }

    // If we're doing horizontal or vertical, just use the optimized rect draw so we don't need to deal with single pixels or buffers.
    return qp_st7735r_rect(device, x0, y0, x1, y1, hue, sat, val, true);
}

// Draw a rectangle
bool qp_st7735r_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;

    if (filled) {
        // Convert the color to RGB565
        rgb565_t clr = hsv_to_st7735r(hue, sat, val);

        // Build a larger buffer so we can stream to the LCD in larger chunks, for speed
        rgb565_t buf[ST7735R_PIXDATA_BUFSIZE];
        for (uint32_t i = 0; i < ST7735R_PIXDATA_BUFSIZE; ++i) buf[i] = clr;

        qp_st7735r_internal_lcd_start(lcd);

        // Configure where we're going to be rendering to
        qp_st7735r_internal_lcd_viewport(lcd, left, top, right, bottom);

        // Transmit the data to the LCD in chunks
        uint32_t remaining = (right - left + 1) * (bottom - top + 1);
        while (remaining > 0) {
            uint32_t transmit = (remaining < ST7735R_PIXDATA_BUFSIZE ? remaining : ST7735R_PIXDATA_BUFSIZE);
            uint32_t bytes    = transmit * sizeof(rgb565_t);
            qp_st7735r_internal_lcd_sendbuf(lcd, buf, bytes);
            remaining -= transmit;
        }

        qp_st7735r_internal_lcd_stop();
    } else {
        if (!qp_st7735r_rect(device, left, top, right, top, hue, sat, val, true)) {
            return false;
        }
        if (!qp_st7735r_rect(device, left, bottom, right, bottom, hue, sat, val, true)) {
            return false;
        }
        if (!qp_st7735r_rect(device, left, top + 1, left, bottom - 1, hue, sat, val, true)) {
            return false;
        }
        if (!qp_st7735r_rect(device, right, top + 1, right, bottom - 1, hue, sat, val, true)) {
            return false;
        }
    }

    return true;
}

// Draw an image
bool qp_st7735r_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    st7735r_painter_device_t *lcd = (st7735r_painter_device_t *)device;
    qp_st7735r_internal_lcd_start(lcd);

    // Configure where we're going to be rendering to
    qp_st7735r_internal_lcd_viewport(lcd, x, y, x + image->width - 1, y + image->height - 1);

    uint32_t pixel_count = (((uint32_t)image->width) * image->height);
    if (image->compression == IMAGE_UNCOMPRESSED) {
        const painter_raw_image_descriptor_t *raw_image_desc = (const painter_raw_image_descriptor_t *)image;
        // Stream data to the LCD
        if (image->image_format == IMAGE_FORMAT_RAW || image->image_format == IMAGE_FORMAT_RGB565) {
            // The pixel data is in the correct format already -- send it directly to the device
            qp_st7735r_internal_lcd_sendbuf(lcd, raw_image_desc->image_data, pixel_count);
        } else if (image->image_format == IMAGE_FORMAT_GRAYSCALE) {
            // Supplied pixel data is in 4bpp monochrome -- decode it to the equivalent pixel data
            lcd_send_mono_pixdata_recolor(lcd, raw_image_desc->base.image_bpp, pixel_count, raw_image_desc->image_data, raw_image_desc->byte_count, hue, sat, val, hue, sat, 0);
        } else if (image->image_format == IMAGE_FORMAT_PALETTE) {
            // Supplied pixel data is in 1bpp monochrome -- decode it to the equivalent pixel data
            lcd_send_palette_pixdata(lcd, raw_image_desc->image_palette, raw_image_desc->base.image_bpp, pixel_count, raw_image_desc->image_data, raw_image_desc->byte_count);
        }
    }

    qp_st7735r_internal_lcd_stop();

    return true;
}
