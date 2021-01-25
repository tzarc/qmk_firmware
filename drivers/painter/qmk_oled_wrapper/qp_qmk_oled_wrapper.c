/* Copyright 2020 Nick Brassel (@tzarc)
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

#include <string.h>

#include "spi_master.h"

#include "qp.h"
#include "qp_qmk_oled_wrapper.h"
#include "qp_internal.h"
#include "qp_fallback.h"
#include "oled_driver.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct qmk_oled_painter_device_t {
    struct painter_driver_t qp_driver;  // must be first, so it can be cast from the painter_device_t* type
    painter_rotation_t      rotation;
} qmk_oled_painter_device_t;

void translate_pixel_location(qmk_oled_painter_device_t *oled, uint16_t *x, uint16_t *y) {
    switch (oled->rotation) {
        case QP_ROTATION_0:
            // No change
            break;
        case QP_ROTATION_90: {
            int new_x = *y;
            int new_y = *x;
            *x        = new_x;
            *y        = new_y;
        } break;
        case QP_ROTATION_180: {
            int new_x = *y;
            int new_y = *x;
            *x        = new_x;
            *y        = new_y;
        } break;
        case QP_ROTATION_270: {
            int new_x = *y;
            int new_y = *x;
            *x        = new_x;
            *y        = new_y;
        } break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_qmk_oled_wrapper_init(painter_device_t device, painter_rotation_t rotation) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;
    oled->rotation                  = rotation;
    oled_init(OLED_ROTATION_0); // rotation is handled by quantum painter
    oled_clear();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_qmk_oled_wrapper_clear(painter_device_t device) {
    oled_clear();
    return true;
}

bool qp_qmk_oled_wrapper_power(painter_device_t device, bool power_on) {
    if (power_on)
        oled_on();
    else
        oled_off();
    return true;
}

bool qp_qmk_oled_wrapper_pixdata(painter_device_t device, const void *pixel_data, uint32_t byte_count) { return false; }

bool qp_qmk_oled_wrapper_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) { return false; }

bool qp_qmk_oled_wrapper_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    qmk_oled_painter_device_t *oled = (qmk_oled_painter_device_t *)device;
    translate_pixel_location(oled, &x, &y);
    oled_write_pixel(x, y, (val >= 128 ? true : false));
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver storage
static qmk_oled_painter_device_t driver;

// Factory function for creating a handle to the ILI9341 device
painter_device_t qp_qmk_oled_wrapper_make_device(void) {
    memset(&driver, 0, sizeof(qmk_oled_painter_device_t));
    driver.qp_driver.init      = qp_qmk_oled_wrapper_init;
    driver.qp_driver.clear     = qp_qmk_oled_wrapper_clear;
    driver.qp_driver.power     = qp_qmk_oled_wrapper_power;
    driver.qp_driver.pixdata   = qp_qmk_oled_wrapper_pixdata;
    driver.qp_driver.viewport  = qp_qmk_oled_wrapper_viewport;
    driver.qp_driver.setpixel  = qp_qmk_oled_wrapper_setpixel;
    driver.qp_driver.line      = qp_fallback_line;
    driver.qp_driver.rect      = qp_fallback_rect;
    driver.qp_driver.circle    = qp_fallback_circle;
    driver.qp_driver.ellipse   = qp_fallback_ellipse;
    driver.qp_driver.drawimage = NULL;
    return (painter_device_t)&driver;
}
