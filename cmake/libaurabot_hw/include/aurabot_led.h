#ifndef AURABOT_LED_H
#define AURABOT_LED_H

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_LED_SYSTEM = 0,
    AURABOT_LED_AUTONOMOUS,
    AURABOT_LED_MANUAL,
    AURABOT_LED_OBSTACLE
} aurabot_led_id_t;


/*
 * LED control operations will be added
 * Physical GPIO assignments will remain private to the implementation
 */


#endif