/* Copyright 2018-2020 Nick Brassel (@tzarc)
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


// USB Device parameters
#define VENDOR_ID 0x1209
#define PRODUCT_ID 0x4919
#define MANUFACTURER Gr1mR3aver
#define PRODUCT Wing70
#define DESCRIPTION Split 70-ish percent keyboard

// 1000Hz poll rate
#define USB_POLLING_INTERVAL_MS 1

// Matrix
#ifdef SPLIT_KEYBOARD
#    define MATRIX_ROWS 12
#else
#    define MATRIX_ROWS 6
#endif
#define MATRIX_COLS 6


#define DIODE_DIRECTION ROW2COL


#ifndef ENCODER_RESOLUTION
#    define ENCODER_RESOLUTION 2
#endif  // ENCODER_RESOLUTION

// Debugging
#define DEBUG_MATRIX_SCAN_RATE
#define DEBUG_EEPROM_OUTPUT


// EEPROM Settings
#define EXTERNAL_EEPROM_SPI_SLAVE_SELECT_PIN B2
#define EXTERNAL_EEPROM_BYTE_COUNT 32768
#define EXTERNAL_EEPROM_PAGE_SIZE 1
#define EXTERNAL_EEPROM_SPI_CLOCK_DIVISOR 4

// Bootloader
#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 5
#define BOOTMAGIC_LITE_ROW_RIGHT 6
#define BOOTMAGIC_LITE_COLUMN_RIGHT 5



// Split shared state types
#define SPLIT_SYNC_TYPE_KB kb_runtime_config\
#define SPLIT_SYNC_TYPE_USER user_runtime_config

// ADC Configuration
// #define ADC_COUNT 5
// #define ADC_IGNORE_OVERSAMPLING
// #define ADC_SAMPLING_RATE ADC_SMPR_SMP_2P5
// #define ADC_RESOLUTION ADC_CFGR_RES_10BITS

/* disable these deprecated features by default */
#define NO_ACTION_MACRO
#define NO_ACTION_FUNCTION
