#include <stddef.h>
#include <libgpio.h>

#include "../include/aurabot_led.h"
#include "aurabot_hw_internal.h"

#define LED_COUNT 4
#define PIN_NOT_CONFIGURED -1

typedef struct {
    int pin;
    int active_high;
} led_config_t;


/*
 * TODO: Replace PIN_NOT_CONFIGURED with the GPIO pins
 *
 * Each entry follows this order:
 * {gpio_pin, active_high}
 *
 * Example:
 * {17, 1}
 *
 * This means:
 * - The LED is controlled through GPIO 17
 * - active_high = 1 means writing HIGH turns the LED on
 *
 * If the hardware uses inverted logic, use active_high = 0
 *
 * Entry order:
 * 0 -> AURABOT_LED_SYSTEM
 * 1 -> AURABOT_LED_AUTONOMOUS
 * 2 -> AURABOT_LED_MANUAL
 * 3 -> AURABOT_LED_OBSTACLE
 *
 * GPIO values shown in examples are not actual project wiring
 */
static const led_config_t led_config[LED_COUNT] = {
    {PIN_NOT_CONFIGURED, 1},
    {PIN_NOT_CONFIGURED, 1},
    {PIN_NOT_CONFIGURED, 1},
    {PIN_NOT_CONFIGURED, 1}
};


static int led_id_is_valid(aurabot_led_id_t led)
{
    return led == AURABOT_LED_SYSTEM || led == AURABOT_LED_AUTONOMOUS || led == AURABOT_LED_MANUAL || led == AURABOT_LED_OBSTACLE;
}


static int led_state_is_valid(aurabot_led_state_t state)
{
    return state == AURABOT_LED_OFF || state == AURABOT_LED_ON;
}


static int led_is_configured(const led_config_t *led)
{
    return led->pin >= 0;
}


aurabot_hw_status_t aurabot_led_set_state(
    aurabot_led_id_t led,
    aurabot_led_state_t state
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!led_id_is_valid(led) || !led_state_is_valid(state)) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    const led_config_t *config = &led_config[led];

    if (!led_is_configured(config)) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    if (pinMode(config->pin, "out") != 0) {
        return AURABOT_HW_ERROR;
    }

    int value;

    if (config->active_high) {
        value = (state == AURABOT_LED_ON);
    } else {
        value = (state == AURABOT_LED_OFF);
    }

    if (digitalWrite(config->pin, value) != 0) {
        return AURABOT_HW_ERROR;
    }

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_led_get_state(
    aurabot_led_id_t led,
    aurabot_led_state_t *state
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!led_id_is_valid(led) || state == NULL) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    const led_config_t *config = &led_config[led];

    if (!led_is_configured(config)) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    int value = digitalRead(config->pin);

    if (value < 0) {
        return AURABOT_HW_ERROR;
    }

    int led_on;

    if (config->active_high) {
        led_on = (value != 0);
    } else {
        led_on = (value == 0);
    }

    if (led_on) {
        *state = AURABOT_LED_ON;
    } else {
        *state = AURABOT_LED_OFF;
    }

    return AURABOT_HW_OK;
}