// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <color.h>
#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_draw.h>
#include <qp_ili9xxx_internal.h>
#include <qp_ili9xxx_opcodes.h>

#define BYTE_SWAP(x) (((((uint16_t)(x)) >> 8) & 0x00FF) | ((((uint16_t)(x)) << 8) & 0xFF00))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Native pixel format conversion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Color conversion to LCD-native
static inline rgb565_t rgb_to_ili9xxx(uint8_t r, uint8_t g, uint8_t b) {
    rgb565_t rgb565 = (((rgb565_t)r) >> 3) << 11 | (((rgb565_t)g) >> 2) << 5 | (((rgb565_t)b) >> 3);
    return BYTE_SWAP(rgb565);
}

// Color conversion to LCD-native
static inline rgb565_t hsv_to_ili9xxx(uint8_t hue, uint8_t sat, uint8_t val) {
    RGB rgb = hsv_to_rgb_nocie((HSV){hue, sat, val});
    return rgb_to_ili9xxx(rgb.r, rgb.g, rgb.b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter API implementations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Power control
bool qp_ili9xxx_power(painter_device_t device, bool power_on) {
    qp_comms_command(device, power_on ? ILI9XXX_CMD_DISPLAY_ON : ILI9XXX_CMD_DISPLAY_OFF);
    return true;
}

// Screen clear
bool qp_ili9xxx_clear(painter_device_t device) {
    ili9xxx_painter_device_t *lcd = (ili9xxx_painter_device_t *)device;
    lcd->qp_driver.driver_vtable->init(device, lcd->rotation);  // Re-init the LCD
    return true;
}

// Screen flush
bool qp_ili9xxx_flush(painter_device_t device) {
    // No-op, as there's no framebuffer in RAM for this device.
    (void)device;
    return true;
}

// Viewport to draw to
bool qp_ili9xxx_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    // Set up the x-window
    uint8_t xbuf[4] = {left >> 8, left & 0xFF, right >> 8, right & 0xFF};
    qp_comms_command_databuf(device,
                             ILI9XXX_SET_COL_ADDR,  // column address set
                             xbuf, sizeof(xbuf));

    // Set up the y-window
    uint8_t ybuf[4] = {top >> 8, top & 0xFF, bottom >> 8, bottom & 0xFF};
    qp_comms_command_databuf(device,
                             ILI9XXX_SET_PAGE_ADDR,  // page (row) address set
                             ybuf, sizeof(ybuf));

    // Lock in the window
    qp_comms_command(device, ILI9XXX_SET_MEM);  // enable memory writes
    return true;
}

// Stream pixel data to the current write position in GRAM
bool qp_ili9xxx_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qp_comms_send(device, pixel_data, native_pixel_count * sizeof(uint16_t));
    return true;
}

bool qp_ili9xxx_palette_convert(painter_device_t device, int16_t palette_size, qp_pixel_color_t *palette) {
    for (int16_t i = 0; i < palette_size; ++i) {
        palette[i].rgb565 = hsv_to_ili9xxx(palette[i].hsv888.h, palette[i].hsv888.s, palette[i].hsv888.v);
    }
    return true;
}

bool qp_ili9xxx_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_color_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices) {
    uint16_t *buf = (uint16_t *)target_buffer;
    for (uint32_t i = 0; i < pixel_count; ++i) {
        buf[pixel_offset + i] = palette[palette_indices[i]].rgb565;
    }
    return true;
}
