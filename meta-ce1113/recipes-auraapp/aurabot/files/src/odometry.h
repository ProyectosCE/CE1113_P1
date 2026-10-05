#ifndef AURABOT_ODOMETRY_H
#define AURABOT_ODOMETRY_H

#include "aurabot_state.h"
#include "hardware.h"

typedef struct {
    unsigned int left_ticks_per_revolution;
    unsigned int right_ticks_per_revolution;
    unsigned int left_wheel_circumference_mm;
    unsigned int right_wheel_circumference_mm;
    unsigned int wheel_base_mm;
} odometry_config_t;

int odometry_update(aurabot_state_t *state,
                    const encoder_snapshot_t *encoders,
                    const odometry_config_t *config);

#endif
