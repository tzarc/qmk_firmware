// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum.h>
#include <utf8.h>
#include <qp_internal.h>
#include <qp_comms.h>
#include <qp_draw.h>

static bool validate_driver_vtable(struct painter_driver_t *driver) { return (driver->driver_vtable && driver->driver_vtable->init && driver->driver_vtable->power && driver->driver_vtable->clear && driver->driver_vtable->viewport && driver->driver_vtable->pixdata && driver->driver_vtable->palette_convert && driver->driver_vtable->append_pixels) ? true : false; }

static bool validate_comms_vtable(struct painter_driver_t *driver) { return (driver->comms_vtable && driver->comms_vtable->comms_init && driver->comms_vtable->comms_start && driver->comms_vtable->comms_stop && driver->comms_vtable->comms_send) ? true : false; }

static bool validate_driver_integrity(struct painter_driver_t *driver) { return validate_driver_vtable(driver) && validate_comms_vtable(driver); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// External API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_init(painter_device_t device, painter_rotation_t rotation) {
    qp_dprintf("qp_init: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;

    driver->validate_ok = false;
    if (!validate_driver_integrity(driver)) {
        qp_dprintf("Failed to validate driver integrity in qp_init\n");
        return false;
    }

    driver->validate_ok = true;

    if (!qp_comms_init(device)) {
        driver->validate_ok = false;
        qp_dprintf("qp_init: fail (could not init comms)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_init: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->init(device, rotation);
    qp_comms_stop(device);
    qp_dprintf("qp_init: %s\n", ret ? "ok" : "fail");
    return ret;
}

bool qp_power(painter_device_t device, bool power_on) {
    qp_dprintf("qp_power: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_power: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_power: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->power(device, power_on);
    qp_comms_stop(device);
    qp_dprintf("qp_power: %s\n", ret ? "ok" : "fail");
    return ret;
}

bool qp_clear(painter_device_t device) {
    qp_dprintf("qp_clear: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_clear: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_clear: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->clear(device);
    qp_comms_stop(device);
    qp_dprintf("qp_clear: %s\n", ret ? "ok" : "fail");
    return ret;
}

bool qp_flush(painter_device_t device) {
    qp_dprintf("qp_flush: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_flush: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_flush: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->flush(device);
    qp_comms_stop(device);
    qp_dprintf("qp_flush: %s\n", ret ? "ok" : "fail");
    return ret;
}

void qp_geometry(painter_device_t device, uint16_t *width, uint16_t *height) {
    qp_dprintf("qp_geometry: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;

    if (width) {
        *width = driver->screen_width;
    }

    if (height) {
        *height = driver->screen_height;
    }

    qp_dprintf("qp_geometry: ok\n");
}

bool qp_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    qp_dprintf("qp_viewport: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_viewport: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_viewport: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->viewport(device, left, top, right, bottom);
    qp_dprintf("qp_viewport: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}

bool qp_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    qp_dprintf("qp_pixdata: entry\n");
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!driver->validate_ok) {
        qp_dprintf("qp_pixdata: fail (validation_ok == false)\n");
        return false;
    }

    if (!qp_comms_start(device)) {
        qp_dprintf("qp_pixdata: fail (could not start comms)\n");
        return false;
    }

    bool ret = driver->driver_vtable->pixdata(device, pixel_data, native_pixel_count);
    qp_dprintf("qp_pixdata: %s\n", ret ? "ok" : "fail");
    qp_comms_stop(device);
    return ret;
}
