// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// General config
#define EEPROM_I2C_24LC128
//#define QUANTUM_PAINTER_DEBUG

// Matrix config
#define MATRIX_ROW_PINS \
    { C14 }
#define MATRIX_COL_PINS \
    { C15 }
#define DIODE_DIRECTION COL2ROW

// I2C config
#define I2C_DRIVER I2CD1
#define I2C1_SCL_PIN A9
#define I2C1_SCL_PAL_MODE 4
#define I2C1_SDA_PIN A10
#define I2C1_SDA_PAL_MODE 4
#define I2C1_TIMINGR_PRESC 0U
#define I2C1_TIMINGR_SCLDEL 11U
#define I2C1_TIMINGR_SDADEL 0U
#define I2C1_TIMINGR_SCLH 14U
#define I2C1_TIMINGR_SCLL 42U

// SPI config
#define SPI_DRIVER SPID1

#define SPI_SCK_PIN A5
#define SPI_SCK_PAL_MODE 5
#define SPI_MOSI_PIN A7
#define SPI_MOSI_PAL_MODE 5
#define SPI_MISO_PIN A6
#define SPI_MISO_PAL_MODE 5

// Display common config
#define DISPLAY_DC_PIN B0

// 240x240 ST7789
#define DISPLAY_CS_PIN_1_3_INCH_LCD_ST7789 A4
#define DISPLAY_RST_PIN_1_3_INCH_LCD_ST7789 A3
// 128x128 ILI9163
#define DISPLAY_CS_PIN_1_44_INCH_LCD_ILI9163 B7
#define DISPLAY_RST_PIN_1_44_INCH_LCD_ILI9163 B6
