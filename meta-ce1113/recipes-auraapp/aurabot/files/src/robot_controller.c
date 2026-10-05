#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "audio_controller.h"
#include "aurabot_config.h"
#include "autonomous.h"
#include "control_lease.h"
#include "fsm.h"
#include "hardware.h"
#include "map.h"
#include "motion.h"
#include "odometry.h"
#include "robot_controller.h"
#include "state_outputs.h"
#include "time_utils.h"

static int enter_safe_stop(aurabot_state_t *state)
{
    (void)motion_stop(state);
    control_lease_clear(state);
    fsm_enter_safe_stop(state);
    (void)state_outputs_apply(state);
    return AURABOT_ERR_HARDWARE;
}

static odometry_config_t read_odometry_config(void)
{
    odometry_config_t config;
    config.left_ticks_per_revolution =
        hardware_left_ticks_per_revolution();
    config.right_ticks_per_revolution =
        hardware_right_ticks_per_revolution();
    config.left_wheel_circumference_mm =
        hardware_left_wheel_circumference_mm();
    config.right_wheel_circumference_mm =
        hardware_right_wheel_circumference_mm();
    config.wheel_base_mm = hardware_wheel_base_mm();
    return config;
}

int robot_controller_init(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    memset(state, 0, sizeof(*state));
    fsm_initialize(state);
    state->public_status.audio_state = AURABOT_AUDIO_STOPPED;
    state->public_status.audio_volume_percent = 50;
    state->public_status.audio_track_index = -1;
    map_init(&state->map);
    if (hardware_init() != 0) {
        fsm_enter_safe_stop(state);
        return AURABOT_ERR_HARDWARE;
    }
    clock_gettime(CLOCK_MONOTONIC, &state->last_tick);
    state->motion_last_tick = state->last_tick;
    state->initialized = 1;
    {
        sensor_snapshot_t sensors;
        if (hardware_read_sensors(&sensors) != 0) return enter_safe_stop(state);
        state->public_status.left_obstacle = sensors.left_obstacle;
        state->public_status.right_obstacle = sensors.right_obstacle;
    }
    audio_controller_event("boot");
    return robot_controller_set_mode(state, AURABOT_MODE_AUTONOMOUS);
}

void robot_controller_shutdown(aurabot_state_t *state)
{
    if (state == NULL) return;
    (void)motion_stop(state);
    state_outputs_disable();
    hardware_shutdown();
    state->initialized = 0;
}

static void update_map(aurabot_state_t *state,
                       const sensor_snapshot_t *sensors)
{
    map_update_pose(&state->map, state->public_status.x_mm,
                    state->public_status.y_mm,
                    state->public_status.heading_mrad,
                    sensors->left_obstacle, sensors->right_obstacle);
    state->public_status.map_revision = state->map.revision;
}

static int motion_is_stalled(const aurabot_state_t *state,
                             const encoder_snapshot_t *encoders,
                             const struct timespec *now)
{
    int moving = (hardware_left_feedback_enabled() && state->public_status.left_speed != 0) ||
                 (hardware_right_feedback_enabled() && state->public_status.right_speed != 0);
    if (!moving || encoders->left_ticks > 0U || encoders->right_ticks > 0U)
        return 0;
    return time_elapsed_ms(&state->motion_last_tick, now) >
           AURABOT_MOTION_STALL_MS;
}

