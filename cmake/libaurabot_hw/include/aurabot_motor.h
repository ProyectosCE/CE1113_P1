#ifndef AURABOT_MOTOR_H
#define AURABOT_MOTOR_H

#include <stdint.h>

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_MOTOR_LEFT = 0,
    AURABOT_MOTOR_RIGHT
} aurabot_motor_id_t;


typedef enum {
    AURABOT_ENCODER_LEFT = 0,
    AURABOT_ENCODER_RIGHT
} aurabot_encoder_id_t;


aurabot_hw_status_t aurabot_motor_set_speed(
    aurabot_motor_id_t motor,
    int speed_percent
);

aurabot_hw_status_t aurabot_motor_stop(
    aurabot_motor_id_t motor
);


aurabot_hw_status_t aurabot_encoder_get_count(
    aurabot_encoder_id_t encoder,
    int64_t *count
);

aurabot_hw_status_t aurabot_encoder_reset(
    aurabot_encoder_id_t encoder
);


#endif