// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum.h>
#include <qp.h>
#include <qp_ili9163.h>
#include <qp_st7789.h>

// Test assets
#include "thintel15.c"
#include "test-image.c"

painter_device_t ili9163;
painter_device_t st7789;
painter_device_t st7735;

void init_and_clear(painter_device_t device, painter_rotation_t rotation) {
    uint16_t width;
    uint16_t height;
    qp_get_geometry(device, &width, &height, NULL, NULL, NULL);

    qp_init(device, rotation);
    qp_rect(device, 0, 0, width - 1, height - 1, 0, 0, 0, true);
}

void draw_test(painter_device_t device, const char *name, uint32_t now) {
    uint16_t width;
    uint16_t height;
    qp_get_geometry(device, &width, &height, NULL, NULL, NULL);

    static uint8_t hue = 0;
    hue += 4;

    char buf1[32] = {0};
    char buf2[32] = {0};
    char buf3[32] = {0};
    sprintf(buf1, "QMK on %s!", name);
    sprintf(buf2, "MCU time is: %dms", (int)now);
    sprintf(buf3, "Hue is: %d", (int)hue);

    qp_drawtext(device, 0, 0 * font_thintel15->glyph_height, font_thintel15, buf1);
    int16_t xpos = qp_drawtext_recolor(device, 0, 1 * font_thintel15->glyph_height, font_thintel15, buf2, hue, 255, 255, hue, 255, 0);
    qp_rect(device, xpos, 1 * font_thintel15->glyph_height, width - 1, 2 * font_thintel15->glyph_height, 0, 0, 0, true);
    xpos = qp_drawtext_recolor(device, 0, 2 * font_thintel15->glyph_height, font_thintel15, buf3, hue, 255, 255, hue, 255, 0);
    qp_rect(device, xpos, 2 * font_thintel15->glyph_height, width - 1, 3 * font_thintel15->glyph_height, 0, 0, 0, true);
    qp_drawimage_recolor(device, 0, 3 * font_thintel15->glyph_height, gfx_test_image, hue, 255, 255, hue, 255, 0);
    qp_flush(device);
}

void keyboard_post_init_kb(void) {
    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;

    ili9163 = qp_ili9163_make_spi_device(128, 128, DISPLAY_CS_PIN_1_44_INCH_LCD_ILI9163, DISPLAY_DC_PIN, DISPLAY_RST_PIN_1_44_INCH_LCD_ILI9163, 8, 0);
    init_and_clear(ili9163, QP_ROTATION_90);

    st7789 = qp_st7789_make_spi_device(240, 240, DISPLAY_CS_PIN_1_3_INCH_LCD_ST7789, DISPLAY_DC_PIN, DISPLAY_RST_PIN_1_3_INCH_LCD_ST7789, 16, 3);
    init_and_clear(st7789, QP_ROTATION_270);
}

void matrix_scan_kb(void) {
    static uint32_t last_scan = 0;
    uint32_t        now       = timer_read32();
    if (TIMER_DIFF_32(now, last_scan) >= 100) {
        last_scan = now;

        draw_test(ili9163, "ILI9163", now);
        draw_test(st7789, "ST7789", now);
    }
}
