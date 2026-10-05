#ifndef AURABOT_AUTONOMOUS_H
#define AURABOT_AUTONOMOUS_H

#include "aurabot_state.h"

int autonomous_enter(aurabot_state_t *state);
int autonomous_tick(aurabot_state_t *state);

#endif
