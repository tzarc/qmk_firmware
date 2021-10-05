// Copyright 2021 Paul Cotter (@gr1mr3aver)
// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <gpio.h>
#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ST7789 configurables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The number of ST7789 devices we're going to be talking to
#ifndef ST7789_NUM_DEVICES
#    define ST7789_NUM_DEVICES 1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ST7789 device factory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Factory method for an ST7789 device
#ifdef QUANTUM_PAINTER_ST7789_SPI_ENABLE
painter_device_t qp_st7789_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode);
#endif  // QUANTUM_PAINTER_ST7789_SPI_ENABLE
