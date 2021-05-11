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

#include "config_common.h"

#define DEBUG_EEPROM_OUTPUT

// USB Device parameters
#define VENDOR_ID 0x1209
#define PRODUCT_ID 0x4919
#define DEVICE_VER 0x0001
#define MANUFACTURER Tzarc
#define PRODUCT Djinn

// Matrix
#define MATRIX_ROWS 12
#define MATRIX_COLS 7
#define MATRIX_ROW_PINS \
    { B13, B14, B15, C6, C7, C8 }
#define MATRIX_COL_PINS \
    { C0, C1, C2, C3, A0, A1, A2 }
#define DIODE_DIRECTION ROW2COL

// Encoders
#define ENCODERS_PAD_A \
    { C14 }
#define ENCODERS_PAD_B \
    { C15 }

#ifndef ENCODER_RESOLUTION
#    define ENCODER_RESOLUTION 2
#endif  // ENCODER_RESOLUTION

// Bootloader
#define STM32_BOOTLOADER_DUAL_BANK TRUE
#define STM32_BOOTLOADER_DUAL_BANK_GPIO B7

// Peripheral power control pins
#define LCD_POWER_ENABLE_PIN A6

// Split configuration
#define SPLIT_PLUG_DETECT_PIN B12
#define SPLIT_TRANSACTION_IDS_KB RPC_ID_SYNC_STATE_KB
#define SPLIT_TRANSPORT_MIRROR
#define SPLIT_LAYER_STATE_ENABLE
#define SPLIT_LED_STATE_ENABLE
#define SPLIT_MODS_ENABLE
#define SPLIT_WPM_ENABLE

// SPI Configuration
#define SPI_DRIVER SPID3
#define SPI_SCK_PIN C10
#define SPI_SCK_PAL_MODE 6
#define SPI_MOSI_PIN C12
#define SPI_MOSI_PAL_MODE 6
#define SPI_MISO_PIN C11
#define SPI_MISO_PAL_MODE 6

// LCD Configuration
#define ILI9341_PIXDATA_BUFSIZE 240
#define LCD_RST_PIN B3
#define LCD_CS_PIN D2
#define LCD_DC_PIN A15
#ifndef LCD_ACTIVITY_TIMEOUT
#    define LCD_ACTIVITY_TIMEOUT 30000
#endif  // LCD_ACTIVITY_TIMEOUT

// Backlight driver (to control LCD backlight)
#define BACKLIGHT_LEVELS 4
#define BACKLIGHT_PIN A7
#define BACKLIGHT_PWM_DRIVER PWMD17
#define BACKLIGHT_PWM_CHANNEL 1
#define BACKLIGHT_PAL_MODE 1

// RGB configuration
#define WS2812_EXTERNAL_PULLUP
#define RGB_DI_PIN B2
#ifdef RGB_MATRIX_ENABLE
#    define DRIVER_LED_TOTAL RGBLED_NUM
#    define RGB_MATRIX_SPLIT RGBLED_SPLIT
#endif  //  RGB_MATRIX_ENABLE
#define WS2812_PWM_DRIVER PWMD20
#define WS2812_PWM_CHANNEL 1
#define WS2812_PWM_PAL_MODE 3
#define WS2812_DMA_STREAM STM32_DMA1_STREAM1
#define WS2812_DMA_CHANNEL 1
#define WS2812_DMAMUX_ID STM32_DMAMUX1_TIM20_UP

// Audio configuration
#define AUDIO_PIN A5
#define AUDIO_PIN_ALT A4
#define AUDIO_PIN_ALT_AS_NEGATIVE
#define A5_AUDIO
#ifndef STARTUP_SONG
#    define STARTUP_SONG SONG(STARTUP_SOUND)
#endif  // STARTUP_SONG

/* disable these deprecated features by default */
#define NO_ACTION_MACRO
#define NO_ACTION_FUNCTION
