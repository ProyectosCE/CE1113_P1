#include <stdlib.h>

#include <libgpio.h>

#include "../include/aurabot_motor.h"

#include "aurabot_hw_internal.h"


#define MOTOR_COUNT 2
#define MOTOR_PWM_FREQUENCY_HZ 1000
#define PIN_NOT_CONFIGURED -1


typedef struct {
    int pin1;
    int pin2;
    int pwm;
    int forward_pin1_value;
    int forward_pin2_value;
} motor_config_t;


/*
 * TODO: Replace these placeholder values with the GPIO/PWM
 *
 * Each entry follows this order:
 * {pin1, pin2, pwm, forward_pin1_value, forward_pin2_value}
 *
 * Example:
 * {17, 27, 0, 1, 0}
 *
 * This would mean:
 * - pin1 = GPIO 17
 * - pin2 = GPIO 27
 * - pwm  = PWM channel 0
 * - forward direction: pin1 = 1, pin2 = 0
 *
 * If a motor is physically mounted in the opposite orientation,
 * its forward values may need to be reversed, for example:
 * {22, 23, 1, 0, 1}
 *
 * The first entry corresponds to AURABOT_MOTOR_LEFT.
 * The second entry corresponds to AURABOT_MOTOR_RIGHT.
 *
 * The GPIO/PWM values shown above are examples only.
 */
static const motor_config_t motor_config[MOTOR_COUNT] = {
    {PIN_NOT_CONFIGURED, PIN_NOT_CONFIGURED, PIN_NOT_CONFIGURED, 1, 0},
    {PIN_NOT_CONFIGURED, PIN_NOT_CONFIGURED, PIN_NOT_CONFIGURED, 1, 0}
};


static int motor_id_is_valid(aurabot_motor_id_t motor)
{
    return motor == AURABOT_MOTOR_LEFT ||
           motor == AURABOT_MOTOR_RIGHT;
}


static int motor_is_configured(const motor_config_t *motor)
{
    return motor->pin1 >= 0 &&
           motor->pin2 >= 0 &&
           motor->pwm >= 0;
}


aurabot_hw_status_t aurabot_motor_set_speed(
    aurabot_motor_id_t motor,
    int speed_percent
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!motor_id_is_valid(motor)) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    if (speed_percent < -100 || speed_percent > 100) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    const motor_config_t *config = &motor_config[motor];

    if (!motor_is_configured(config)) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    if (speed_percent == 0) {
        if (setPWM(config->pwm, MOTOR_PWM_FREQUENCY_HZ, 0) != 0) {
            return AURABOT_HW_ERROR;
        }

        if (digitalWrite(config->pin1, 0) != 0 || digitalWrite(config->pin2, 0) != 0) {
            return AURABOT_HW_ERROR;
        }

        return AURABOT_HW_OK;
    }

    int pin1_value = config->forward_pin1_value;
    int pin2_value = config->forward_pin2_value;

    if (speed_percent < 0) {
        pin1_value = !pin1_value;
        pin2_value = !pin2_value;
    }

    if (digitalWrite(config->pin1, pin1_value) != 0 ||
        digitalWrite(config->pin2, pin2_value) != 0) {
        return AURABOT_HW_ERROR;
    }

    if (setPWM(config->pwm, MOTOR_PWM_FREQUENCY_HZ, abs(speed_percent)) != 0) {
        return AURABOT_HW_ERROR;
    }

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_motor_stop(
    aurabot_motor_id_t motor
)
{
    return aurabot_motor_set_speed(motor, 0);
}


aurabot_hw_status_t aurabot_encoder_get_count(
    aurabot_encoder_id_t encoder,
    int64_t *count
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (encoder != AURABOT_ENCODER_LEFT && encoder != AURABOT_ENCODER_RIGHT) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    if (count == NULL) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    return AURABOT_HW_NOT_IMPLEMENTED;
}


aurabot_hw_status_t aurabot_encoder_reset(
    aurabot_encoder_id_t encoder
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (encoder != AURABOT_ENCODER_LEFT &&
        encoder != AURABOT_ENCODER_RIGHT) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    return AURABOT_HW_NOT_IMPLEMENTED;
}