#include <stddef.h>

#include "fsm.h"

void fsm_initialize(aurabot_state_t *state)
{
    if (state == NULL) return;
    state->public_status.mode = AURABOT_MODE_INIT;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
}

int fsm_transition_mode(aurabot_state_t *state, aurabot_mode_t mode)
{
    if (state == NULL || (mode != AURABOT_MODE_MANUAL &&
        mode != AURABOT_MODE_AUTONOMOUS))
        return AURABOT_ERR_INVALID_ARGUMENT;
    state->public_status.mode = mode;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
    return AURABOT_OK;
}

int fsm_transition_auto(aurabot_state_t *state,
                        aurabot_auto_state_t auto_state)
{
    if (state == NULL || state->public_status.mode != AURABOT_MODE_AUTONOMOUS)
        return AURABOT_ERR_WRONG_MODE;
    if (auto_state != AURABOT_AUTO_FORWARD &&
        auto_state != AURABOT_AUTO_REVERSE &&
        auto_state != AURABOT_AUTO_TURN)
        return AURABOT_ERR_INVALID_ARGUMENT;
    state->public_status.auto_state = auto_state;
    return AURABOT_OK;
}

void fsm_enter_safe_stop(aurabot_state_t *state)
{
    if (state == NULL) return;
    state->public_status.mode = AURABOT_MODE_SAFE_STOP;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
}
