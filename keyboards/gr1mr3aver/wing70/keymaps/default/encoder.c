/* Copyright 2018-2020 Nick Brassel (@tzarc)
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

#include QMK_KEYBOARD_H
#include "print.h"

#define MEDIA_KEY_DELAY 2

void encoder_update_user_left(bool clockwise, uint8_t mods) {
    bool    is_ctrl  = mods & MOD_MASK_CTRL;
    bool    is_shift = mods & MOD_MASK_SHIFT;
    if (is_shift) {
        dprintf(" -- ENCODER: RGB hue ");
        if (clockwise) {
            rgblight_increase_hue();
            dprintf("INCREASED\n");
        } else {
            rgblight_decrease_hue();
            dprintf("DECREASED\n");
        }
    } else if (is_ctrl) {
        dprintf(" -- ENCODER: RGB value ");
        if (clockwise) {
            rgblight_increase_val();
            dprintf("INCREASED\n");
        } else {
            rgblight_decrease_val();
            dprintf("DECREASED\n");
        }
    } else {
        dprintf(" -- ENCODER: Mouse scroll wheel ");
        if (clockwise) {
            tap_code16(KC_MS_WH_DOWN);
            dprintf("DOWN\n");
        } else {
            tap_code16(KC_MS_WH_UP);
            dprintf("UP\n");
        }
    }

}

void encoder_update_user_right(bool clockwise, uint8_t mods) {
    bool    is_ctrl  = mods & MOD_MASK_CTRL;
    bool    is_shift = mods & MOD_MASK_SHIFT;
    if (is_shift) {
            dprintf(" -- ENCODER: RGB saturation ");
            if (clockwise) {
                dprintf("INCREASED\n");
                rgblight_decrease_sat();
            } else {
                rgblight_increase_sat();
                dprintf("DECREASED\n");
            }
    } else if (is_ctrl) {
            dprintf(" -- ENCODER: RGB speed ");
        if (clockwise) {
            rgblight_increase_speed();
            dprintf("INCREASED\n");
        } else {
            rgblight_decrease_speed();
            dprintf("DECREASED\n");
        }
    } else {
        uint16_t held_keycode_timer = timer_read();
        uint16_t mapped_code        = 0;
        dprintf(" -- ENCODER: Volume ");
        if (clockwise) {
            mapped_code = KC_VOLD;
            dprintf("DECREASED\n");
        } else {
            mapped_code = KC_VOLU;
            dprintf("INCREASED\n");
        }
        register_code(mapped_code);
        while (timer_elapsed(held_keycode_timer) < MEDIA_KEY_DELAY)
            ; /* no-op */
        unregister_code(mapped_code);
    }

}

void encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t mods = get_mods() | get_oneshot_mods();
//    uint8_t temp_layer = get_

// catch the case where we are in "config" mode

//  navigate menu options


    if (index == 0) { /* First encoder */
        encoder_update_user_left(clockwise, mods);
    }
    else
    {
        encoder_update_user_right(clockwise, mods);
    }


}
