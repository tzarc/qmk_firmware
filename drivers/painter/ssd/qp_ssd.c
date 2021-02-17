/* Copyright 2021 Michael Spradling <mike@mspradling.com>
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
#include <qp.h>
#include <quantum.h>
#include <qp_internal.h>
#include <qp_ssd.h>
#include <qp_ssd_commands.h>
#include <qp_ssd_internal.h>

#ifdef QUANTUM_PAINTER_BUS_I2C
#include <i2c_master.h>
#endif
#ifdef QUANTUM_PAINTER_BUS_SPI
#include <spi_master.h>
#endif

#define QP_SSD_I2C_DAT 0x40
#define QP_SSD_I2C_CMD 0x00
#define QP_SSD_I2C_TIMEOUT 100

/*
 * Low-level LCD control functions
 */

void qp_ssd_internal_lcd_init(ssd_painter_device_t *lcd)
{
    switch (lcd->bus) {
#ifdef QUANTUM_PAINTER_BUS_SPI
	case QP_BUS_SPI_3WIRE:
	    /* Not supported until qmk supports PR #11724 */
	    spi_init();

	    /* Chip Select Setup */
	    setPinOutput(lcd->chip_select_pin);
	    writePinHigh(lcd->chip_select_pin);
	    break;
	case QP_BUS_SPI_4WIRE:
	    spi_init();

	    /* Chip Select Setup */
	    setPinOutput(lcd->chip_select_pin);
	    writePinHigh(lcd->chip_select_pin);

	    /* 4 Wire has a dedicated pin for data/CMD */
	    setPinOutput(lcd->data_pin);
	    writePinLow(lcd->data_pin); /* Command Mode */
	    break;
#endif
#ifdef QUANTUM_PAINTER_BUS_I2C
	case QP_BUS_I2C:
	    i2c_init();
	    break;
#endif
	default:
	    break;
    };
}

/* Enable communication bus */
void qp_ssd_internal_lcd_start(ssd_painter_device_t *lcd)
{
    switch (lcd->bus) {
#ifdef QUANTUM_PAINTER_BUS_SPI
        case QP_BUS_SPI_3WIRE:
	    /* Not supported until qmk supports PR #11724 */
        case QP_BUS_SPI_4WIRE:
            spi_start(lcd->chip_select_pin, SSD_SPI_LSB_FIRST, SSD_SPI_MODE, lcd->spi_divisor);
            break;
#endif
#ifdef QUANTUM_PAINTER_BUS_I2C
        case QP_BUS_I2C:
            i2c_start(lcd->i2c_addr);
            break;
#endif
        default:
            break;
    };
}

/* Disable communication bus */
void qp_ssd_internal_lcd_stop(ssd_painter_device_t *lcd)
{
    switch (lcd->bus) {
#ifdef QUANTUM_PAINTER_BUS_SPI
        case QP_BUS_SPI_3WIRE:
	    /* Not supported until qmk supports PR #11724 */
        case QP_BUS_SPI_4WIRE:
            spi_stop();
            break;
#endif
#ifdef QUANTUM_PAINTER_BUS_I2C
        case QP_BUS_I2C:
            i2c_stop();
            break;
#endif
        default:
            break;
    };
}

void qp_ssd_internal_lcd_cmd_set(ssd_painter_device_t *lcd, uint8_t cmd)
{
    qp_ssd_internal_lcd_cmd(lcd, &cmd, 1);
}

void qp_ssd_internal_lcd_cmd_set_val(ssd_painter_device_t *lcd, uint8_t cmd, uint8_t val)
{
    uint8_t data[2];
    data[0] = cmd;
    data[1] = val;
    qp_ssd_internal_lcd_cmd(lcd, data, 2);
}

/* Send command to lcd */
void qp_ssd_internal_lcd_cmd(ssd_painter_device_t *lcd, const uint8_t *data, uint16_t len)
{
    /* Enable Communication to LCD */
    qp_ssd_internal_lcd_start(lcd);

    switch (lcd->bus) {
#ifdef QUANTUM_PAINTER_BUS_SPI
        case QP_BUS_SPI_3WIRE:
	    /* Not supported until qmk supports PR #11724 */
	    break;
        case QP_BUS_SPI_4WIRE:
            writePinLow(lcd->data_pin);
            spi_transmit(data, len);
            break;
#endif
#ifdef QUANTUM_PAINTER_BUS_I2C
        case QP_BUS_I2C:
	uint8_t header = QP_SSD_I2C_CMD;
	    i2c_transmit(lcd->i2c_addr, &header, 1, QP_SSD_I2C_TIMEOUT);
	    i2c_transmit(lcd->i2c_addr, data, len, QP_SSD_I2C_TIMEOUT);
            break;
#endif
        default:
            break;
    };

    /* Disable Communication to LCD */
    qp_ssd_internal_lcd_stop(lcd);
}

