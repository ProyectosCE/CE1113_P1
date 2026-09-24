#ifndef AURABOT_SENSOR_H
#define AURABOT_SENSOR_H

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_PROXIMITY_FRONT = 0,
    AURABOT_PROXIMITY_SIDE
} aurabot_proximity_sensor_id_t;


typedef enum {
    AURABOT_PROXIMITY_CLEAR = 0,
    AURABOT_PROXIMITY_OBSTACLE
} aurabot_proximity_state_t;


aurabot_hw_status_t aurabot_proximity_get_state(
    aurabot_proximity_sensor_id_t sensor,
    aurabot_proximity_state_t *state
);


#endif