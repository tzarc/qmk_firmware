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

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ST7735R command opcodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// System function commands

#define ST7735R_CMD_NOP 0x00                 // No operation
#define ST7735R_CMD_RESET 0x01               // Software reset
#define ST7735R_GET_ID_INFO 0x04             // Get ID information
#define ST7735R_GET_STATUS 0x09              // Get status
#define ST7735R_GET_PWR_MODE 0x0A            // Get power mode
#define ST7735R_GET_MADCTL 0x0B              // Get MADCTL
#define ST7735R_GET_PIX_FMT 0x0C             // Get pixel format
#define ST7735R_GET_IMG_FMT 0x0D             // Get image format
#define ST7735R_GET_SIG_MODE 0x0E            // Get signal mode
#define ST7735R_CMD_SLEEP_ON 0x10            // Enter sleep mode
#define ST7735R_CMD_SLEEP_OFF 0x11           // Exist sleep mode
#define ST7735R_CMD_PARTIAL_ON 0x12          // Enter partial mode
#define ST7735R_CMD_NORMAL_ON 0x13         // Exit partial mode
#define ST7735R_CMD_INVERT_OFF 0x20          // Exit inverted mode
#define ST7735R_CMD_INVERT_ON 0x21           // Enter inverted mode
#define ST7735R_SET_GAMMA 0x26               // Set gamma params
#define ST7735R_CMD_DISPLAY_OFF 0x28         // Disable display
#define ST7735R_CMD_DISPLAY_ON 0x29          // Enable display
#define ST7735R_SET_COL_ADDR 0x2A            // Set column address
#define ST7735R_SET_ROW_ADDR 0x2B           // Set page address
#define ST7735R_SET_MEM 0x2C                 // Set memory
#define ST7735R_GET_MEM 0x2E                 // Get memory
#define ST7735R_SET_PARTIAL_AREA 0x30        // Set partial area
#define ST7735R_CMD_TEARING_ON 0x34          // Tearing line enabled
#define ST7735R_CMD_TEARING_OFF 0x35         // Tearing line disabled
#define ST7735R_SET_MEM_ACS_CTL 0x36         // Set mem access ctl
#define ST7735R_CMD_IDLE_OFF 0x38            // Exit idle mode
#define ST7735R_CMD_IDLE_ON 0x39             // Enter idle mode
#define ST7735R_SET_PIX_FMT 0x3A             // Set pixel format
#define ST7735R_GET_ID1 0xDA                 // Get ID1
#define ST7735R_GET_ID2 0xDB                 // Get ID2
#define ST7735R_GET_ID3 0xDC                 // Get ID3


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Panel Function Commands

#define ST7735R_SET_FRAME_CTL_NORMAL 0xB1    // Set frame ctl (normal)
#define ST7735R_SET_FRAME_CTL_IDLE 0xB2      // Set frame ctl (idle)
#define ST7735R_SET_FRAME_CTL_PARTIAL 0xB3   // Set frame ctl (partial)
#define ST7735R_SET_INVERSION_CTL 0xB4       // Set inversion ctl
#define ST7735R_SET_FUNCTION_CTL 0xB6        // Set function ctl
#define ST7735R_SET_POWER_CTL_1 0xC0         // Set power ctl 1
#define ST7735R_SET_POWER_CTL_2 0xC1         // Set power ctl 2
#define ST7735R_SET_POWER_CTL_NORMAL 0xC2    // Set power ctl 3
#define ST7735R_SET_POWER_CTL_IDLE 0xC3      // Set power ctl 4
#define ST7735R_SET_POWER_CTL_PARTIAL 0xC4   // Set power ctl 5
#define ST7735R_SET_VCOM_CTL_1 0xC5          // Set VCOM ctl 1
#define ST7735R_SET_VCOM_OFFSET 0xC7         // Set VCOM offset ctl
#define ST7735R_SET_ID2 0xD1                 // Get ID2
#define ST7735R_SET_ID3 0xD2                 // Get ID3
#define ST7735R_SET_POWER_CTL_6 0xFC         // Set power ctl 6
#define ST7735R_CTL_NVMEM 0xD9               // Enable NVMEM access
#define ST7735R_GET_NVMEM 0xDE               // Get NVMEM data
#define ST7735R_SET_NVMEM 0xDF               // Set NVMEM data
#define ST7735R_SET_PGAMMA 0xE0              // Set positive gamma
#define ST7735R_SET_NGAMMA 0xE1              // Set negative gamma
#define ST7735R_SET_EXT_CTL 0xF0               // Set External Control Extension
#define ST7735R_SET_VCOM4_LVL 0xFF               // Set VCOM4 level
