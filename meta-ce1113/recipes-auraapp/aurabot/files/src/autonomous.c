#include <stddef.h>

#include "audio_controller.h"
#include "aurabot_config.h"
#include "autonomous.h"
#include "fsm.h"
#include "motion.h"
#include "time_utils.h"

static int absolute_value(int value)
{
    return value < 0 ? -value : value;
}

static void start_maneuver_timer(aurabot_state_t *state)
{
    clock_gettime(CLOCK_MONOTONIC, &state->maneuver_started);
}

int autonomous_enter(aurabot_state_t *state)
{
    int result;
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    result = fsm_transition_auto(state, AURABOT_AUTO_FORWARD);
    if (result != AURABOT_OK) return result;
    state->maneuver_distance_mm = 0;
    state->maneuver_angle_mrad = 0;
    start_maneuver_timer(state);
    return motion_drive(state, AURABOT_AUTO_FORWARD_SPEED,
                        AURABOT_AUTO_FORWARD_SPEED);
}

static int maneuver_timed_out(const aurabot_state_t *state)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return time_elapsed_ms(&state->maneuver_started, &now) >
           AURABOT_AUTO_MANEUVER_TIMEOUT_MS;
}

static int begin_reverse(aurabot_state_t *state)
{
    int result = motion_stop(state);
    if (result != AURABOT_OK) return result;
    audio_controller_event("obstacle");
    state->turn_direction = state->public_status.left_obstacle &&
        !state->public_status.right_obstacle ? -1 : 1;
    state->maneuver_distance_mm = 0;
    start_maneuver_timer(state);
    result = fsm_transition_auto(state, AURABOT_AUTO_REVERSE);
    if (result != AURABOT_OK) return result;
    return motion_drive(state, -AURABOT_AUTO_REVERSE_SPEED,
                        -AURABOT_AUTO_REVERSE_SPEED);
}

static int begin_turn(aurabot_state_t *state)
{
    int result = motion_stop(state);
    if (result != AURABOT_OK) return result;
    state->maneuver_angle_mrad = 0;
    start_maneuver_timer(state);
    result = fsm_transition_auto(state, AURABOT_AUTO_TURN);
    if (result != AURABOT_OK) return result;
    if (state->turn_direction > 0)
        return motion_drive(state, -AURABOT_AUTO_TURN_SPEED,
                            AURABOT_AUTO_TURN_SPEED);
    return motion_drive(state, AURABOT_AUTO_TURN_SPEED,
                        -AURABOT_AUTO_TURN_SPEED);
}

static int resume_forward(aurabot_state_t *state)
{
    int result = motion_stop(state);
    if (result != AURABOT_OK) return result;
    result = fsm_transition_auto(state, AURABOT_AUTO_FORWARD);
    if (result != AURABOT_OK) return result;
    return motion_drive(state, AURABOT_AUTO_FORWARD_SPEED,
                        AURABOT_AUTO_FORWARD_SPEED);
}

int autonomous_tick(aurabot_state_t *state)
{
    aurabot_status_t *status;
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    status = &state->public_status;

    if ((status->auto_state == AURABOT_AUTO_REVERSE ||
         status->auto_state == AURABOT_AUTO_TURN) &&
        maneuver_timed_out(state))
        return AURABOT_ERR_HARDWARE;

    switch (status->auto_state) {
    case AURABOT_AUTO_FORWARD:
        if (!status->left_obstacle && !status->right_obstacle)
            return AURABOT_OK;
        return begin_reverse(state);
    case AURABOT_AUTO_REVERSE:
        if (absolute_value(state->maneuver_distance_mm) <
            AURABOT_AUTO_REVERSE_DISTANCE_MM)
            return AURABOT_OK;
        return begin_turn(state);
    case AURABOT_AUTO_TURN:
        if (absolute_value(state->maneuver_angle_mrad) <
            AURABOT_AUTO_TURN_ANGLE_MRAD)
            return AURABOT_OK;
        return resume_forward(state);
    default:
        return AURABOT_ERR_PROTOCOL;
    }
}
