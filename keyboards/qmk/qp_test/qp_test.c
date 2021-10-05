/* Copyright 2021 Nick Brassel (@tzarc)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#include <qp.h>
#include <qp_ili9163.h>
#include <qp_st7789.h>

// Test assets
#include "thintel15.c"
#include "dh-test.c"

painter_device_t ili9163;
painter_device_t st7789;

void keyboard_post_init_kb(void) {
    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;

    ili9163 = qp_ili9163_make_spi_device(128, 128, DISPLAY_CS_PIN_1_44_INCH_LCD_ILI9163, DISPLAY_DC_PIN, DISPLAY_RST_PIN, 16, 0);
    qp_init(ili9163, QP_ROTATION_0);
    qp_rect(ili9163, 0, 0, 127, 127, 0, 0, 0, true);

    // st7789 = qp_st7789_make_spi_device(240, 240, DISPLAY_CS_PIN_1_3_INCH_LCD_ST7789, DISPLAY_DC_PIN, DISPLAY_RST_PIN, 16, 3);
    // qp_init(st7789, QP_ROTATION_0);
    // qp_rect(st7789, 0, 0, 239, 239, 0, 0, 0, true);
}

void matrix_scan_kb(void) {
    static uint32_t last_scan = 0;
    uint32_t        now       = timer_read32();
    if (TIMER_DIFF_32(now, last_scan) >= 500) {
        last_scan = now;

        static uint8_t hue = 0;
        hue += 4;

        char buf1[32] = {0};
        char buf2[32] = {0};
        sprintf(buf1, "The current MCU time is: %dms", (int)last_scan);
        sprintf(buf2, "The current hue is: %d", (int)hue);

        qp_drawtext(ili9163, 0, 0 * font_thintel15->glyph_height, font_thintel15, "QMK on ILI9163!");
        int16_t xpos = qp_drawtext_recolor(ili9163, 0, 1 * font_thintel15->glyph_height, font_thintel15, buf1, hue, 255, 255, hue, 255, 0);
        qp_rect(ili9163, xpos, 1 * font_thintel15->glyph_height, 127, 2 * font_thintel15->glyph_height, 0, 0, 0, true);
        xpos = qp_drawtext_recolor(ili9163, 0, 2 * font_thintel15->glyph_height, font_thintel15, buf2, hue, 255, 255, hue, 255, 0);
        qp_rect(ili9163, xpos, 2 * font_thintel15->glyph_height, 127, 3 * font_thintel15->glyph_height, 0, 0, 0, true);
        qp_drawimage(ili9163, 0, 3 * font_thintel15->glyph_height, gfx_dh_test);
        qp_flush(ili9163);

        // qp_drawtext(st7789, 0, 0 * font_thintel15->glyph_height, font_thintel15, "QMK on ST7789!");
        // qp_drawtext_recolor(st7789, 0, 1 * font_thintel15->glyph_height, font_thintel15, buf1, hue, 255, 255, hue, 255, 0);
        // qp_drawtext_recolor(st7789, 0, 2 * font_thintel15->glyph_height, font_thintel15, buf2, hue, 255, 255, hue, 255, 0);
        // qp_drawimage(st7789, 0, 3 * font_thintel15->glyph_height, gfx_dh_test);
        // qp_flush(st7789);
    }
}
