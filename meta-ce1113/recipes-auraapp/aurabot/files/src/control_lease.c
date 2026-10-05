#include <stddef.h>
#include <string.h>

#include "aurabot_config.h"
#include "control_lease.h"
#include "time_utils.h"

static int token_is_zero(const unsigned char *token)
{
    unsigned int index;
    unsigned char value = 0;
    for (index = 0; index < AURABOT_OWNER_TOKEN_SIZE; ++index)
        value |= token[index];
    return value == 0;
}

int control_lease_is_owner(const aurabot_state_t *state,
                           const unsigned char *token)
{
    unsigned int index;
    unsigned char difference = 0;
    if (state == NULL || !state->owner_active || token == NULL) return 0;
    for (index = 0; index < AURABOT_OWNER_TOKEN_SIZE; ++index)
        difference |= state->owner_token[index] ^ token[index];
    return difference == 0;
}

void control_lease_clear(aurabot_state_t *state)
{
    if (state == NULL) return;
    memset(state->owner_token, 0, sizeof(state->owner_token));
    state->owner_active = 0;
    state->public_status.manual_control_busy = 0;
}

int control_lease_claim(aurabot_state_t *state, const unsigned char *token)
{
    if (state == NULL || token == NULL || token_is_zero(token))
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (state->owner_active && !control_lease_is_owner(state, token))
        return AURABOT_ERR_BUSY;
    memcpy(state->owner_token, token, AURABOT_OWNER_TOKEN_SIZE);
    state->owner_active = 1;
    state->public_status.manual_control_busy = 1;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    return AURABOT_OK;
}

int control_lease_heartbeat(aurabot_state_t *state,
                            const unsigned char *token)
{
    if (!control_lease_is_owner(state, token)) return AURABOT_ERR_NOT_OWNER;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    return AURABOT_OK;
}

int control_lease_release(aurabot_state_t *state,
                          const unsigned char *token)
{
    if (!control_lease_is_owner(state, token)) return AURABOT_ERR_NOT_OWNER;
    control_lease_clear(state);
    return AURABOT_OK;
}

int control_lease_has_expired(const aurabot_state_t *state,
                              const struct timespec *now)
{
    if (state == NULL || now == NULL || !state->owner_active) return 0;
    return time_elapsed_ms(&state->owner_last_heartbeat, now) >
           AURABOT_OWNER_LEASE_MS;
}
