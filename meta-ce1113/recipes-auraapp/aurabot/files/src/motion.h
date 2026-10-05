#ifndef AURABOT_MOTION_H
#define AURABOT_MOTION_H

#include "aurabot_state.h"

int motion_drive(aurabot_state_t *state, int left_percent,
                 int right_percent);
int motion_stop(aurabot_state_t *state);

#endif
