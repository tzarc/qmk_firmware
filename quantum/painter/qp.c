/* Copyright 2021 Nick Brassel (@tzarc)
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

#include <quantum.h>
#include <utf8.h>
#include <qp_internal.h>
#include <qp_utils.h>

static bool validate_driver_vtable(struct painter_driver_t *driver) { return (driver->driver_vtable && driver->driver_vtable->init && driver->driver_vtable->power && driver->driver_vtable->clear && driver->driver_vtable->viewport && driver->driver_vtable->pixdata && driver->driver_vtable->palette_convert && driver->driver_vtable->append_pixels) ? true : false; }

static bool validate_comms_vtable(struct painter_driver_t *driver) { return (driver->comms_vtable && driver->comms_vtable->comms_init && driver->comms_vtable->comms_start && driver->comms_vtable->comms_stop && driver->comms_vtable->comms_send) ? true : false; }

static bool validate_driver_integrity(struct painter_driver_t *driver) { return validate_driver_vtable(driver) && validate_comms_vtable(driver); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// External API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool qp_init(painter_device_t device, painter_rotation_t rotation) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (!validate_driver_integrity(driver)) {
        return false;
    }

    return driver->driver_vtable->init(device, rotation);
}

bool qp_power(painter_device_t device, bool power_on) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->driver_vtable->power(device, power_on);
}

bool qp_clear(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->driver_vtable->clear(device);
}

bool qp_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->driver_vtable->viewport(device, left, top, right, bottom);
}

bool qp_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->driver_vtable->pixdata(device, pixel_data, native_pixel_count);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image) { return qp_drawimage_recolor(device, x, y, image, 0, 0, 255); }

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, uint8_t hue, uint8_t sat, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->TEMP_vtable && driver->TEMP_vtable->drawimage) {
        return driver->TEMP_vtable->drawimage(device, x, y, image, hue, sat, val);
    }
    return false;
}

int16_t qp_textwidth(painter_font_t font, const char *str) {
    const painter_raw_font_descriptor_t *fdesc = (const painter_raw_font_descriptor_t *)font;
    const char *                         c     = str;
    int16_t                              width = 0;
    while (*c) {
        int32_t code_point = 0;
        c                  = decode_utf8(c, &code_point);

        if (code_point >= 0) {
            if (code_point >= 0x20 && code_point < 0x7F) {
                if (fdesc->ascii_glyph_definitions != NULL) {
                    // Search the font's ascii table
                    uint8_t                                  index      = code_point - 0x20;
                    const painter_font_ascii_glyph_offset_t *glyph_desc = &fdesc->ascii_glyph_definitions[index];
                    width += glyph_desc->width;
                }
            }
#ifdef UNICODE_ENABLE
            else {
                // Search the font's unicode table
                if (fdesc->unicode_glyph_definitions != NULL) {
                    for (uint16_t index = 0; index < fdesc->unicode_glyph_count; ++index) {
                        const painter_font_unicode_glyph_offset_t *glyph_desc = &fdesc->unicode_glyph_definitions[index];
                        if (glyph_desc->unicode_glyph == code_point) {
                            width += glyph_desc->width;
                        }
                    }
                }
            }
#endif  // UNICODE_ENABLE
        }
    }

    return width;
}

int16_t qp_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str) { return qp_drawtext_recolor(device, x, y, font, str, 0, 0, 255, 0, 0, 0); }

int16_t qp_drawtext_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if (driver->TEMP_vtable && driver->TEMP_vtable->drawtext) {
        return driver->TEMP_vtable->drawtext(device, x, y, font, str, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    }
    return 0;
}
