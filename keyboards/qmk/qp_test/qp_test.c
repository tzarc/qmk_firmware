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
#include "thintel15.c"

painter_device_t ili9163;
painter_device_t st7789;

void keyboard_post_init_kb(void) {
    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;

    // ili9163 = qp_ili9163_make_spi_device(DISPLAY_CS_PIN_1_44_INCH_LCD_ILI9163, DISPLAY_DC_PIN, DISPLAY_RST_PIN, 8);
    // qp_init(ili9163, QP_ROTATION_0);

    st7789 = qp_st7789_make_spi_device(DISPLAY_CS_PIN_1_3_INCH_LCD_ST7789, DISPLAY_DC_PIN, DISPLAY_RST_PIN, 8);
    qp_init(st7789, QP_ROTATION_0);
}

void matrix_scan_kb(void) {
    static uint32_t last_scan = 0;
    uint32_t        now       = timer_read32();
    if (TIMER_DIFF_32(now, last_scan) >= 5000) {
        last_scan = now;

        char buf[32] = {0};
        sprintf(buf, "The current MCU time is: %dms", (int)last_scan);

        // qp_rect(ili9163, 0, 0, 127, 127, 0, 0, 0, true);
        // qp_drawtext(ili9163, 0, 0, font_thintel15, "QMK on ILI9163!");
        // qp_drawtext_recolor(ili9163, 0, font_thintel15->glyph_height, font_thintel15, buf, 150, 255, 255, 150, 255, 0);
        // qp_flush(ili9163);

        qp_rect(st7789, 0, 0, 239, 239, 0, 0, 0, true);
        qp_drawtext(st7789, 0, 0, font_thintel15, "QMK on ST7789!");
        qp_drawtext_recolor(st7789, 0, font_thintel15->glyph_height, font_thintel15, buf, 150, 255, 255, 150, 255, 0);
        qp_flush(st7789);
    }
}
