#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fake_hardware.h"
#include "fsm.h"

static void tick_for(aurabot_state_t *state, int milliseconds)
{
    struct timespec delay = { .tv_sec = 0, .tv_nsec = 25000000L };
    int elapsed;
    for (elapsed = 0; elapsed < milliseconds; elapsed += 25) {
        nanosleep(&delay, NULL);
        assert(fsm_tick(state) == AURABOT_OK);
    }
}

int main(void)
{
    aurabot_state_t state;
    unsigned char owner[AURABOT_OWNER_TOKEN_SIZE] = { 1 };
    unsigned char other[AURABOT_OWNER_TOKEN_SIZE] = { 2 };
    int left;
    int right;

    assert(fsm_init(&state) == AURABOT_OK);
    assert(state.public_status.mode == AURABOT_MODE_MANUAL);
    assert(state.public_status.auto_state == AURABOT_AUTO_INACTIVE);
    assert(fsm_claim_control(&state, owner) == AURABOT_OK);
    assert(fsm_claim_control(&state, other) == AURABOT_ERR_BUSY);
    assert(fsm_drive(&state, other, 50, 50) == AURABOT_ERR_NOT_OWNER);
    assert(fsm_drive(&state, owner, 50, 50) == AURABOT_OK);
    tick_for(&state, 250);
    assert(state.public_status.x_mm > 0);
    assert(fsm_release_control(&state, owner) == AURABOT_OK);

    fake_hardware_set_obstacles(1, 0);
    assert(fsm_set_mode(&state, AURABOT_MODE_AUTONOMOUS) == AURABOT_OK);
    assert(fsm_tick(&state) == AURABOT_OK);
    assert(state.public_status.auto_state == AURABOT_AUTO_REVERSE);
    fake_hardware_get_motor_speeds(&left, &right);
    assert(left < 0 && right < 0);
    fake_hardware_set_obstacles(0, 0);
    tick_for(&state, 700);
    assert(state.public_status.auto_state == AURABOT_AUTO_FORWARD);
    assert(state.public_status.heading_mrad < 0);

    assert(fsm_emergency_stop(&state) == AURABOT_OK);
    assert(state.public_status.mode == AURABOT_MODE_SAFE_STOP);
    fake_hardware_get_motor_speeds(&left, &right);
    assert(left == 0 && right == 0);
    fsm_shutdown(&state);
    puts("core tests passed");
    return 0;
}
