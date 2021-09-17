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

#include <quantum.h>
#include <utf8.h>
#include <qp_internal.h>
#include <qp_utils.h>
#include <qp.h>

#include "qp_draw_impl.c"

#ifdef CONSOLE_ENABLE
#   include <print.h>
#endif // CONSOLE_ENABLE


#ifdef QP_ENABLE_SPI
#   include <spi_master.h>
#endif

#ifdef QP_ENABLE_PARALLEL
#   include "parallel_common/parallel.h"
#endif

#ifdef QP_ENABLE_SPI
bool qp_internal_send_spi(painter_device_t device, const void *data, uint16_t data_length) {
    uint16_t       bytes_remaining = data_length;
    const uint8_t *p               = (const uint8_t *)data;
    while (bytes_remaining > 0) {
        uint16_t bytes_this_loop = bytes_remaining < 1024 ? bytes_remaining : 1024;
        spi_transmit(p, bytes_this_loop);
        p += bytes_this_loop;
        bytes_remaining -= bytes_this_loop;
    }
    return true;
}
#endif // QP_ENABLE_SPI

#ifdef QP_ENABLE_PARALLEL
bool qp_internal_send_parallel(painter_device_t device, const void *data, uint16_t data_length) {
    return parallel_transmit(data, data_length);
}
#endif // QP_ENABLE_PARALLEL

#ifdef QP_ENABLE_I2C
bool qp_internal_send_i2c(painter_device_t device, const void *data, uint16_t data_length) {
#ifdef CONSOLE_ENABLE
    dprint("I2C interface is not implemented");
#endif // CONSOLE ENABLE
}
#endif // QP_ENABLE_I2C

bool qp_comms_init(painter_device_t device, painter_rotation_t rotation) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    switch(driver->comms_interface) {
#ifdef QP_ENABLE_SPI
        case SPI:
            spi_init();
            driver->comms_write = qp_internal_send_spi;
            break;
#endif // QP_ENABLE_SPI
#ifdef QP_ENABLE_PARALLEL
        case PARALLEL:
            parallel_init();
            driver->comms_write = qp_internal_send_parallel;
            break;
#endif // QP_ENABLE_PARALLEL
#ifdef QP_ENABLE_I2C
        case I2C:
#   ifdef CONSOLE_ENABLE
            driver->comms_write = qp_internal_send_i2c;
#   endif
            break;
#endif // QP_ENABLE_I2C
        default:
#   ifdef CONSOLE_ENABLE
            dprintf("Interface %i is not implemented", driver->comms_interface);
#   endif // CONSOLE_ENABLE
            break;
    }
    return true;
}

bool qp_start(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;

    static bool retval = false;
    if(!driver->render_started) {
        switch(driver->comms_interface) {
    #ifdef QP_ENABLE_SPI
            case SPI:
                retval = spi_start(driver->spi.chip_select_pin, driver->spi.is_little_endian, driver->spi.mode, driver->spi.clock_divisor);
                driver->render_started = true;
                break;
    #endif // QP_ENABLE_SPI
    #ifdef QP_ENABLE_PARALLEL
            case PARALLEL:
                retval = parallel_start(driver->parallel.write_pin, driver->parallel.read_pin, driver->parallel.chip_select_pin);
                driver->render_started = true;
                break;
    #endif // QP_ENABLE_PARALLEL
    #ifdef QP_ENABLE_I2C
            case I2C:
    #   ifdef CONSOLE_ENABLE
                dprint("I2C interface is not implemented");
    #   endif // CONSOLE ENABLE
                break;
    #endif // QP_ENABLE_I2C
            default:
    #   ifdef CONSOLE_ENABLE
                dprintf("interface %i is not implemented", driver->comms_interface);
    #   endif // CONSOLE_ENABLE
                break;
        }
    }
#ifdef CONSOLE_ENABLED
    else {
            dprintf("skipping qp_start - already started");
    }
#endif

    return retval;
}

bool qp_stop(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    if(driver->render_started) {
        switch(driver->comms_interface) {
    #ifdef QP_ENABLE_SPI
            case SPI:
                spi_stop();
                driver->render_started = false;
                return true;
    #endif // QP_ENABLE_SPI
    #ifdef QP_ENABLE_PARALLEL
            case PARALLEL:
                parallel_stop();
                driver->render_started = false;
                return true;
    #endif // QP_ENABLED_PARALLEL
    #ifdef QP_ENABLE_I2C
            case I2C:
    #   ifdef CONSOLE_ENABLE
                dprint("I2C interface is not implemented");
    #   endif // CONSOLE ENABLE
                break;
    #endif // QP_ENABLE_I2C
            default:
    #   ifdef CONSOLE_ENABLE
                dprintf("interface %i is not implemented", driver->comms_interface);
                driver->render_started = false;
    #   endif // CONSOLE_ENABLE
                break;
        }
    }
#ifdef CONSOLE_ENABLED
    else {
        dprint("Skipping qp_stop - render not started");
    }
#endif

    return false;
}

