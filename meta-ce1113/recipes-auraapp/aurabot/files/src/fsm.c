#include <string.h>

#include "autonomous.h"
#include "fsm.h"
#include "hardware.h"

#define OWNER_LEASE_MS 3000
#define MOTION_STALL_MS 1000
#define HALF_TURN_MRAD 3142
#define FULL_TURN_MRAD 6283

static const int cosine_table[24] = {
    1000, 966, 866, 707, 500, 259, 0, -259, -500, -707, -866, -966,
    -1000, -966, -866, -707, -500, -259, 0, 259, 500, 707, 866, 966
};
static const int sine_table[24] = {
    0, 259, 500, 707, 866, 966, 1000, 966, 866, 707, 500, 259,
    0, -259, -500, -707, -866, -966, -1000, -966, -866, -707, -500, -259
};

static long long elapsed_ms(const struct timespec *start,
                            const struct timespec *end)
{
    return (long long)(end->tv_sec - start->tv_sec) * 1000LL +
           (end->tv_nsec - start->tv_nsec) / 1000000LL;
}

static int token_is_zero(const unsigned char *token)
{
    unsigned int index;
    unsigned char value = 0;
    for (index = 0; index < AURABOT_OWNER_TOKEN_SIZE; ++index)
        value |= token[index];
    return value == 0;
}

static int token_matches(const aurabot_state_t *state,
                         const unsigned char *token)
{
    unsigned int index;
    unsigned char difference = 0;
    if (!state->owner_active || token == NULL) return 0;
    for (index = 0; index < AURABOT_OWNER_TOKEN_SIZE; ++index)
        difference |= state->owner_token[index] ^ token[index];
    return difference == 0;
}

static void clear_owner(aurabot_state_t *state)
{
    memset(state->owner_token, 0, sizeof(state->owner_token));
    state->owner_active = 0;
    state->public_status.manual_control_busy = 0;
}

static int normalize_heading(int heading)
{
    while (heading > HALF_TURN_MRAD) heading -= FULL_TURN_MRAD;
    while (heading <= -HALF_TURN_MRAD) heading += FULL_TURN_MRAD;
    return heading;
}

static int direction_index(int heading)
{
    int normalized = normalize_heading(heading);
    if (normalized < 0) normalized += FULL_TURN_MRAD;
    return ((normalized + 131) / 262) % 24;
}

static int apply_leds(aurabot_state_t *state)
{
    int obstacle = state->public_status.left_obstacle ||
                   state->public_status.right_obstacle;
    return hardware_set_leds(
        state->public_status.mode != AURABOT_MODE_INIT,
        state->public_status.mode == AURABOT_MODE_MANUAL,
        state->public_status.mode == AURABOT_MODE_AUTONOMOUS,
        obstacle);
}

static int enter_safe_stop(aurabot_state_t *state)
{
    (void)hardware_stop();
    clear_owner(state);
    state->public_status.left_speed = 0;
    state->public_status.right_speed = 0;
    state->public_status.mode = AURABOT_MODE_SAFE_STOP;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
    (void)apply_leds(state);
    return AURABOT_ERR_HARDWARE;
}

static void update_odometry(aurabot_state_t *state,
                            const encoder_snapshot_t *encoders)
{
    long long left_numerator;
    long long right_numerator;
    int left_distance;
    int right_distance;
    int center_distance;
    int delta_heading;
    int direction;

    left_numerator = state->left_distance_remainder +
        (long long)encoders->left_direction * encoders->left_ticks *
        hardware_left_wheel_circumference_mm();
    right_numerator = state->right_distance_remainder +
        (long long)encoders->right_direction * encoders->right_ticks *
        hardware_right_wheel_circumference_mm();
    left_distance = (int)(left_numerator /
        hardware_left_ticks_per_revolution());
    right_distance = (int)(right_numerator /
        hardware_right_ticks_per_revolution());
    state->left_distance_remainder = (int)(left_numerator %
        hardware_left_ticks_per_revolution());
    state->right_distance_remainder = (int)(right_numerator %
        hardware_right_ticks_per_revolution());

    center_distance = (left_distance + right_distance) / 2;
    delta_heading = (right_distance - left_distance) * 1000 /
        (int)hardware_wheel_base_mm();
    state->public_status.heading_mrad = normalize_heading(
        state->public_status.heading_mrad + delta_heading);
    direction = direction_index(state->public_status.heading_mrad);
    state->public_status.x_mm += center_distance * cosine_table[direction] / 1000;
    state->public_status.y_mm += center_distance * sine_table[direction] / 1000;
    state->maneuver_distance_mm += center_distance;
    state->maneuver_angle_mrad += delta_heading;
}

int fsm_init(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    memset(state, 0, sizeof(*state));
    state->public_status.mode = AURABOT_MODE_INIT;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
    state->public_status.audio_state = AURABOT_AUDIO_STOPPED;
    state->public_status.audio_volume_percent = 50;
    state->public_status.audio_track_index = -1;
    map_init(&state->map);
    if (hardware_init() != 0) {
        state->public_status.mode = AURABOT_MODE_SAFE_STOP;
        return AURABOT_ERR_HARDWARE;
    }
    clock_gettime(CLOCK_MONOTONIC, &state->last_tick);
    state->motion_last_tick = state->last_tick;
    state->public_status.mode = AURABOT_MODE_MANUAL;
    state->initialized = 1;
    (void)hardware_audio_event("boot");
    return apply_leds(state) == 0 ? AURABOT_OK : enter_safe_stop(state);
}

