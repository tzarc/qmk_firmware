/* Copyright 2021 Paul Cotter (@Gr1mR3aver)
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

#include <string.h>
#include <hal.h>

#ifdef ENABLE_ADC_USBPD_CHECK
#    include <analog.h>
#endif

#include "wing70.h"
#include "serial_usart_statesync.h"
#include "sync_state_kb.h"

#include "qp_st7735r.h"

painter_device_t lcd;
painter_device_t surf;

//----------------------------------------------------------
// LCD power control

// void djinn_lcd_on(void) {
//     dprint("djinn_lcd_on\n");
//     kb_runtime_config* kb_state = get_split_sync_state_kb();
//     kb_state->values.lcd_power  = 1;
// }

// void djinn_lcd_off(void) {
//     dprint("djinn_lcd_off\n");
//     kb_runtime_config* kb_state = get_split_sync_state_kb();
//     kb_state->values.lcd_power  = 0;
// }

// void djinn_lcd_toggle(void) {
//     kb_runtime_config* kb_state = get_split_sync_state_kb();
//     if (kb_state->values.lcd_power)
//         djinn_lcd_off();
//     else
//         djinn_lcd_on();
// }

//----------------------------------------------------------
// Initialisation

void keyboard_post_init_kb(void) {
    debug_enable = true;
    debug_matrix = true;

    // Reset the initial shared data value between master and slave
    // kb_runtime_config* kb_state = get_split_sync_state_kb();
    // kb_state->raw               = 0;

    // Let the LCD get some power...
    wait_ms(50);

    // Initialise the LCD
    lcd = qp_st7735r_make_device(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, 4, true);
    qp_init(lcd, QP_ROTATION_0);

    qp_power(lcd, true);

    // Turn on the LCD and draw the logo
    qp_rect(lcd, 0, 0, 131, 161, HSV_BLACK, true);

    // draw hue column
    for (int i = 0; i < QP_SCREEN_HEIGHT; i++) {
        qp_line(lcd, 0, i, 7, i, ((256/QP_SCREEN_HEIGHT)*i) % 256, 255, 255);
    }

    // Turn on the LCD backlight
    backlight_enable();

    uint8_t bllevel = BACKLIGHT_LEVELS;
    do
    {
        backlight_level(bllevel);
        wait_ms(50);
        bllevel = get_backlight_level() -1;
    } while (bllevel > 0);

    backlight_level((uint8_t)(BACKLIGHT_LEVELS/2));

    while(rgb_matrix_get_val() > 128)
    {
        rgb_matrix_decrease_val_noeeprom();
    }

    rgb_matrix_decrease_val();
    qp_circle(lcd,50,50,20,HSV_RED,true);

}

//----------------------------------------------------------
// QMK overrides

void encoder_update_kb(uint8_t index, bool clockwise) {
    // Offload to the keymap instead.
    extern void encoder_update_user(uint8_t index, bool clockwise);
    encoder_update_user(index, clockwise);
}

void suspend_power_down_kb(void) {
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    suspend_wakeup_init_user();
}

#ifdef RGB_MATRIX_ENABLE
    // clang-format off
    #define RLO 30
    #define LLI(x) (x)
    #define LLP(x,y) {(x),(y)}
    #define RLI(x) (RLO+(x))
    #define RLP(x,y) {(224-(x)),((y))}
    led_config_t g_led_config = {
        {
            // Key Matrix to LED Index
            { LLI(0),  LLI(1),  LLI(2),  LLI(3),  LLI(4),  LLI(5) },
            { LLI(11),  LLI(10),  LLI(9),  LLI(7),  LLI(7), LLI(6) },
            { LLI(12), LLI(13), LLI(14), LLI(15), LLI(16), LLI(17) },
            { LLI(23), LLI(22), LLI(21), LLI(20), LLI(19), LLI(18) },
            { LLI(24), LLI(25), LLI(26), LLI(27), LLI(28), LLI(29) },
            { NO_LED,  NO_LED,  NO_LED,  NO_LED,  NO_LED,  NO_LED },
            { RLI(0),  RLI(1),  RLI(2),  RLI(3),  RLI(4),  RLI(5) },
            { RLI(11),  RLI(10),  RLI(9),  RLI(7),  RLI(7), RLI(6) },
            { RLI(12), RLI(13), RLI(14), RLI(15), RLI(16), RLI(17) },
            { RLI(23), RLI(22), RLI(21), RLI(20), RLI(19), RLI(18) },
            { RLI(24), RLI(25), RLI(26), RLI(27), RLI(28), RLI(29) },
            { NO_LED,  NO_LED,  NO_LED,  NO_LED,  NO_LED,  NO_LED },        },
        {
            // LED Index to Physical Position
            // Matrix left
            LLP( 160,	   7), 	LLP( 128,	   5), 	LLP(  96,	   0), 	LLP(  64,	   4), 	LLP(  32,	   8), 	LLP(   0,	   8),
            LLP(   0,	  19), 	LLP(  32,	  19), 	LLP(  64,	  15), 	LLP(  96,	  11), 	LLP( 128,	  16), 	LLP( 160,	  18),
            LLP( 160,	  29), 	LLP( 128,	  27), 	LLP(  96,	  21), 	LLP(  64,	  26), 	LLP(  32,	  30), 	LLP(   0,	  30),
            LLP(   0,	  41), 	LLP(  32,	  41), 	LLP(  64,	  37), 	LLP(  96,	  32), 	LLP( 128,	  38), 	LLP( 160,	  40),
            LLP( 122,	  48), 	LLP( 128,	  50), 	LLP( 176,	  55), 	LLP( 192,	  64), 	LLP( 224,	  60), 	LLP( 208,	  53),
            // Matrix right
            RLP( 160,	   7), 	RLP( 128,	   5), 	RLP(  96,	   0), 	RLP(  64,	   4), 	RLP(  32,	   8), 	RLP(   0,	   8),
            RLP(   0,	  19), 	RLP(  32,	  19), 	RLP(  64,	  15), 	RLP(  96,	  11), 	RLP( 128,	  16), 	RLP( 160,	  18),
            RLP( 160,	  29), 	RLP( 128,	  27), 	RLP(  96,	  21), 	RLP(  64,	  26), 	RLP(  32,	  30), 	RLP(   0,	  30),
            RLP(   0,	  41), 	RLP(  32,	  41), 	RLP(  64,	  37), 	RLP(  96,	  32), 	RLP( 128,	  38), 	RLP( 160,	  40),
            RLP( 122,	  48), 	RLP( 128,	  50), 	RLP( 176,	  55), 	RLP( 192,	  64), 	RLP( 224,	  60), 	RLP( 208,	  53),

        },
        {
            // LED Index to Flag
            // Matrix left
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            // Matrix right
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
            LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT,
        }
    };
// clang-format on
#endif  // RGB_MATRIX_ENABLE
