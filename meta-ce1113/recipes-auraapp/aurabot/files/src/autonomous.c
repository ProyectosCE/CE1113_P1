#include "autonomous.h"
#include "hardware.h"

#define AUTO_FORWARD_SPEED 45
#define AUTO_REVERSE_SPEED 35
#define AUTO_TURN_SPEED 35
#define AUTO_REVERSE_DISTANCE_MM 80
#define AUTO_TURN_ANGLE_MRAD 262
#define AUTO_MANEUVER_TIMEOUT_MS 3000

static int absolute_value(int value)
{
    return value < 0 ? -value : value;
}

static long long elapsed_ms(const struct timespec *start,
                            const struct timespec *end)
{
    return (long long)(end->tv_sec - start->tv_sec) * 1000LL +
           (end->tv_nsec - start->tv_nsec) / 1000000LL;
}

int autonomous_enter(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    state->public_status.auto_state = AURABOT_AUTO_FORWARD;
    state->maneuver_distance_mm = 0;
    state->maneuver_angle_mrad = 0;
    state->previous_heading_mrad = state->public_status.heading_mrad;
    clock_gettime(CLOCK_MONOTONIC, &state->maneuver_started);
    return hardware_drive(AUTO_FORWARD_SPEED, AUTO_FORWARD_SPEED) == 0
        ? AURABOT_OK : AURABOT_ERR_HARDWARE;
}

int autonomous_tick(aurabot_state_t *state)
{
    aurabot_status_t *status;
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    status = &state->public_status;

    if (status->auto_state == AURABOT_AUTO_REVERSE ||
        status->auto_state == AURABOT_AUTO_TURN) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (elapsed_ms(&state->maneuver_started, &now) >
            AUTO_MANEUVER_TIMEOUT_MS)
            return AURABOT_ERR_HARDWARE;
    }

    switch (status->auto_state) {
    case AURABOT_AUTO_FORWARD:
        if (!status->left_obstacle && !status->right_obstacle)
            return AURABOT_OK;
        if (hardware_stop() != 0) return AURABOT_ERR_HARDWARE;
        (void)hardware_audio_event("obstacle");
        state->turn_direction = status->left_obstacle && !status->right_obstacle
            ? -1 : 1;
        state->maneuver_distance_mm = 0;
        clock_gettime(CLOCK_MONOTONIC, &state->maneuver_started);
        status->auto_state = AURABOT_AUTO_REVERSE;
        if (hardware_drive(-AUTO_REVERSE_SPEED, -AUTO_REVERSE_SPEED) != 0)
            return AURABOT_ERR_HARDWARE;
        status->left_speed = -AUTO_REVERSE_SPEED;
        status->right_speed = -AUTO_REVERSE_SPEED;
        break;
    case AURABOT_AUTO_REVERSE:
        if (absolute_value(state->maneuver_distance_mm) < AUTO_REVERSE_DISTANCE_MM)
            return AURABOT_OK;
        if (hardware_stop() != 0) return AURABOT_ERR_HARDWARE;
        state->maneuver_angle_mrad = 0;
        state->previous_heading_mrad = status->heading_mrad;
        clock_gettime(CLOCK_MONOTONIC, &state->maneuver_started);
        status->auto_state = AURABOT_AUTO_TURN;
        if (state->turn_direction > 0) {
            if (hardware_drive(-AUTO_TURN_SPEED, AUTO_TURN_SPEED) != 0)
                return AURABOT_ERR_HARDWARE;
            status->left_speed = -AUTO_TURN_SPEED;
            status->right_speed = AUTO_TURN_SPEED;
        } else if (hardware_drive(AUTO_TURN_SPEED, -AUTO_TURN_SPEED) != 0) {
            return AURABOT_ERR_HARDWARE;
        } else {
            status->left_speed = AUTO_TURN_SPEED;
            status->right_speed = -AUTO_TURN_SPEED;
        }
        break;
    case AURABOT_AUTO_TURN:
        if (absolute_value(state->maneuver_angle_mrad) < AUTO_TURN_ANGLE_MRAD)
            return AURABOT_OK;
        if (hardware_stop() != 0) return AURABOT_ERR_HARDWARE;
        status->auto_state = AURABOT_AUTO_FORWARD;
        if (hardware_drive(AUTO_FORWARD_SPEED, AUTO_FORWARD_SPEED) != 0)
            return AURABOT_ERR_HARDWARE;
        status->left_speed = AUTO_FORWARD_SPEED;
        status->right_speed = AUTO_FORWARD_SPEED;
        break;
    default:
        return AURABOT_ERR_PROTOCOL;
    }
    return AURABOT_OK;
}
