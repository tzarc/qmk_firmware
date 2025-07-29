# Encoders

::: info Modern JSON Configuration
This documentation now shows the modern JSON-based configuration approach. All examples use `keyboard.json` or `info.json` configuration with legacy `config.h`/`rules.mk` alternatives provided for reference. For more information about data-driven configuration, see [Data Driven Configuration](data_driven_config).
:::

### Modern Configuration (Recommended)

Configure encoders in your `keyboard.json`:

```json
{
    "features": {
        "encoder": true
    },
    "encoder": {
        "rotary": [
            {"pin_a": "B12", "pin_b": "B13"}
        ]
    }
}
```

::: details Legacy rules.mk/config.h Configuration
Basic (EC11 compatible) encoders are supported by adding this to your `rules.mk`:

```make
ENCODER_ENABLE = yes
```

and this to your `config.h`:

```c
#define ENCODER_A_PINS { B12 }
#define ENCODER_B_PINS { B13 }
```

This method is still supported but consider migrating to the JSON configuration above.
:::

Configure multiple encoders by adding more entries to the rotary array:

```json
{
    "encoder": {
        "rotary": [
            {"pin_a": "B6", "pin_b": "B2"},
            {"pin_a": "B3", "pin_b": "B1"}
        ]
    }
}
```

### Configuration Options

::: warning
Direction configuration is not currently available in JSON format. If your encoder's clockwise directions are incorrect, swap the A & B pad definitions or use C code configuration with `#define ENCODER_DIRECTION_FLIP`.
:::

Configure the resolution (pulses between each detent):

```json
{
    "encoder": {
        "rotary": [
            {"pin_a": "B12", "pin_b": "B13", "resolution": 4}
        ]
    }
}
```

For different resolutions per encoder:

```json
{
    "encoder": {
        "rotary": [
            {"pin_a": "B6", "pin_b": "B2", "resolution": 4},
            {"pin_a": "B3", "pin_b": "B1", "resolution": 2}
        ]
    }
}
```

::: details Legacy config.h Configuration
```c
#define ENCODER_A_PINS { encoder1a, encoder2a }
#define ENCODER_B_PINS { encoder1b, encoder2b }
#define ENCODER_DIRECTION_FLIP
#define ENCODER_RESOLUTION 4
#define ENCODER_RESOLUTIONS { 4, 2 }
#define ENCODER_DEFAULT_POS 0x3
```
:::

## Split Keyboards

### Modern Configuration

Configure different encoders for each half of a split keyboard:

```json
{
    "encoder": {
        "rotary": [
            {"pin_a": "B6", "pin_b": "B2", "resolution": 4}
        ]
    },
    "split": {
        "encoder": {
            "right": {
                "rotary": [
                    {"pin_a": "B3", "pin_b": "B1", "resolution": 2}
                ]
            }
        }
    }
}
```

For a split keyboard with only a right-side encoder:

```json
{
    "encoder": {
        "rotary": []
    },
    "split": {
        "encoder": {
            "right": {
                "rotary": [
                    {"pin_a": "B12", "pin_b": "B13", "resolution": 4}
                ]
            }
        }
    }
}
```

::: details Legacy config.h Configuration
```c
#define ENCODER_A_PINS_RIGHT { encoder1a, encoder2a }
#define ENCODER_B_PINS_RIGHT { encoder1b, encoder2b }
#define ENCODER_RESOLUTIONS_RIGHT { 2, 4 }
```

For a split keyboard with only a right-side encoder:

```c
#define ENCODER_A_PINS { }
#define ENCODER_B_PINS { }
#define ENCODER_RESOLUTIONS { }
#define ENCODER_A_PINS_RIGHT { B12 }
#define ENCODER_B_PINS_RIGHT { B13 }
#define ENCODER_RESOLUTIONS_RIGHT { 4 }
```
:::

::: warning
Keep in mind that whenver you change the encoder resolution, you will need to reflash the half that has the encoder affected by the change.
:::

## Encoder map {#encoder-map}

Encoder mapping may be added to your `keymap.c`, which replicates the normal keyswitch layer handling functionality, but with encoders.

### Modern Configuration

Enable encoder map in your `keymap.json`:

```json
{
    "features": {
        "encoder_map": true
    }
}
```

