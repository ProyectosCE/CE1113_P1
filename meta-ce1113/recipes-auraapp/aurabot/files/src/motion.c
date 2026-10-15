#include <stddef.h>

#include "hardware.h"
#include "motion.h"

int motion_drive(aurabot_state_t *state, int left_percent,
                 int right_percent)
{
    if (state == NULL || left_percent < -100 || left_percent > 100 ||
        right_percent < -100 || right_percent > 100)
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_drive(left_percent, right_percent) != 0)
        return AURABOT_ERR_HARDWARE;
    state->public_status.left_speed = hardware_left_motor_enabled() ? left_percent : 0;
    state->public_status.right_speed = hardware_right_motor_enabled() ? right_percent : 0;
    clock_gettime(CLOCK_MONOTONIC, &state->motion_last_tick);
    return AURABOT_OK;
}

int motion_stop(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_stop() != 0) return AURABOT_ERR_HARDWARE;
    state->public_status.left_speed = 0;
    state->public_status.right_speed = 0;
    return AURABOT_OK;
}
