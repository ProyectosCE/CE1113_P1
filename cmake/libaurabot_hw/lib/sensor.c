#include <stddef.h>

#include <libgpio.h>

#include "../include/aurabot_sensor.h"

#include "aurabot_hw_internal.h"


#define PROXIMITY_SENSOR_COUNT 2
#define PIN_NOT_CONFIGURED -1


typedef struct {
    int pin;
    int active_low;
} proximity_sensor_config_t;


/*
 * TODO: Replace PIN_NOT_CONFIGURED with the GPIO pins
 *
 * Each entry follows this order:
 * {gpio_pin, active_low}
 *
 * Example:
 * {17, 1}
 *
 * This means:
 * - GPIO 17 receives the sensor digital output
 * - active_low = 1 means LOW indicates an obstacle
 *
 * Flying-Fish infrared sensors commonly use active-low output,
 * but this should be verified with the physical sensors before
 * final hardware integration
 *
 * The first entry corresponds to AURABOT_PROXIMITY_FRONT
 * The second entry corresponds to AURABOT_PROXIMITY_SIDE
 *
 * GPIO values shown in examples are not actual project wiring.
 */
static const proximity_sensor_config_t sensor_config[PROXIMITY_SENSOR_COUNT] = {
    {PIN_NOT_CONFIGURED, 1},
    {PIN_NOT_CONFIGURED, 1}
};


static int sensor_id_is_valid(
    aurabot_proximity_sensor_id_t sensor
)
{
    return sensor == AURABOT_PROXIMITY_FRONT || sensor == AURABOT_PROXIMITY_SIDE;
}


static int sensor_is_configured(
    const proximity_sensor_config_t *sensor
)
{
    return sensor->pin >= 0;
}


aurabot_hw_status_t aurabot_proximity_get_state(
    aurabot_proximity_sensor_id_t sensor,
    aurabot_proximity_state_t *state
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!sensor_id_is_valid(sensor) || state == NULL) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    const proximity_sensor_config_t *config = &sensor_config[sensor];

    if (!sensor_is_configured(config)) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    if (pinMode(config->pin, "in") != 0) {
        return AURABOT_HW_ERROR;
    }

    int value = digitalRead(config->pin);

    if (value < 0) {
        return AURABOT_HW_ERROR;
    }

    int obstacle_detected;

    if (config->active_low) {
        obstacle_detected = (value == 0);
    } else {
        obstacle_detected = (value != 0);
    }

    if (obstacle_detected) {
        *state = AURABOT_PROXIMITY_OBSTACLE;
    } else {
        *state = AURABOT_PROXIMITY_CLEAR;
    }

    return AURABOT_HW_OK;
}