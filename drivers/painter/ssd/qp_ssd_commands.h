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
#pragma once

/*
 * Quantum Painter SSD OLED command opcodes
 */
#define SSD_SET_COLUMN_ADDR_LOWER(addr) ((addr) & 0xf)            /* Set Lower Column Address */
#define SSD_SET_COLUMN_ADDR_UPPER(addr) (0x10 | ((addr) & 0xf))   /* Set Higher Column Address */
#define SSD_SET_PUMP_VOLT(volt) (0x30 | ((volt) & 0x3))           /* Set Pump voltage Value */
#define SSD_SET_DISPLAY_START_LINE(addr) (0x40 | ((addr) & 0x3f)) /* Set Display Start Line */
#define SSD_CMD_CONTRAST 0x81                                     /* Set Constrast Control Mode Set, next byte: 0-255 */
#define SSD_SET_SEGMENT_REMAP 0xa0                                /* Right Rotates Column */
#define SSD_SET_SEGMENT_REMAP_REV 0xa1                            /* Reverse(Lett) Rotates Column */
#define SSD_SET_DISPLAY_ON 0xa4                                   /* Normal Display */
#define SSD_SET_DISPLAY_FORCE_ON 0xa5                             /* Force entire display On */
#define SSD_SET_NORMAL_DISPLAY 0xa6                               /* Normal Display ON/OFF status output */
#define SSD_SET_REVERSE_DISPLAY 0xa7                              /* Reverse Display ON/OFF status output */
#define SSD_CMD_MULTIPLEX_RATION 0xa8                             /* Switch multiplex mode, next byte 1-64 */
#define SSD_CMD_DCDC_CONTROL 0xad                                 /* Set DC-DC voltage Mode */
#define SSD_DAT_DCDC_DISABLE 0x8a                                 /* External Vpp used */
#define SSD_DAT_DCDC_ENABLE 0x8b                                  /* Built-in DC_DC used */
#define SSD_SET_POWER_OFF 0xae                                    /* Turns off OLED panel */
#define SSD_SET_POWER_ON 0xaf                                     /* Turns on OLED panel */
#define SSD_SET_PAGE_ADDR(addr) (0xb0 | ((addr) & 0xf))           /* Set Display Start Line 0-7 */
#define SSD_SET_SCAN_DIR_NORMAL 0xc0                              /* Output Scan Direction Normal */
#define SSD_SET_SCAN_DIR_FLIP 0xc8                                /* Output Scan Direction Flip vertically */
#define SSD_CMD_DISPLAY_OFFSET 0xd3                               /* Moves the start line next byte 0-63 */
#define SSD_CMD_DIVIDE_FREQUENCY 0xd5                             /* Sets the internal display clock next byte: see docs */
#define SSD_CMD_DIS_PRE_CHARGE_PERIOD 0xd9                        /* Sets dis/precharge, next byte: periods in display clks */
#define SSD_CMD_COMMON_HWD_CONF 0xda                              /* Set SEQ or ALT to match OLED panel hardware layout */
#define SSD_DAT_COMMON_HWD_CONF_SEQ 0x02                          /* Set Sequential Configuration */
#define SSD_DAT_COMMON_HWD_CONF_ALT 0x12                          /* Set Alternative Configuration */
#define SSD_CMD_VCOM_DESELECT_LEVEL 0xdb                          /* Set VCOM Deselect Level Mode, next byte (0-255) Vcom = 0.43 + (next byte * 0.006415) */
#define SSD_CMD_NOP 0xe3                                          /* NOP */