bool qp_send(painter_device_t device, const uint8_t *data, uint16_t data_length) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    return driver->comms_write(driver, data, data_length);
}

bool qp_init(painter_device_t device, painter_rotation_t rotation) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;

    qp_comms_init(device, rotation);

    qp_start(device);
    if (driver->init) {
        retval = driver->init(device, rotation);
    }
    qp_stop(device);
    return retval;
}

bool qp_clear(painter_device_t device) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->clear) {
        retval = driver->clear(device);
    } else {
        dprint("--blanking screen with rect\n");
        // just draw a black rectangle on the screen
        qp_internal_impl_rect(device, 0, 0, driver->screen_width - 1, driver->screen_height - 1, HSV_BLACK, true);
    }
    qp_stop(device);
    return retval;
}

bool qp_power(painter_device_t device, bool power_on) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->power) {
        retval = driver->power(device, power_on);
    }
    qp_stop(device);
    return retval;
}

bool qp_brightness(painter_device_t device, uint8_t val) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->brightness) {
        retval = driver->brightness(device, val);
    }
    qp_stop(device);
    return retval;
}

bool qp_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->viewport) {
        retval = driver->viewport(device, left, top, right, bottom, render_continue);
    }
    if(!render_continue) qp_stop(device);
    return retval;
}

bool qp_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->pixdata) {
        retval = driver->pixdata(device, pixel_data, native_pixel_count, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "pixdata");
#endif
        retval = qp_internal_impl_pixdata(device, pixel_data, native_pixel_count);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_setpixel(painter_device_t device, uint16_t x, uint16_t y, uint8_t hue, uint8_t sat, uint8_t val, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->setpixel) {
        retval = driver->setpixel(device, x, y, hue, sat, val, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "setpixel");
#endif
        retval = qp_internal_impl_setpixel(device, x, y, hue, sat, val);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_line(painter_device_t device, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t hue, uint8_t sat, uint8_t val, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval = false;
    qp_start(device);
    if (driver->line) {
        retval = driver->line(device, x0, y0, x1, y1, hue, sat, val, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "line");
#endif
        retval = qp_internal_impl_line(device, x0, y0, x1, y1, hue, sat, val);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_rect(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, uint8_t hue, uint8_t sat, uint8_t val, bool filled, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;

    qp_start(device);
    if (driver->rect) {
#ifdef CONSOLE_ENABLE
        dprintf("using driver %s func\n", "rect");
#endif
        retval = driver->rect(device, left, top, right, bottom, hue, sat, val, filled, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "rect");
#endif
        retval = qp_internal_impl_rect(device, left, top, right, bottom, hue, sat, val, filled);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_circle(painter_device_t device, uint16_t x, uint16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->circle) {
        retval = driver->circle(device, x, y, radius, hue, sat, val, filled, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "circle");
#endif
        retval = qp_internal_impl_circle(device, x, y, radius, hue, sat, val, filled);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_ellipse(painter_device_t device, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint8_t hue, uint8_t sat, uint8_t val, bool filled, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->ellipse) {
        retval = driver->ellipse(device, x, y, sizex, sizey, hue, sat, val, filled, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "ellipse");
#endif
        retval = qp_internal_impl_ellipse(device, x, y, sizex, sizey, hue, sat, val, filled);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}

bool qp_drawimage(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, bool render_continue) {
    return qp_drawimage_recolor(device, x, y, image, 0, 0, 255, render_continue);
}

bool qp_drawimage_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_image_t image, uint8_t hue, uint8_t sat, uint8_t val, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->drawimage) {
        retval = driver->drawimage(device, x, y, image, hue, sat, val, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "drawimage_recolor");
#endif
        retval = qp_internal_impl_drawimage(device, x, y, image, hue, sat, val);
    }
    if (!render_continue) qp_stop(device);
    return retval;
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

int16_t qp_drawtext(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, bool render_continue) { return qp_drawtext_recolor(device, x, y, font, str, 0, 0, 255, 0, 0, 0, render_continue); }

int16_t qp_drawtext_recolor(painter_device_t device, uint16_t x, uint16_t y, painter_font_t font, const char *str, uint8_t hue_fg, uint8_t sat_fg, uint8_t val_fg, uint8_t hue_bg, uint8_t sat_bg, uint8_t val_bg, bool render_continue) {
    struct painter_driver_t *driver = (struct painter_driver_t *)device;
    bool retval;
    qp_start(device);
    if (driver->drawtext) {
        retval = driver->drawtext(device, x, y, font, str, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg, render_continue);
    } else {
#ifdef CONSOLE_ENABLE
        dprintf("using default %s func\n", "drawtext_recolor");
#endif
        retval = qp_internal_impl_drawtext_recolor(device, x, y, font, str, hue_fg, sat_fg, val_fg, hue_bg, sat_bg, val_bg);
    }
    if (!render_continue) qp_stop(device);
    return retval;
}



