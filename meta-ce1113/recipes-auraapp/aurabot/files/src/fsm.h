#ifndef AURABOT_FSM_H
#define AURABOT_FSM_H

#include "aurabot_state.h"

int fsm_init(aurabot_state_t *state);
void fsm_shutdown(aurabot_state_t *state);
int fsm_tick(aurabot_state_t *state);
int fsm_set_mode(aurabot_state_t *state, aurabot_mode_t mode);
int fsm_claim_control(aurabot_state_t *state, const unsigned char *token);
int fsm_control_heartbeat(aurabot_state_t *state, const unsigned char *token);
int fsm_release_control(aurabot_state_t *state, const unsigned char *token);
int fsm_drive(aurabot_state_t *state, const unsigned char *token,
              int left_percent, int right_percent);
int fsm_stop(aurabot_state_t *state, const unsigned char *token);
int fsm_emergency_stop(aurabot_state_t *state);

#endif
