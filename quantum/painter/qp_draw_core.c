// Copyright 2021 Paul Cotter (@gr1mr3aver)
// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_draw.h>

_Static_assert((QP_PIXDATA_BUFFER_SIZE > 0) && (QP_PIXDATA_BUFFER_SIZE % 16) == 0, "QP_PIXDATA_BUFFER_SIZE needs to be a non-zero multiple of 16");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
//
// NOTE: The variables in this section are intentionally outside a stack frame. They are able to be defined with larger
//       sizes than the normal stack frames would allow, and as such need to be external.
//
//       **** DO NOT refactor this and decide to place the variables inside the function calling them -- you will ****
//       **** very likely get artifacts rendered to the screen as a result.                                       ****
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Buffer used for transmitting native pixel data to the downstream device.
uint8_t qp_global_pixdata_buffer[QP_PIXDATA_BUFFER_SIZE];

// Static buffer to contain a generated color palette
static bool             generated_palette = false;
static int16_t          generated_steps   = -1;
static qp_pixel_color_t interpolated_fg_hsv888;
static qp_pixel_color_t interpolated_bg_hsv888;
#if QUANTUM_PAINTER_SUPPORTS_256_PALETTE
qp_pixel_color_t qp_global_pixel_lookup_table[256];
#else
qp_pixel_color_t qp_global_pixel_lookup_table[16];
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t qp_num_pixels_in_buffer(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return ((QP_PIXDATA_BUFFER_SIZE * 8) / driver->native_bits_per_pixel);
}

// qp_setpixel internal implementation, but accepts a buffer with pre-converted native pixel. Only the first pixel is used.
bool qp_setpixel_impl(painter_device_t device, uint16_t x, uint16_t y) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->driver_vtable->viewport(device, x, y, x, y) && driver->driver_vtable->pixdata(device, qp_global_pixdata_buffer, 1);
}

// Fills the global native pixel buffer with equivalent pixels matching the supplied HSV
void qp_fill_pixdata(painter_device_t device, uint32_t num_pixels, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver            = (struct painter_driver_t *)device;
    uint32_t                 pixels_in_pixdata = qp_num_pixels_in_buffer(device);
    num_pixels                                 = QP_MIN(pixels_in_pixdata, num_pixels);

    // Convert the color to native pixel format
    qp_pixel_color_t color = {.hsv888 = {.h = hue, .s = sat, .v = val}};
    driver->driver_vtable->palette_convert(device, 1, &color);

    // Append the required number of pixels
    uint8_t palette_idx = 0;
    for (uint32_t i = 0; i < num_pixels; ++i) {
        driver->driver_vtable->append_pixels(device, qp_global_pixdata_buffer, &color, i, 1, &palette_idx);
    }
}

// Interpolates between two colors to generate a palette
bool qp_interpolate_palette(qp_pixel_color_t fg_hsv888, qp_pixel_color_t bg_hsv888, int16_t steps) {
    // Check if we need to generate a new palette -- if the input parameters match then assume the palette can stay unchanged.
    // This may present a problem if using the same parameters but a different screen converts pixels -- use qp_invalidate_palette() to reset.
    if (generated_palette == true && generated_steps == steps && memcmp(&interpolated_fg_hsv888, &fg_hsv888, sizeof(fg_hsv888)) == 0 && memcmp(&interpolated_bg_hsv888, &bg_hsv888, sizeof(bg_hsv888)) == 0) {
        // We already have the correct palette, no point regenerating it.
        return false;
    }

    // Save the parameters so we know whether we can skip generation
    generated_palette      = true;
    generated_steps        = steps;
    interpolated_fg_hsv888 = fg_hsv888;
    interpolated_bg_hsv888 = bg_hsv888;

    int16_t hue_fg = fg_hsv888.hsv888.h;
    int16_t hue_bg = bg_hsv888.hsv888.h;

    // Make sure we take the "shortest" route from one hue to the other
    if ((hue_fg - hue_bg) >= 128) {
        hue_bg += 256;
    } else if ((hue_fg - hue_bg) <= -128) {
        hue_bg -= 256;
    }

    // Interpolate each of the lookup table entries
    for (int16_t i = 0; i < steps; ++i) {
        qp_global_pixel_lookup_table[i].hsv888.h = (uint8_t)((hue_fg - hue_bg) * i / (steps - 1) + hue_bg);
        qp_global_pixel_lookup_table[i].hsv888.s = (uint8_t)((fg_hsv888.hsv888.s - bg_hsv888.hsv888.s) * i / (steps - 1) + bg_hsv888.hsv888.s);
        qp_global_pixel_lookup_table[i].hsv888.v = (uint8_t)((fg_hsv888.hsv888.v - bg_hsv888.hsv888.v) * i / (steps - 1) + bg_hsv888.hsv888.v);

        qp_dprintf("qp_interpolate_palette: %3d of %d -- H: %3d, S: %3d, V: %3d\n", (int)(i + 1), (int)steps, (int)qp_global_pixel_lookup_table[i].hsv888.h, (int)qp_global_pixel_lookup_table[i].hsv888.s, (int)qp_global_pixel_lookup_table[i].hsv888.v);
    }

    return true;
}

