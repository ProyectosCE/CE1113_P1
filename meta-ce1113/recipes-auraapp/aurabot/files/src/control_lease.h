#ifndef AURABOT_CONTROL_LEASE_H
#define AURABOT_CONTROL_LEASE_H

#include "aurabot_state.h"

int control_lease_claim(aurabot_state_t *state, const unsigned char *token);
int control_lease_heartbeat(aurabot_state_t *state,
                            const unsigned char *token);
int control_lease_release(aurabot_state_t *state,
                          const unsigned char *token);
int control_lease_is_owner(const aurabot_state_t *state,
                           const unsigned char *token);
int control_lease_has_expired(const aurabot_state_t *state,
                              const struct timespec *now);
void control_lease_clear(aurabot_state_t *state);

#endif
