// Copyright 2021 Paul Cotter (@gr1mr3aver)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <gpio.h>
#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter GC9A01 configurables (add to your keyboard's config.h)

// The number of GC9A01 devices we're going to be talking to. If you have more than one display you need to increase it.
#ifndef GC9A01_NUM_DEVICES
#    define GC9A01_NUM_DEVICES 1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter GC9A01 device factories

#ifdef QUANTUM_PAINTER_GC9A01_SPI_ENABLE
// Factory method for an GC9A01 SPI device
painter_device_t qp_gc9a01_make_spi_device(uint16_t screen_width, uint16_t screen_height, pin_t chip_select_pin, pin_t dc_pin, pin_t reset_pin, uint16_t spi_divisor, int spi_mode);
#endif  // QUANTUM_PAINTER_GC9A01_SPI_ENABLE
