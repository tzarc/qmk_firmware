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

#include "qp_internal.h"

bool qp_comms_init(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->comms_vtable && driver->comms_vtable->comms_init) {
        return driver->comms_vtable->comms_init(device);
    }
    return false;
}

bool qp_comms_start(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->comms_vtable && driver->comms_vtable->comms_start) {
        return driver->comms_vtable->comms_start(device);
    }
    return false;
}

void qp_comms_stop(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->comms_vtable && driver->comms_vtable->comms_stop) {
        driver->comms_vtable->comms_stop(device);
    }
}

size_t qp_comms_send(painter_device_t device, const void *data, size_t byte_count) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->comms_vtable && driver->comms_vtable->comms_send) {
        return driver->comms_vtable->comms_send(device, data, byte_count);
    }
    return 0;
}
