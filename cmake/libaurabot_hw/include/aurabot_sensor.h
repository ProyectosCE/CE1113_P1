#ifndef AURABOT_SENSOR_H
#define AURABOT_SENSOR_H

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_PROXIMITY_FRONT = 0,
    AURABOT_PROXIMITY_SIDE
} aurabot_proximity_sensor_id_t;


/*
 * Proximity sensor operations will be added
 * The infrared sensors are digital, so the public API will expose
 * logical obstacle detection instead of physical GPIO values
 */


#endif