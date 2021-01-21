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

#include "quantum.h"

#include "qp_internal.h"
#include "qp_utils.h"

bool qp_init(painter_device_t device, painter_rotation_t rotation) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->init) {
        return driver->init(device, rotation);
    }
    return false;
}

bool qp_clear(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->clear) {
        return driver->clear(device);
    }
    return false;
}

bool qp_power(painter_device_t device, bool power_on) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->power) {
        return driver->power(device, power_on);
    }
    return false;
}

bool qp_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->viewport) {
        return driver->viewport(device, left, top, right, bottom);
    }
    return false;
}

bool qp_pixdata(painter_device_t device, const void *pixel_data, uint32_t byte_count) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->pixdata) {
        return driver->pixdata(device, pixel_data, byte_count);
    }
    return false;
}

bool qp_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->setpixel) {
        return driver->setpixel(device, x, y, hue, sat, val);
    }
    return false;
}

bool qp_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->line) {
        return driver->line(device, x0, y0, x1, y1, hue, sat, val);
    }
    return false;
}

bool qp_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->rect) {
        return driver->rect(device, left, top, right, bottom, hue, sat, val, filled);
    }
    return false;
}

bool qp_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->circle) {
        return driver->circle(device, x, y, radius, hue, sat, val, filled);
    }
    return false;
}

bool qp_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->ellipse) {
        return driver->ellipse(device, x, y, sizex, sizey, hue, sat, val, filled);
    }
    return false;
}

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image) { return qp_drawimage_recolor(device, x, y, image, 0, 0, 255); }

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->drawimage) {
        return driver->drawimage(device, x, y, image, hue, sat, val);
    }
    return false;
}