// Resets the global palette so that it can be regenerated. Only needed if the colors are identical, but a different display is used with a different internal pixel format.
void qp_invalidate_palette(void) {
    generated_palette = false;
    generated_steps   = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_setpixel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_setpixel: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("Failed to start comms in qp_setpixel\n");
        return false;
    }

    qp_fill_pixdata(device, 1, hue, sat, val);
    bool ret = qp_setpixel_impl(device, x, y);
    qp_comms_stop(device);
    qp_dprintf("qp_setpixel: %s\n", ret ? "ok" : "fail");
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_line
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    if (x0 == x1 || y0 == y1) {
        qp_dprintf("qp_line(%d, %d, %d, %d): entry (deferring to qp_rect)\n", (int)x0, (int)y0, (int)x1, (int)y1);
        bool ret = qp_rect(device, x0, y0, x1, y1, hue, sat, val, true);
        qp_dprintf("qp_line(%d, %d, %d, %d): %s (deferred to qp_rect)\n", (int)x0, (int)y0, (int)x1, (int)y1, ret ? "ok" : "fail");
        return ret;
    }

    qp_dprintf("qp_line(%d, %d, %d, %d): entry\n", (int)x0, (int)y0, (int)x1, (int)y1);
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_line: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("Failed to start comms in qp_line\n");
        return false;
    }

    qp_fill_pixdata(device, 1, hue, sat, val);

    // draw angled line using Bresenham's algo
    int16_t x      = ((int16_t)x0);
    int16_t y      = ((int16_t)y0);
    int16_t slopex = ((int16_t)x0) < ((int16_t)x1) ? 1 : -1;
    int16_t slopey = ((int16_t)y0) < ((int16_t)y1) ? 1 : -1;
    int16_t dx     = abs(((int16_t)x1) - ((int16_t)x0));
    int16_t dy     = -abs(((int16_t)y1) - ((int16_t)y0));

    int16_t e  = dx + dy;
    int16_t e2 = 2 * e;

    bool ret = true;
    while (x != x1 || y != y1) {
        if (!qp_setpixel_impl(device, x, y)) {
            ret = false;
            break;
        }
        e2 = 2 * e;
        if (e2 >= dy) {
            e += dy;
            x += slopex;
        }
        if (e2 <= dx) {
            e += dx;
            y += slopey;
        }
    }
    // draw the last pixel
    if (!qp_setpixel_impl(device, x, y)) {
        ret = false;
    }

    qp_comms_stop(device);
    qp_dprintf("qp_line(%d, %d, %d, %d): %s\n", (int)x0, (int)y0, (int)x1, (int)y1, ret ? "ok" : "fail");
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantum Painter External API: qp_rect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_fillrect_helper_impl(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    uint32_t                 pixels_in_pixdata = qp_num_pixels_in_buffer(device);
    struct painter_driver_t *driver            = (struct painter_driver_t *)device;

    uint16_t l = QP_MIN(left, right);
    uint16_t r = QP_MAX(left, right);
    uint16_t t = QP_MIN(top, bottom);
    uint16_t b = QP_MAX(top, bottom);
    uint16_t w = r - l + 1;
    uint16_t h = b - t + 1;

    uint32_t remaining = w * h;
    driver->driver_vtable->viewport(device, l, t, r, b);
    while (remaining > 0) {
        uint32_t transmit = QP_MIN(remaining, pixels_in_pixdata);
        if (!driver->driver_vtable->pixdata(device, qp_global_pixdata_buffer, transmit)) {
            return false;
        }
        remaining -= transmit;
    }
    return true;
}

bool qp_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    qp_dprintf("qp_rect(%d, %d, %d, %d): entry\n", (int)left, (int)top, (int)right, (int)bottom);
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_rect: fail (validation_ok == false)\n");
        return false;
    }

    // Cater for cases where people have submitted the coordinates backwards
    uint16_t l = QP_MIN(left, right);
    uint16_t r = QP_MAX(left, right);
    uint16_t t = QP_MIN(top, bottom);
    uint16_t b = QP_MAX(top, bottom);
    uint16_t w = r - l + 1;
    uint16_t h = b - t + 1;

    bool ret = true;
    if (!qp_comms_start(device)) {
        qp_dprintf("Failed to start comms in qp_rect\n");
        return false;
    }

    if (filled) {
        // Fill up the pixdata buffer with the required number of native pixels
        qp_fill_pixdata(device, w * h, hue, sat, val);

        // Perform the draw
        ret = qp_fillrect_helper_impl(device, l, t, r, b);
    } else {
        // Fill up the pixdata buffer with the required number of native pixels
        qp_fill_pixdata(device, QP_MAX(w, h), hue, sat, val);

        // Draw 4x filled single-width rects to create an outline
        if (!qp_fillrect_helper_impl(device, l, t, r, t) || !qp_fillrect_helper_impl(device, l, b, r, b) || !qp_fillrect_helper_impl(device, l, t + 1, l, b - 1) || !qp_fillrect_helper_impl(device, r, t + 1, r, b - 1)) {
            ret = false;
        }
    }

    qp_comms_stop(device);
    qp_dprintf("qp_rect(%d, %d, %d, %d): %s\n", (int)l, (int)t, (int)r, (int)b, ret ? "ok" : "fail");
    return ret;
}
