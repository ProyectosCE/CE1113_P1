#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fake_hardware.h"
#include "robot_controller.h"

static void tick_for(aurabot_state_t *state, int milliseconds)
{
    struct timespec delay = { .tv_sec = 0, .tv_nsec = 25000000L };
    int elapsed;
    for (elapsed = 0; elapsed < milliseconds; elapsed += 25) {
        nanosleep(&delay, NULL);
        assert(robot_controller_tick(state) == AURABOT_OK);
    }
}

int main(void)
{
    aurabot_state_t state;
    unsigned char owner[AURABOT_OWNER_TOKEN_SIZE] = { 1 };
    unsigned char other[AURABOT_OWNER_TOKEN_SIZE] = { 2 };
    int left;
    int right;
    int power_led, manual_led, auto_led, obstacle_led;

    assert(robot_controller_init(&state) == AURABOT_OK);
    assert(state.public_status.mode == AURABOT_MODE_AUTONOMOUS);
    assert(state.public_status.auto_state == AURABOT_AUTO_FORWARD);
    fake_hardware_get_motor_speeds(&left, &right);
    assert(left > 0 && right > 0);
    fake_hardware_get_leds(&power_led, &manual_led, &auto_led, &obstacle_led);
    assert(power_led == 1 && manual_led == 0 && auto_led == 1 && obstacle_led == 0);
    assert(robot_controller_set_mode(&state, AURABOT_MODE_MANUAL) == AURABOT_OK);
    fake_hardware_get_leds(&power_led, &manual_led, &auto_led, &obstacle_led);
    assert(power_led == 1 && manual_led == 1 && auto_led == 0);
    assert(state.public_status.mode == AURABOT_MODE_MANUAL);
    assert(state.public_status.auto_state == AURABOT_AUTO_INACTIVE);
    assert(robot_controller_claim(&state, owner) == AURABOT_OK);
    assert(robot_controller_claim(&state, other) == AURABOT_ERR_BUSY);
    assert(robot_controller_drive(&state, other, 50, 50) ==
           AURABOT_ERR_NOT_OWNER);
    assert(robot_controller_drive(&state, owner, 50, 50) == AURABOT_OK);
    tick_for(&state, 250);
    assert(state.public_status.x_mm > 0);
    assert(robot_controller_release(&state, owner) == AURABOT_OK);

    fake_hardware_set_obstacles(1, 0);
    assert(robot_controller_set_mode(&state, AURABOT_MODE_AUTONOMOUS) ==
           AURABOT_OK);
    assert(robot_controller_tick(&state) == AURABOT_OK);
    assert(state.public_status.auto_state == AURABOT_AUTO_REVERSE);
    fake_hardware_get_leds(&power_led, &manual_led, &auto_led, &obstacle_led);
    assert(obstacle_led == 1);
    fake_hardware_get_motor_speeds(&left, &right);
    assert(left < 0 && right < 0);
    fake_hardware_set_obstacles(0, 0);
    tick_for(&state, 700);
    assert(state.public_status.auto_state == AURABOT_AUTO_FORWARD);
    assert(state.public_status.heading_mrad < 0);

    assert(robot_controller_emergency_stop(&state) == AURABOT_OK);
    assert(state.public_status.mode == AURABOT_MODE_SAFE_STOP);
    fake_hardware_get_motor_speeds(&left, &right);
    assert(left == 0 && right == 0);
    robot_controller_shutdown(&state);
    puts("core tests passed");
    return 0;
}
