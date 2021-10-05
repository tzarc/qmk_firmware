// Copyright 2021 Paul Cotter (@gr1mr3aver)
// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <qp.h>
#include <qp_comms_spi.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter ILI9xxx internals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Device definition
typedef struct st77xx_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type

    union {
        struct qp_comms_spi_dc_reset_config_t spi_dc_reset_config;
        // TODO: I2C/parallel etc.
    };
    painter_rotation_t rotation;
    uint16_t           x_offset;
    uint16_t           y_offset;
} st77xx_painter_device_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_st77xx_power(painter_device_t device, bool power_on);
bool qp_st77xx_clear(painter_device_t device);
bool qp_st77xx_flush(painter_device_t device);
bool qp_st77xx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
bool qp_st77xx_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count);
bool qp_st77xx_palette_convert(painter_device_t driver, int16_t palette_size, qp_pixel_color_t *pixels);
bool qp_st77xx_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices);
