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

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9486 additional command opcodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ILI9486_GET_ERROR_COUNT 0x05             // Get count of errors on the DSI
#define ILI9486_CMD_INVERT_OFF 0x20              // Exit inverted mode
#define ILI9486_CMD_INVERT_ON 0x21               // Enter inverted mode
#define ILI9486_CMD_READ_FIRST_CHECKSUM 0xAA
#define ILI9486_CMD_READ_SECOND_CHECKSUM 0xAF
#define ILI9486_SET_POWER_CTL_NORMAL 0xC2       // Set power ctl normal mode
#define ILI9486_SET_POWER_CTL_IDLE 0xC3         // Set power ctl idle mode
#define ILI9486_SET_POWER_CTL_PARTIAL 0xC4      // Set power ctl partial mode
#define ILI9486_SET_CABC_CTL_1 0xC6             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_2 0xC8             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_3 0xC9             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_4 0xCA             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_5 0xCB             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_6 0xCC             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_7 0xCD             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_8 0xCE             // Set Content Adaptive Brightness
#define ILI9486_SET_CABC_CTL_9 0xCF             // Set Content Adaptive Brightness
#define ILI9486_SPI_READ_COMMAND 0xFB           // SPI Read Command Setting
