#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fsm.h"

int main(void)
{
    aurabot_state_t state;

    memset(&state, 0, sizeof(state));
    fsm_initialize(&state);
    assert(state.public_status.mode == AURABOT_MODE_INIT);
    assert(state.public_status.auto_state == AURABOT_AUTO_INACTIVE);

    assert(fsm_transition_mode(&state, AURABOT_MODE_MANUAL) == AURABOT_OK);
    assert(fsm_transition_auto(&state, AURABOT_AUTO_FORWARD) ==
           AURABOT_ERR_WRONG_MODE);
    assert(fsm_transition_mode(&state, AURABOT_MODE_AUTONOMOUS) ==
           AURABOT_OK);
    assert(fsm_transition_auto(&state, AURABOT_AUTO_FORWARD) == AURABOT_OK);
    assert(fsm_transition_auto(&state, AURABOT_AUTO_REVERSE) == AURABOT_OK);
    assert(fsm_transition_auto(&state, AURABOT_AUTO_TURN) == AURABOT_OK);
    assert(fsm_transition_auto(&state, AURABOT_AUTO_INACTIVE) ==
           AURABOT_ERR_INVALID_ARGUMENT);

    fsm_enter_safe_stop(&state);
    assert(state.public_status.mode == AURABOT_MODE_SAFE_STOP);
    assert(state.public_status.auto_state == AURABOT_AUTO_INACTIVE);
    puts("fsm tests passed");
    return 0;
}
