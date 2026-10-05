#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <libgpio.h>
#include <libleds.h>
#include <libpwm.h>
#include <libsensors.h>

#include "hardware.h"
#include "robot_controller.h"

/* Nivel eléctrico real del sensor: alto en reposo, bajo con proximidad. */
static int sensor_value = 1;
static int red_led;
static int auto_led;
static int power_led;

/* Cualquier acceso a un dispositivo deshabilitado debe fallar la prueba. */
int pinMode(int pin, const char *mode)
{
    assert(pin == 24 && strcmp(mode, "in") == 0);
    return 0;
}
int digitalRead(int pin) { (void)pin; assert(0); return -1; }
int digitalWrite(int pin, int value)
{ (void)pin; (void)value; assert(0); return -1; }
int setPWM(int pin, int frequency, int duty)
{ (void)pin; (void)frequency; (void)duty; assert(0); return -1; }
int stopPWM(int pin) { (void)pin; assert(0); return -1; }
int sensorReadDigital(int pin, int active_low)
{
    assert(pin == 24 && active_low == 1);
    if (sensor_value < 0) {
        errno = EIO;
        return -1;
    }
    return active_low ? !sensor_value : sensor_value;
}
int ledSet(int pin, int active_high, int enabled)
{
    assert(active_high == 1);
    if (pin == 17) red_led = enabled;
    else if (pin == 22) power_led = enabled;
    else if (pin == 27) auto_led = enabled;
    else assert(0);
    return 0;
}

int main(void)
{
    aurabot_state_t state;
    encoder_snapshot_t sample;
    unsigned char owner[AURABOT_OWNER_TOKEN_SIZE] = {1};

    assert(robot_controller_init(&state) == AURABOT_OK);
    assert(state.public_status.mode == AURABOT_MODE_AUTONOMOUS);
    assert(state.public_status.auto_state == AURABOT_AUTO_INACTIVE);
    assert(state.public_status.left_speed == 0 && state.public_status.right_speed == 0);
    assert(power_led == 1 && auto_led == 1 && red_led == 0);
    assert(hardware_take_encoder_sample(&sample) == 0);
    assert(sample.left_ticks == 0 && sample.right_ticks == 0);
    assert(sample.left_angular_velocity_mrad_s == 0 && sample.right_angular_velocity_mrad_s == 0);

    sensor_value = 0;
    state.motion_last_tick.tv_sec -= 60;
    state.maneuver_started.tv_sec -= 60;
    assert(robot_controller_tick(&state) == AURABOT_OK);
    assert(state.public_status.left_obstacle == 0 && state.public_status.right_obstacle == 1);
    assert(red_led == 1 && state.public_status.mode == AURABOT_MODE_AUTONOMOUS);
    assert(state.public_status.x_mm == 0 && state.public_status.y_mm == 0);
    sensor_value = 1;
    assert(robot_controller_tick(&state) == AURABOT_OK);
    assert(red_led == 0);

    assert(robot_controller_set_mode(&state, AURABOT_MODE_MANUAL) == AURABOT_OK);
    sensor_value = 0;
    assert(robot_controller_tick(&state) == AURABOT_OK);
    assert(red_led == 1 && state.public_status.right_obstacle == 1);
    sensor_value = 1;
    assert(robot_controller_tick(&state) == AURABOT_OK);
    assert(red_led == 0 && state.public_status.right_obstacle == 0);
    assert(robot_controller_claim(&state, owner) == AURABOT_OK);
    assert(robot_controller_drive(&state, owner, 50, 50) == AURABOT_OK);
    assert(state.public_status.left_speed == 0 && state.public_status.right_speed == 0);
    assert(hardware_audio_track_count() == 0);
    assert(hardware_audio_play(0) == -1 && errno == ENODEV);

    /* Los errores del sensor conectado siguen deteniendo el controlador. */
    sensor_value = -1;
    assert(robot_controller_tick(&state) == AURABOT_ERR_HARDWARE);
    assert(state.public_status.mode == AURABOT_MODE_SAFE_STOP);
    robot_controller_shutdown(&state);
    assert(power_led == 0 && auto_led == 0 && red_led == 0);
    puts("partial hardware tests passed");
    return 0;
}
