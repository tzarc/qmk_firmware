// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <qp.h>
#include <qp_comms_spi.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9xxx internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device definition
typedef struct ili9xxx_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type

    union {
        struct qp_comms_spi_dc_reset_config_t spi_dc_reset_config;
        // TODO: I2C/parallel etc.
    };
    painter_rotation_t rotation;
} ili9xxx_painter_device_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_ili9xxx_power(painter_device_t device, bool power_on);
bool qp_ili9xxx_clear(painter_device_t device);
bool qp_ili9xxx_flush(painter_device_t device);
bool qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
bool qp_ili9xxx_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
bool qp_ili9xxx_palette_convert(painter_device_t driver, int16_t palette_size, qp_pixel_color_t *pixels);
bool qp_ili9xxx_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices);