/* send data to lcd */
void qp_ssd_internal_lcd_data(ssd_painter_device_t *lcd, const uint8_t *data, uint16_t len)
{
    /* Enable Communication to LCD */
    qp_ssd_internal_lcd_start(lcd);

    switch (lcd->bus) {
#ifdef QUANTUM_PAINTER_BUS_SPI
        case QP_BUS_SPI_3WIRE:
	    /* Not supported until qmk supports PR #11724 */
	    break;
        case QP_BUS_SPI_4WIRE:
            writePinHigh(lcd->data_pin);
            spi_transmit(data, len);
            break;
#endif
#ifdef QUANTUM_PAINTER_BUS_I2C
        case QP_BUS_I2C:
	    uint8_t header = QP_SSD_I2C_DAT;
	    i2c_transmit(lcd->i2c_addr, &header, 1, QP_SSD_I2C_TIMEOUT);
	    i2c_transmit(lcd->i2c_addr, data, len, QP_SSD_I2C_TIMEOUT);
            break;
#endif
        default:
            break;
    };

    /* Disable Communication to LCD */
    qp_ssd_internal_lcd_stop(lcd);
}

/*
 * Quantum Painter API imlementations
 */

bool qp_ssd_flush(painter_device_t device)
{
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    uint8_t *lcd_ram = (uint8_t*)qp_mono2_surface_get_buffer_ptr(lcd->fb);

    for (uint8_t page = 0; page < SSD_PAGE_COUNT; page++) {
        qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_COLUMN_ADDR_UPPER(0));
        qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_COLUMN_ADDR_LOWER(SSD_COLUMN_OFFSET));
        qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_PAGE_ADDR(page));
        qp_ssd_internal_lcd_data(lcd, &lcd_ram[page*SSD_RESOLUTION_X], SSD_RESOLUTION_X);
    }

    return true;
}

/* Clear screen */
bool qp_ssd_clear(painter_device_t device)
{
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_clear(lcd->fb);
}

bool qp_ssd_power(painter_device_t device, bool power_on)
{
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;

    if (power_on) {
        qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_POWER_ON);
    }else {
        qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_POWER_OFF);
    }

    return true;
}

bool qp_ssd_brightness(painter_device_t device, uint8_t val)
{
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;

    /* Configure Brightness */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_CONTRAST, val);

    return true;
}

bool qp_ssd_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_viewport(lcd->fb, left, top, right, bottom);
}

bool qp_ssd_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_pixdata(lcd->fb, pixel_data, native_pixel_count);
}

bool qp_ssd_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_setpixel(lcd->fb, x, y, hue, sat, val);
}

bool qp_ssd_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_line(lcd->fb, x0, y0, x1, y1,  hue, sat, val);
}

bool qp_ssd_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_rect(lcd->fb, left, top, right, bottom, hue, sat, val, filled);
}

bool qp_ssd_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_circle(lcd->fb, x, y, radius, hue, sat, val, filled);
}

bool qp_ssd_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_ellipse(lcd->fb, x, y, sizex, sizey, hue, sat, val, filled);
}

bool qp_ssd_drawimage(painter_device_t device, uint16_t x, uint16_t y, const painter_image_descriptor_t *image, uint8_t hue, uint8_t sat, uint8_t val) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_drawimage(lcd->fb, x, y, image);
}

bool qp_ssd_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    return qp_drawtext(lcd->fb, x, y, font, str);
}



/*
 * Initialization
 */
bool qp_ssd_init(painter_device_t device, painter_rotation_t rotation)
{
    ssd_painter_device_t *lcd = (ssd_painter_device_t*)device;
    lcd->rotation             = rotation;

    /* Initialize the communication */
    qp_ssd_internal_lcd_init(lcd);

    /* Reset device */
    setPinOutput(lcd->reset_pin);
    writePinLow(lcd->reset_pin);
    wait_ms(20);
    writePinHigh(lcd->reset_pin);
    wait_ms(20);

    /* Turn off LCD, prevent snow */
    qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_POWER_OFF);

    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_DCDC_CONTROL, SSD_DAT_DCDC_ENABLE);

    /* Map Memory to Display (board vendor) */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_COMMON_HWD_CONF, SSD_DAT_COMMON_HWD_CONF_ALT);

    /* Handle Flip rotation */
    if (rotation == QP_ROTATION_180) {
	qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_SEGMENT_REMAP_REV);
	qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_SCAN_DIR_FLIP);
    } else {
	qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_SEGMENT_REMAP);
	qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_SCAN_DIR_NORMAL);
    }

    /* Configure LCD Sizing to controller */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_MULTIPLEX_RATION, SSD_RESOLUTION_X - 1);

    /* Configure timing */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_DIVIDE_FREQUENCY, 0x80); /* Divide Ratio 0, Fosc+15% */

    /* Configure Deselect Vcom voltage */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_VCOM_DESELECT_LEVEL, 0x20);

    /* Configure Brightness */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_CONTRAST, 0x80); /* Default Contrast */

    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_DIS_PRE_CHARGE_PERIOD, 0xf1); /* Discharge = 15 dclks, Precharge = 1dlk */

    /* Initial position */
    qp_ssd_internal_lcd_cmd_set_val(lcd, SSD_CMD_DISPLAY_OFFSET, 0); /* Display start line = 0 */
    qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_DISPLAY_START_LINE(0));

    /* Initialize framebuffer and blank screen */
    if (!qp_init(lcd->fb, rotation)) {
	return false;
    }
    if (!qp_clear(lcd)) {
    }
    if (!qp_flush(device)) {
	return false;
    }

    /* Turn on display */
    qp_ssd_internal_lcd_cmd_set(lcd, SSD_SET_POWER_ON);

    return true;
}

