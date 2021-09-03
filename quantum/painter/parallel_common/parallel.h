/* Copyright 2020 Paul Cotter (@gr1mr3aver)
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

#include <quantum.h>
#include <hal_pal.h>
#include <stdint.h>

#ifndef PARALLEL_WRITE_DELAY_NS
#   define PARALLEL_WRITE_DELAY_NS 0
#endif

#ifndef PARALLEL_POST_WRITE_DELAY_NS
#   define PARALLEL_POST_WRITE_DELAY_NS PARALLEL_WRITE_DELAY_NS
#endif

#ifndef PARALLEL_READ_DELAY_NS
#   define PARALLEL_READ_DELAY_NS 350
#endif

#ifndef PARALLEL_POST_READ_DELAY_NS
#   define PARALLEL_POST_READ_DELAY_NS PARALLEL_READ_DELAY_NS
#endif

bool parallel_init(void);
bool parallel_start(pin_t write_pin, pin_t read_pin, pin_t chip_select_pin);
bool parallel_write(uint8_t data);
bool parallel_transmit(const uint8_t *data, uint16_t length);
void parallel_stop(void);
