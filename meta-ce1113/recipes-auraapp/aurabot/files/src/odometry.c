#include <stddef.h>

#include "angle_math.h"
#include "aurabot_config.h"
#include "odometry.h"

int odometry_update(aurabot_state_t *state,
                    const encoder_snapshot_t *encoders,
                    const odometry_config_t *config)
{
    long long left_numerator;
    long long right_numerator;
    int left_distance;
    int right_distance;
    int center_distance;
    int delta_heading;

    if (state == NULL || encoders == NULL || config == NULL ||
        config->left_ticks_per_revolution == 0U ||
        config->right_ticks_per_revolution == 0U ||
        config->wheel_base_mm == 0U)
        return AURABOT_ERR_INVALID_ARGUMENT;

    left_numerator = state->left_distance_remainder +
        (long long)encoders->left_direction * encoders->left_ticks *
        config->left_wheel_circumference_mm;
    right_numerator = state->right_distance_remainder +
        (long long)encoders->right_direction * encoders->right_ticks *
        config->right_wheel_circumference_mm;
    left_distance = (int)(left_numerator /
        config->left_ticks_per_revolution);
    right_distance = (int)(right_numerator /
        config->right_ticks_per_revolution);
    state->left_distance_remainder = (int)(left_numerator %
        config->left_ticks_per_revolution);
    state->right_distance_remainder = (int)(right_numerator %
        config->right_ticks_per_revolution);

    center_distance = (left_distance + right_distance) / 2;
    delta_heading = (right_distance - left_distance) * 1000 /
        (int)config->wheel_base_mm;
    state->public_status.heading_mrad = angle_normalize_mrad(
        state->public_status.heading_mrad + delta_heading);
    state->public_status.x_mm += center_distance *
        angle_cosine_scaled(state->public_status.heading_mrad) /
        AURABOT_TRIG_SCALE;
    state->public_status.y_mm += center_distance *
        angle_sine_scaled(state->public_status.heading_mrad) /
        AURABOT_TRIG_SCALE;
    state->maneuver_distance_mm += center_distance;
    state->maneuver_angle_mrad += delta_heading;
    return AURABOT_OK;
}