/*
 * Device creation
 */

/* Driver storage */
static ssd_painter_device_t drivers[SSD_NUM_DEVICES] = {0};

/* Factory function for creating a handle to the SSD Driver with SPI Bus */
#ifdef QUANTUM_PAINTER_BUS_SPI
painter_device_t qp_ssd_spi_make_device(pin_t chip_select_pin, pin_t data_pin, pin_t reset_pin, uint16_t spi_divisor)
{
    for (uint32_t i = 0; i < SSD_NUM_DEVICES; i++) {
	ssd_painter_device_t *driver = &drivers[i];

	if (!driver->allocated) {
	    // Allocate framebuffer for device
	    painter_device_t fb = qp_mono2_surface_make_device(SSD_RESOLUTION_X, SSD_RESOLUTION_Y);
	    if (fb) {
		driver->allocated            = true;
		driver->qp_driver.init       = qp_ssd_init;
		driver->qp_driver.power      = qp_ssd_power;
		driver->qp_driver.flush      = qp_ssd_flush;
		driver->qp_driver.brightness = qp_ssd_brightness;
		driver->qp_driver.clear      = qp_ssd_clear;
		driver->qp_driver.pixdata    = qp_ssd_pixdata;
		driver->qp_driver.viewport   = qp_ssd_viewport;
		driver->qp_driver.setpixel   = qp_ssd_setpixel;
		driver->qp_driver.line       = qp_ssd_line;
		driver->qp_driver.rect       = qp_ssd_rect;
		driver->qp_driver.circle     = qp_ssd_circle;
		driver->qp_driver.ellipse    = qp_ssd_ellipse;
		driver->qp_driver.drawimage  = qp_ssd_drawimage;
		driver->bus                  = QP_BUS_SPI_4WIRE;
		driver->chip_select_pin      = chip_select_pin;
		driver->data_pin             = data_pin;
		driver->reset_pin            = reset_pin;
		driver->spi_divisor          = spi_divisor;
		driver->fb                   = fb;
		return (painter_device_t)driver;
	    }
	}
    }
    return NULL;
}
#endif

/* Factory function for creating a handle to the SSD Driver with I2C Bus */
#ifdef QUANTUM_PAINTER_BUS_I2C
painter_device_t qp_ssd_i2c_make_device(pin_t reset_pin, uint8_t i2c_addr)
{
    for (uint32_t i = 0; i < SSD_NUM_DEVICES; i++) {
	ssd_painter_device_t *driver = &drivers[i];

	if (!driver->allocated) {
	    // Allocate framebuffer for device
	    painter_device_t fb = qp_mono2_surface_make_device(SSD_RESOLUTION_X, SSD_RESOLUTION_Y);
	    if (fb) {
		driver->allocated            = true;
		driver->qp_driver.init       = qp_ssd_init;
		driver->qp_driver.power      = qp_ssd_power;
		driver->qp_driver.flush      = qp_ssd_flush;
		driver->qp_driver.brightness = qp_ssd_brightness;
		driver->qp_driver.clear      = qp_ssd_clear;
		driver->qp_driver.pixdata    = qp_ssd_pixdata;
		driver->qp_driver.viewport   = qp_ssd_viewport;
		driver->qp_driver.setpixel   = qp_ssd_setpixel;
		driver->qp_driver.line       = qp_ssd_line;
		driver->qp_driver.rect       = qp_ssd_rect;
		driver->qp_driver.circle     = qp_ssd_circle;
		driver->qp_driver.ellipse    = qp_ssd_ellipse;
		driver->qp_driver.drawimage  = qp_ssd_drawimage;
		driver->bus                  = QP_BUS_I2C;
		driver->i2c_addr             = (i2c_addr << 1);
		driver->reset_pin            = reset_pin;
		driver->fb                   = fb;
		return (painter_device_t)driver;
	    }
	}
    }
    return NULL;
}
#endif
