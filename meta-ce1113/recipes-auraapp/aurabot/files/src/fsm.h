#ifndef AURABOT_FSM_H
#define AURABOT_FSM_H

#include "aurabot_state.h"

void fsm_initialize(aurabot_state_t *state);
int fsm_transition_mode(aurabot_state_t *state, aurabot_mode_t mode);
int fsm_transition_auto(aurabot_state_t *state,
                        aurabot_auto_state_t auto_state);
void fsm_enter_safe_stop(aurabot_state_t *state);

#endif
