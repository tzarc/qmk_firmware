/* Copyright 2021 Nick Brassel (@tzarc)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#pragma once

// General config

#define QUANTUM_PAINTER_DEBUG
#define DEBUG_EEPROM_OUTPUT
#define EEPROM_I2C_24LC128

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

// Display-specific config

#define DISPLAY_DC_PIN B0
#define DISPLAY_RST_PIN B1

// 128x64 (modded address)
#define I2C_ADDRESS_0_96_INCH_128_64_SSD1306 0x3C
// 128x32
#define I2C_ADDRESS_0_91_INCH_128_32_SSD1306 0x3C

// 128x128
#define DISPLAY_CS_PIN_1_5_INCH_RGB_OLED_SSD1351 B6
// 128x128
#define DISPLAY_CS_PIN_1_44_INCH_LCD_ST7735S B5
// 80x160
#define DISPLAY_CS_PIN_0_96_INCH_LCD_ST7735 B4
// 240x240 (there's no CS pin on this panel, so pick an unused pin)
#define DISPLAY_CS_PIN_1_3_INCH_LCD_ST7789 A8
// 128x128
#define DISPLAY_CS_PIN_1_44_INCH_LCD_ILI9163 B3
