/* Copyright 2021 Nick Brassel (@tzarc)
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

#include <stdbool.h>
#include <stdlib.h>
#include <qp.h>
#include <qp_internal.h>

bool     qp_comms_init(painter_device_t device);
bool     qp_comms_start(painter_device_t device);
void     qp_comms_stop(painter_device_t device);
uint32_t qp_comms_send(painter_device_t device, const void *data, uint32_t byte_count);
