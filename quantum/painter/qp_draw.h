// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <qp_internal.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Global variable used for native pixel data streaming.
extern uint8_t qp_global_pixdata_buffer[QP_PIXDATA_BUFFER_SIZE];

// Returns the number of pixels that can fit in the pixdata buffer
uint32_t qp_num_pixels_in_buffer(painter_device_t device);

// Fills the supplied buffer with equivalent native pixels matching the supplied HSV
void qp_fill_pixdata(painter_device_t device, uint32_t num_pixels, uint8_t hue, uint8_t sat, uint8_t val);

// qp_setpixel internal implementation, but uses the global pixdata buffer with pre-converted native pixel. Only the first pixel is used.
bool qp_setpixel_impl(painter_device_t device, uint16_t x, uint16_t y);

// qp_rect internal implementation, but uses the global pixdata buffer with pre-converted native pixels.
bool qp_fillrect_helper_impl(painter_device_t device, uint16_t l, uint16_t t, uint16_t r, uint16_t b);

// Convert from input pixel data + palette to equivalent pixels
typedef int16_t (*byte_input_callback)(void* cb_arg);
typedef bool (*pixel_output_callback)(qp_pixel_color_t* palette, uint8_t index, void* cb_arg);
bool qp_decode_palette(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, qp_pixel_color_t* palette, pixel_output_callback output_callback, void* output_arg);
bool qp_decode_grayscale(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, pixel_output_callback output_callback, void* output_arg);
bool qp_decode_recolor(painter_device_t device, uint32_t pixel_count, uint8_t bits_per_pixel, byte_input_callback input_callback, void* input_arg, qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, pixel_output_callback output_callback, void* output_arg);

// Global variable used for interpolated pixel lookup table.
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
extern qp_pixel_color_t qp_global_pixel_lookup_table[256];
#else
extern qp_pixel_color_t qp_global_pixel_lookup_table[16];
#endif

// Generates a color-interpolated lookup table based off the number of items, from foreground to background, for use with monochrome image rendering.
// Returns true if a palette was created, false if the palette is reused.
// As this uses a global, this may present a problem if using the same parameters but a different screen converts pixels -- use qp_invalidate_palette() below to reset.
bool qp_interpolate_palette(qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, int16_t steps);

// Resets the global palette so that it can be regenerated. Only needed if the colors are identical, but a different display is used with a different internal pixel format.
void qp_invalidate_palette(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter codec functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum rle_mode_t {
    MARKER_BYTE,
    REPEATING_RUN,
    NON_REPEATING_RUN,
};

struct byte_input_state {
    painter_device_t device;
    const uint8_t QP_RESIDENT_FLASH_OR_RAM* src_data;

    union {
        // RLE-specific
        struct {
            enum rle_mode_t mode;
            uint8_t         remain;  // number of bytes remaining in the current mode
        } rle;
    };
};

struct pixel_output_state {
    painter_device_t device;
    uint32_t         pixel_write_pos;
    uint32_t         max_pixels;
};

int16_t qp_drawimage_byte_uncompressed_decoder(void* cb_arg);
int16_t qp_drawimage_byte_rle_decoder(void* cb_arg);
bool    qp_drawimage_pixel_appender(qp_pixel_color_t* palette, uint8_t index, void* cb_arg);