::: details Legacy rules.mk Configuration
Add this to your keymap's `rules.mk`:

```make
ENCODER_MAP_ENABLE = yes
```
:::

Your `keymap.c` will then need an encoder mapping defined (for four layers and two encoders):

```c
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(MS_WHLU, MS_WHLD),  ENCODER_CCW_CW(KC_VOLD, KC_VOLU)  },
    [1] = { ENCODER_CCW_CW(UG_HUED, UG_HUEU),  ENCODER_CCW_CW(UG_SATD, UG_SATU)  },
    [2] = { ENCODER_CCW_CW(UG_VALD, UG_VALU),  ENCODER_CCW_CW(UG_SPDD, UG_SPDU)  },
    [3] = { ENCODER_CCW_CW(UG_PREV, UG_NEXT),  ENCODER_CCW_CW(KC_RIGHT, KC_LEFT) },
};
#endif
```

::: tip
This should only be enabled at the keymap level.
:::

Using encoder mapping pumps events through the normal QMK keycode processing pipeline, resulting in a _keydown/keyup_ combination pushed through `process_record_xxxxx()`.

::: warning
Encoder map key delay configuration is not currently available in JSON format and requires C code configuration using `#define ENCODER_MAP_KEY_DELAY`.
:::

::: details Legacy config.h Configuration
To configure the amount of time between the encoder "keyup" and "keydown", you can add the following to your `config.h`:

```c
#define ENCODER_MAP_KEY_DELAY 10
```
:::

::: tip
By default, the encoder map delay matches the value of `TAP_CODE_DELAY`.
:::

## Callbacks

::: tip
[**Default Behaviour**](https://github.com/qmk/qmk_firmware/blob/master/quantum/encoder.c#L79-): all encoders installed will function as volume up (`KC_VOLU`) on clockwise rotation and volume down (`KC_VOLD`) on counter-clockwise rotation. If you do not wish to override this, no further configuration is necessary.
:::

If you would like the alter the default behaviour, and are not using `ENCODER_MAP_ENABLE = yes`, the callback functions can be inserted into your `<keyboard>.c`:

```c
bool encoder_update_kb(uint8_t index, bool clockwise) {
    if (!encoder_update_user(index, clockwise)) {
      return false; /* Don't process further events if user function exists and returns false */
    }
    if (index == 0) { /* First encoder */
        if (clockwise) {
            tap_code(KC_PGDN);
        } else {
            tap_code(KC_PGUP);
        }
    } else if (index == 1) { /* Second encoder */
        if (clockwise) {
            rgb_matrix_increase_hue();
        } else {
            rgb_matrix_decrease_hue();
        }
    }
    return true;
}
```

or `keymap.c`:

```c
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) { /* First encoder */
        if (clockwise) {
            tap_code(KC_PGDN);
        } else {
            tap_code(KC_PGUP);
        }
    } else if (index == 1) { /* Second encoder */
        if (clockwise) {
            rgb_matrix_increase_hue();
        } else {
            rgb_matrix_decrease_hue();
        }
    }
    return false;
}
```

::: warning
If you return `true` in the keymap level `_user` function, it will allow the keyboard/core level encoder code to run on top of your own. Returning `false` will override the keyboard level function, if setup correctly. This is generally the safest option to avoid confusion.
:::

## Hardware

The A an B lines of the encoders should be wired directly to the MCU, and the C/common lines should be wired to ground.

## Multiple Encoders

Multiple encoders may share pins so long as each encoder has a distinct pair of pins when the following conditions are met:
- using detent encoders
- pads must be high at the detent stability point which is called 'default position' in QMK
- no more than two encoders sharing a pin can be turned at the same time

For example you can support two encoders using only 3 pins like this
```
#define ENCODER_A_PINS { B1, B1 }
#define ENCODER_B_PINS { B2, B3 }
```

You could even support three encoders using only three pins (one per encoder) however in this configuration, rotating two encoders which share pins simultaneously will often generate incorrect output. For example:
```
#define ENCODER_A_PINS { B1, B1, B2 }
#define ENCODER_B_PINS { B2, B3, B3 }
```
Here rotating Encoder 0 `B1 B2` and Encoder 1 `B1 B3` could be interpreted as rotating Encoder 2 `B2 B3` or `B3 B2` depending on the timing. This may still be a useful configuration depending on your use case