int robot_controller_tick(aurabot_state_t *state)
{
    struct timespec now;
    sensor_snapshot_t sensors;
    encoder_snapshot_t encoders;
    odometry_config_t odometry_config;
    int result;

    if (state == NULL || !state->initialized) return AURABOT_ERR_UNAVAILABLE;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (hardware_read_sensors(&sensors) != 0 ||
        hardware_take_encoder_sample(&encoders) != 0){
        perror("aurabot: lectura de sensores o encoders");
        return enter_safe_stop(state);
    }
    state->public_status.left_obstacle = sensors.left_obstacle;
    state->public_status.right_obstacle = sensors.right_obstacle;
    state->public_status.left_motor_movement = encoders.left_movement;
    state->public_status.right_motor_movement = encoders.right_movement;
    state->public_status.left_motor_direction = encoders.left_direction;
    state->public_status.right_motor_direction = encoders.right_direction;
    odometry_config = read_odometry_config();
    if (odometry_update(state, &encoders, &odometry_config) != AURABOT_OK)
        return enter_safe_stop(state);
    update_map(state, &sensors);

    if (control_lease_has_expired(state, &now)) {
        if (motion_stop(state) != AURABOT_OK) return enter_safe_stop(state);
        control_lease_clear(state);
    }

    if (encoders.left_ticks > 0U || encoders.right_ticks > 0U)
        state->motion_last_tick = now;
    else if (motion_is_stalled(state, &encoders, &now)) {
        fprintf(stderr, "aurabot: motores ordenados pero sin ticks; parada segura\n");
        return enter_safe_stop(state);
    }

    if (state->public_status.mode == AURABOT_MODE_AUTONOMOUS && hardware_autonomous_available()) {
        result = autonomous_tick(state);
        if (result != AURABOT_OK) return enter_safe_stop(state);
    }
    state->last_tick = now;
    return state_outputs_apply(state) == AURABOT_OK
        ? AURABOT_OK : enter_safe_stop(state);
}

int robot_controller_set_mode(aurabot_state_t *state, aurabot_mode_t mode)
{
    int result;
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (mode != AURABOT_MODE_MANUAL && mode != AURABOT_MODE_AUTONOMOUS)
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (motion_stop(state) != AURABOT_OK) return enter_safe_stop(state);
    control_lease_clear(state);
    result = fsm_transition_mode(state, mode);
    if (result != AURABOT_OK) return result;
    if (mode == AURABOT_MODE_AUTONOMOUS && hardware_autonomous_available()) {
        result = autonomous_enter(state);
        if (result != AURABOT_OK) return enter_safe_stop(state);
        result = autonomous_tick(state);
        if (result != AURABOT_OK) return enter_safe_stop(state);
        audio_controller_event("autonomous");
    } else if (mode == AURABOT_MODE_MANUAL) {
        audio_controller_event("manual");
    } else {
        fprintf(stderr, "aurabot: autonomo inactivo; requiere ambos motores/encoders y al menos un infrarrojo habilitado\n");
    }
    return state_outputs_apply(state) == AURABOT_OK
        ? AURABOT_OK : enter_safe_stop(state);
}

int robot_controller_claim(aurabot_state_t *state,
                           const unsigned char *token)
{
    return control_lease_claim(state, token);
}

int robot_controller_heartbeat(aurabot_state_t *state,
                               const unsigned char *token)
{
    return control_lease_heartbeat(state, token);
}

int robot_controller_release(aurabot_state_t *state,
                             const unsigned char *token)
{
    int result;
    if (!control_lease_is_owner(state, token)) return AURABOT_ERR_NOT_OWNER;
    result = motion_stop(state);
    if (result != AURABOT_OK) return enter_safe_stop(state);
    return control_lease_release(state, token);
}

int robot_controller_drive(aurabot_state_t *state,
                           const unsigned char *token,
                           int left_percent, int right_percent)
{
    int result;
    if (state == NULL || left_percent < -100 || left_percent > 100 ||
        right_percent < -100 || right_percent > 100)
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (!control_lease_is_owner(state, token)) return AURABOT_ERR_NOT_OWNER;
    result = motion_drive(state, left_percent, right_percent);
    if (result != AURABOT_OK) return enter_safe_stop(state);
    return control_lease_heartbeat(state, token);
}

int robot_controller_stop(aurabot_state_t *state,
                          const unsigned char *token)
{
    int result;
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (state->public_status.mode != AURABOT_MODE_MANUAL)
        return AURABOT_ERR_WRONG_MODE;
    if (!control_lease_is_owner(state, token)) return AURABOT_ERR_NOT_OWNER;
    result = motion_stop(state);
    if (result != AURABOT_OK) return enter_safe_stop(state);
    return control_lease_heartbeat(state, token);
}

int robot_controller_emergency_stop(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    (void)enter_safe_stop(state);
    return AURABOT_OK;
}
