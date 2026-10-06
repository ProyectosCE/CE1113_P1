#include <stddef.h>

#include "hardware.h"
#include "state_outputs.h"

int state_outputs_apply(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    state->public_status.power_led =
        state->public_status.mode != AURABOT_MODE_INIT;
    state->public_status.manual_led =
        state->public_status.mode == AURABOT_MODE_MANUAL;
    state->public_status.autonomous_led =
        state->public_status.mode == AURABOT_MODE_AUTONOMOUS;
    state->public_status.obstacle_led =
        state->public_status.left_obstacle ||
        state->public_status.right_obstacle;
    return hardware_set_leds(
        state->public_status.power_led,
        state->public_status.manual_led,
        state->public_status.autonomous_led,
        state->public_status.obstacle_led) == 0
        ? AURABOT_OK : AURABOT_ERR_HARDWARE;
}

void state_outputs_disable(void)
{
    (void)hardware_set_leds(0, 0, 0, 0);
}
