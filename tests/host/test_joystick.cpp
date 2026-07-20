// Copyright 2026 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later
#include "test_common.hpp"

using testing::_;
using testing::InSequence;

namespace {
MATCHER_P(JoystickButton0StateEq, expected_state, "has buttons[0] equal to " + testing::PrintToString(expected_state)) {
    return arg.buttons[0] == expected_state;
}
} // namespace

class JoystickHost : public TestFixture {};

TEST_F(JoystickHost, JoystickButtonKeycodeReportsThroughActiveDriver) {
    TestDriver driver;
    KeymapKey  key_js0(0, 0, 0, JS_0);
    set_keymap({key_js0});

    {
        InSequence s;
        EXPECT_CALL(driver, send_joystick_mock(JoystickButton0StateEq(0x01))); // press sets the button bit
        EXPECT_CALL(driver, send_joystick_mock(JoystickButton0StateEq(0x00))); // release clears it
    }
    tap_key(key_js0);

    VERIFY_AND_CLEAR(driver);
}

TEST_F(JoystickHost, NullDriverMemberDropsJoystickReport) {
    TestDriver driver;
    driver.clear_send_joystick(); // NULL member: channel unsupported by this transport
    KeymapKey key_js0(0, 0, 0, JS_0);
    set_keymap({key_js0});

    EXPECT_CALL(driver, send_joystick_mock(_)).Times(0);
    tap_key(key_js0); // no report reaches a driver without the channel

    VERIFY_AND_CLEAR(driver);
}
