#ifndef AURABOT_ROBOT_CONTROLLER_H
#define AURABOT_ROBOT_CONTROLLER_H

#include "aurabot_state.h"

int robot_controller_init(aurabot_state_t *state);
void robot_controller_shutdown(aurabot_state_t *state);
int robot_controller_tick(aurabot_state_t *state);
int robot_controller_set_mode(aurabot_state_t *state, aurabot_mode_t mode);
int robot_controller_claim(aurabot_state_t *state,
                           const unsigned char *token);
int robot_controller_heartbeat(aurabot_state_t *state,
                               const unsigned char *token);
int robot_controller_release(aurabot_state_t *state,
                             const unsigned char *token);
int robot_controller_drive(aurabot_state_t *state,
                           const unsigned char *token,
                           int left_percent, int right_percent);
int robot_controller_stop(aurabot_state_t *state,
                          const unsigned char *token);
int robot_controller_emergency_stop(aurabot_state_t *state);

#endif