void fsm_shutdown(aurabot_state_t *state)
{
    if (state == NULL) return;
    (void)hardware_stop();
    (void)hardware_set_leds(0, 0, 0, 0);
    hardware_shutdown();
    state->initialized = 0;
}

int fsm_tick(aurabot_state_t *state)
{
    struct timespec now;
    sensor_snapshot_t sensors;
    encoder_snapshot_t encoders;
    int moving;
    int result;

    if (state == NULL || !state->initialized) return AURABOT_ERR_UNAVAILABLE;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (hardware_read_sensors(&sensors) != 0 ||
        hardware_take_encoder_sample(&encoders) != 0)
        return enter_safe_stop(state);
    state->public_status.left_obstacle = sensors.left_obstacle;
    state->public_status.right_obstacle = sensors.right_obstacle;
    update_odometry(state, &encoders);
    map_update_pose(&state->map, state->public_status.x_mm,
                    state->public_status.y_mm,
                    state->public_status.heading_mrad,
                    sensors.left_obstacle, sensors.right_obstacle);
    state->public_status.map_revision = state->map.revision;

    if (state->owner_active &&
        elapsed_ms(&state->owner_last_heartbeat, &now) > OWNER_LEASE_MS) {
        (void)hardware_stop();
        state->public_status.left_speed = 0;
        state->public_status.right_speed = 0;
        clear_owner(state);
    }

    moving = state->public_status.left_speed != 0 ||
             state->public_status.right_speed != 0;
    if (encoders.left_ticks > 0U || encoders.right_ticks > 0U)
        state->motion_last_tick = now;
    else if (moving && elapsed_ms(&state->motion_last_tick, &now) >
             MOTION_STALL_MS)
        return enter_safe_stop(state);

    if (state->public_status.mode == AURABOT_MODE_AUTONOMOUS) {
        result = autonomous_tick(state);
        if (result != AURABOT_OK) return enter_safe_stop(state);
    }
    state->last_tick = now;
    return apply_leds(state) == 0 ? AURABOT_OK : AURABOT_ERR_HARDWARE;
}

int fsm_set_mode(aurabot_state_t *state, aurabot_mode_t mode)
{
    if (state == NULL || (mode != AURABOT_MODE_MANUAL &&
        mode != AURABOT_MODE_AUTONOMOUS))
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_stop() != 0) return enter_safe_stop(state);
    state->public_status.left_speed = 0;
    state->public_status.right_speed = 0;
    clear_owner(state);
    state->public_status.mode = mode;
    state->public_status.auto_state = AURABOT_AUTO_INACTIVE;
    clock_gettime(CLOCK_MONOTONIC, &state->motion_last_tick);
    if (mode == AURABOT_MODE_AUTONOMOUS) {
        if (autonomous_enter(state) != AURABOT_OK)
            return enter_safe_stop(state);
        state->public_status.left_speed = 45;
        state->public_status.right_speed = 45;
        (void)hardware_audio_event("autonomous");
    } else {
        (void)hardware_audio_event("manual");
    }
    (void)apply_leds(state);
    return AURABOT_OK;
}

int fsm_claim_control(aurabot_state_t *state, const unsigned char *token)
{
    if (state == NULL || token == NULL || token_is_zero(token))
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (state->owner_active && !token_matches(state, token))
        return AURABOT_ERR_BUSY;
    memcpy(state->owner_token, token, AURABOT_OWNER_TOKEN_SIZE);
    state->owner_active = 1;
    state->public_status.manual_control_busy = 1;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    return AURABOT_OK;
}

int fsm_control_heartbeat(aurabot_state_t *state, const unsigned char *token)
{
    if (!token_matches(state, token)) return AURABOT_ERR_NOT_OWNER;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    return AURABOT_OK;
}

int fsm_release_control(aurabot_state_t *state, const unsigned char *token)
{
    if (!token_matches(state, token)) return AURABOT_ERR_NOT_OWNER;
    (void)hardware_stop();
    state->public_status.left_speed = 0;
    state->public_status.right_speed = 0;
    clear_owner(state);
    return AURABOT_OK;
}

int fsm_drive(aurabot_state_t *state, const unsigned char *token,
              int left_percent, int right_percent)
{
    if (state == NULL || left_percent < -100 || left_percent > 100 ||
        right_percent < -100 || right_percent > 100)
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (!token_matches(state, token)) return AURABOT_ERR_NOT_OWNER;
    if (hardware_drive(left_percent, right_percent) != 0)
        return enter_safe_stop(state);
    state->public_status.left_speed = left_percent;
    state->public_status.right_speed = right_percent;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    state->motion_last_tick = state->owner_last_heartbeat;
    return AURABOT_OK;
}

int fsm_stop(aurabot_state_t *state, const unsigned char *token)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (!token_matches(state, token)) return AURABOT_ERR_NOT_OWNER;
    if (hardware_stop() != 0) return enter_safe_stop(state);
    state->public_status.left_speed = 0;
    state->public_status.right_speed = 0;
    clock_gettime(CLOCK_MONOTONIC, &state->owner_last_heartbeat);
    return AURABOT_OK;
}

int fsm_emergency_stop(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    (void)enter_safe_stop(state);
    return AURABOT_OK;
}
