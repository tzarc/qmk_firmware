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

#include <quantum.h>
#include <qp.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define QP_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define QP_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Cater for AVR address space
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define QP_PACKED __attribute__((packed))

#ifdef __FLASH
#    define QP_RESIDENT_FLASH __flash
#else
#    define QP_RESIDENT_FLASH
#endif

#ifdef __MEMX
#    define QP_RESIDENT_FLASH_OR_RAM __memx
#else
#    define QP_RESIDENT_FLASH_OR_RAM
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specific internal definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <qp_internal_formats.h>
#include <qp_internal_driver.h>
