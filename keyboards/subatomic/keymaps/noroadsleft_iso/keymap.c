#include QMK_KEYBOARD_H
#include "version.h"

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.
// Layer names don't all need to be of the same length, obviously, and you can also skip them
// entirely and just use numbers.
enum layers {
  _QWERTY,
  _DVORAK,
  _COLEMAK,
  _LOWER,
  _RAISE,
  _ADJUST  = 16
};

enum subatomic_keycodes {
  QWERTY = SAFE_RANGE,
  DVORAK,
  COLEMAK,
  LOWER,
  RAISE,
  VRSN
};

#define SYSTEM  TO(_ADJUST)

#define FN_ESC  LT(_LOWER, KC_ESC)
#define BS_ESC  LT(_LOWER, KC_BSPC)

#define LAYOUT_subatomic_1x2u_l( \
    K00,  K01,  K02,  K03,  K04,  K05,  K06,  K07,  K08,  K09,  K0A,  K0B,  K0C,  K0D, \
    K10,  K11,  K12,  K13,  K14,  K15,  K16,  K17,  K18,  K19,  K1A,  K1B,  K1C,  K1D, \
    K20,  K21,  K22,  K23,  K24,  K25,  K26,  K27,  K28,  K29,  K2A,  K2B,  K2C,  K2D, \
    K30,  K31,  K32,  K33,  K34,  K35,  K36,  K37,  K38,  K39,  K3A,  K3B,  K3C,  K3D, \
    K40,  K41,  K42,  K43,  K44,  K45,  K46,  K47,  K48,  K49,  K4A,  K4B,  K4C,  K4D  \
) { \
    { K00,  K01,  K02,  K03,  K04,  K05,  K06,   K07,  K08,  K09,  K0A,  K0B,  K0C,  K0D }, \
    { K10,  K11,  K12,  K13,  K14,  K15,  K16,   K17,  K18,  K19,  K1A,  K1B,  K1C,  K1D }, \
    { K20,  K21,  K22,  K23,  K24,  K25,  K26,   K27,  K28,  K29,  K2A,  K2B,  K2C,  K2D }, \
    { K30,  K31,  K32,  K33,  K34,  K35,  K36,   K37,  K38,  K39,  K3A,  K3B,  K3C,  K3D }, \
    { K40,  K41,  K42,  K43,  K44,  K45,  KC_NO, K47,  K48,  K49,  K4A,  K4B,  K4C,  K4D }, \
}
#define LAYOUT LAYOUT_subatomic_1x2u_l

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  /* Qwerty
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │` ~│1 !│2 "│3 #│4 $│5 %│6 ^│7 &│8 *│9 (│0 )│- _│= +│Bsp│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Tab│Q  │W  │E  │R  │T  │Y  │U  │I  │O  │P  │[ {│] }│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤Ent│
   * │Esc│A  │S  │D  │F• │G  │H  │J• │K  │L  │; :│, @│# ~│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Sft│Z  │X  │C  │V  │B  │N  │M  │,  │.  │/  │Sft│   │Sys│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Ctl│\  │GUI│Alt│Lwr│Spc│Spc│Rse│Alt│Ctl│←  │↓  │↑  │→  │
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_QWERTY] = LAYOUT( \
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, \
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, XXXXXXX, \
    KC_ESC,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_NUHS, KC_ENT,  \
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, _______, SYSTEM,  \
    KC_LCTL, KC_NUBS, KC_LGUI, KC_LALT, LOWER,   KC_SPC,  KC_SPC,  RAISE,   KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT  \
  ),

  /* Dvorak
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │` ~│1 !│2 "│3 #│4 $│5 %│6 ^│7 &│8 *│9 (│0 )│[ {│] }│Bsp│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Tab│' "│,  │.  │P  │Y  │F  │G  │C  │R  │L  │/ ?│= +│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤Ent│
   * │Esc│A  │O  │E  │U• │I  │D  │H• │T  │N  │S  │- _│# ~│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Sft│; :│Q  │J  │K  │X  │B  │M  │W  │V  │Z  │Sft│   │Sys│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Ctl│\  │GUI│Alt│Lwr│Spc│Spc│Rse│Alt│Ctl│←  │↓  │↑  │→  │
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_DVORAK] = LAYOUT( \
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_LBRC, KC_RBRC, KC_BSPC, \
    KC_TAB,  KC_QUOT, KC_COMM, KC_DOT,  KC_P,    KC_Y,    KC_F,    KC_G,    KC_C,    KC_R,    KC_L,    KC_SLSH, KC_EQL,  XXXXXXX, \
    KC_ESC,  KC_A,    KC_O,    KC_E,    KC_U,    KC_I,    KC_D,    KC_H,    KC_T,    KC_N,    KC_S,    KC_MINS, KC_NUHS, KC_ENT,  \
    KC_LSFT, KC_SCLN, KC_Q,    KC_J,    KC_K,    KC_X,    KC_B,    KC_M,    KC_W,    KC_V,    KC_Z,    KC_RSFT, _______, SYSTEM,  \
    KC_LCTL, KC_NUBS, KC_LGUI, KC_LALT, LOWER,   KC_SPC,  KC_SPC,  RAISE,   KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT  \
  ),

  /* Colemak
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │` ~│1 !│2 "│3 #│4 $│5 %│6 ^│7 &│8 *│9 (│0 )│- _│= +│Bsp│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Tab│Q  │W  │F  │P  │G  │J  │L  │U  │Y  │; :│[ {│] }│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤Ent│
   * │Esc│A  │R  │S  │T• │D  │H  │N• │E  │I  │O  │, @│# ~│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Sft│Z  │X  │C  │V  │B  │K  │M  │,  │.  │/  │Sft│   │Sys│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │Ctl│\  │GUI│Alt│Lwr│Spc│Spc│Rse│Alt│Ctl│←  │↓  │↑  │→  │
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_COLEMAK] = LAYOUT( \
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, \
    KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,    KC_G,    KC_J,    KC_L,    KC_U,    KC_Y,    KC_SCLN, KC_LBRC, KC_RBRC, XXXXXXX, \
    KC_ESC,  KC_A,    KC_R,    KC_S,    KC_T,    KC_D,    KC_H,    KC_N,    KC_E,    KC_I,    KC_O,    KC_QUOT, KC_NUHS, KC_ENT,  \
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_K,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, _______, SYSTEM,  \
    KC_LCTL, KC_NUBS, KC_LGUI, KC_LALT, LOWER,   KC_SPC,  KC_SPC,  RAISE,   KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT  \
  ),

  /* Lower
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │XXX│F1 │F2 │F3 │F4 │F5 │F6 │F7 │F8 │F9 │F10│F11│F12│Del│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │Cal│App│   │   │   │Ins│Hm │↑  │End│PUp│PSc│SLk│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤   │
   * │   │   │   │   │ • │   │Del│←• │↓  │→  │PDn│   │Pau│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │CLk│   │   │   │   │   │   │Mut│Vo-│Vo+│   │   │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │▓▓▓│   │   │   │   │   │Prv│Stp│Ply│Nxt│
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_LOWER] = LAYOUT( \
    XXXXXXX, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,  \
    _______, KC_CALC, KC_APP,  _______, _______, _______, KC_INS,  KC_HOME, KC_UP,   KC_END,  KC_PGUP, KC_PSCR, KC_SLCK, _______, \
    _______, _______, _______, _______, _______, _______, KC_DEL,  KC_LEFT, KC_DOWN, KC_RGHT, KC_PGDN, _______, KC_PAUS, _______, \
    KC_CAPS, _______, _______, _______, _______, _______, _______, KC_MUTE, KC_VOLD, KC_VOLU, _______, _______, _______, _______, \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, KC_MPRV, KC_MSTP, KC_MPLY, KC_MNXT  \
  ),

  /* Raise
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │XXX│F13│F14│F15│F16│   │   │7  │8  │9  │   │   │   │Del│
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │F17│F18│F19│F20│   │   │4  │5  │6  │*  │/  │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤Ent│
   * │   │F21│F22│F23│F24│   │   │1• │2  │3  │+  │-  │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │   │   │   │0  │   │.  │   │   │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │   │   │   │▓▓▓│   │   │Prv│Stp│Ply│Nxt│
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_RAISE] = LAYOUT( \
    XXXXXXX, KC_F13,  KC_F14,  KC_F15,  KC_F16,  _______, _______, KC_P7,   KC_P8,   KC_P9,   _______, _______, _______, KC_DEL,  \
    _______, KC_F17,  KC_F18,  KC_F19,  KC_F20,  _______, _______, KC_P4,   KC_P5,   KC_P6,   KC_PAST, KC_PSLS, _______, _______, \
    _______, KC_F21,  KC_F22,  KC_F23,  KC_F24,  _______, _______, KC_P1,   KC_P2,   KC_P3,   KC_PPLS, KC_PMNS, _______, KC_PENT, \
    _______, _______, _______, _______, _______, _______, _______, KC_P0,   _______, KC_PDOT, _______, _______, _______, _______, \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, KC_MPRV, KC_MSTP, KC_MPLY, KC_MNXT  \
  ),

  /* Adjust (Lower + Raise)
   * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
   * │   │   │   │   │   │   │   │   │RST│   │DBG│   │VRS│   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │   │   │   │   │   │   │   │   │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤   │
   * │   │   │   │   │ • │   │   │ • │   │   │   │   │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │   │   │   │   │   │   │   │   │   │   │
   * ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
   * │   │   │   │   │▓▓▓│   │   │▓▓▓│   │   │   │   │   │   │
   * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
   */
  [_ADJUST] = LAYOUT( \
    _______, QWERTY,  DVORAK,  COLEMAK, _______, _______, _______, _______, RESET,   _______, DEBUG,   _______, VRSN,    _______, \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, \
    _______, _______, _______, AU_ON,   AU_OFF,  _______, _______, _______, _______, _______, _______, _______, _______, _______, \
    _______, MUV_DE,  MUV_IN,  MU_ON,   MU_OFF,  MI_ON,   MI_OFF,  _______, _______, _______, _______, _______, _______, _______, \
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______  \
  ),

};


bool process_record_user(uint16_t keycode, keyrecord_t *record) {

  switch (keycode) {

    case QWERTY:
      if (record->event.pressed) {
        set_single_persistent_default_layer(_QWERTY);
      }
      return false;
      break;

    case DVORAK:
      if (record->event.pressed) {
        set_single_persistent_default_layer(_DVORAK);
      }
      return false;
      break;

    case COLEMAK:
      if (record->event.pressed) {
        set_single_persistent_default_layer(_COLEMAK);
      }
      return false;
      break;

    case LOWER:
      if (record->event.pressed) {
        layer_on(_LOWER);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      } else {
        layer_off(_LOWER);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      }
      return false;
      break;

    case RAISE:
      if (record->event.pressed) {
        layer_on(_RAISE);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      } else {
        layer_off(_RAISE);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      }
      return false;
      break;

    case VRSN:
      if (record->event.pressed) {
        SEND_STRING( QMK_KEYBOARD "/" QMK_KEYMAP " @ " QMK_VERSION );
      }
      return false;
      break;
  }
  return true;
};
