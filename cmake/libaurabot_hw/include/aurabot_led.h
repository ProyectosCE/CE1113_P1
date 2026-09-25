#ifndef AURABOT_LED_H
#define AURABOT_LED_H

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_LED_SYSTEM = 0,
    AURABOT_LED_AUTONOMOUS,
    AURABOT_LED_MANUAL,
    AURABOT_LED_OBSTACLE
} aurabot_led_id_t;


typedef enum {
    AURABOT_LED_OFF = 0,
    AURABOT_LED_ON
} aurabot_led_state_t;


aurabot_hw_status_t aurabot_led_set_state(
    aurabot_led_id_t led,
    aurabot_led_state_t state
);

aurabot_hw_status_t aurabot_led_get_state(
    aurabot_led_id_t led,
    aurabot_led_state_t *state
);


#endif