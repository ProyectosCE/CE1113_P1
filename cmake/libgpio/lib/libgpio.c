#include <stdio.h>
#include <stdlib.h>

#include "../include/libgpio.h"

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_BASE 512

#define PWM_CHIP 0
#define NSEC_PER_SEC 1000000000LL


int pinMode(int pin, const char *mode)
{
    if (pin < 0 || mode == NULL) {
        return -1;
    }

    char export_command[100];
    char mode_command[100];

    snprintf(export_command, sizeof(export_command), "echo %d > %s/export", pin + GPIO_BASE, GPIO_PATH);

    snprintf(mode_command, sizeof(mode_command), "echo %s > %s/gpio%d/direction", mode, GPIO_PATH, pin + GPIO_BASE);

    /*
     * Export may fail when the GPIO was already exported.
     * Direction configuration is therefore the relevant
     * operation checked here.
     */
    system(export_command);

    if (system(mode_command) != 0) {
        return -1;
    }

    return 0;
}


int digitalWrite(int pin, int value)
{
    if (pin < 0) {
        return -1;
    }

    if (value != 0 && value != 1) {
        return -1;
    }

    char command[100];

    snprintf(command, sizeof(command), "echo %d > %s/gpio%d/value", value, GPIO_PATH, pin + GPIO_BASE);

    if (system(command) != 0) {
        return -1;
    }

    return 0;
}


int digitalRead(int pin)
{
    if (pin < 0) {
        return -1;
    }

    char path[100];
    int value;

    snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_PATH, pin + GPIO_BASE);

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return -1;
    }

    if (fscanf(file, "%d", &value) != 1) {
        fclose(file);
        return -1;
    }

    fclose(file);

    return value;
}


int setPWM(int pin, int freq, int duty_percent)
{
    if (pin < 0 || freq <= 0) {
        return -1;
    }

    if (duty_percent < 0) {
        duty_percent = 0;
    }

    if (duty_percent > 100) {
        duty_percent = 100;
    }

    long long period_ns = NSEC_PER_SEC / freq;
    long long duty_ns = (period_ns * duty_percent) / 100;

    char export_command[100];
    char period_command[128];
    char duty_command[128];
    char enable_command[128];

    snprintf(export_command, sizeof(export_command), "echo %d > /sys/class/pwm/pwmchip%d/export", pin, PWM_CHIP);

    snprintf(period_command, sizeof(period_command), "echo %lld > /sys/class/pwm/pwmchip%d/pwm%d/period", period_ns, PWM_CHIP, pin);

    snprintf(duty_command, sizeof(duty_command), "echo %lld > /sys/class/pwm/pwmchip%d/pwm%d/duty_cycle", duty_ns, PWM_CHIP, pin);

    snprintf(enable_command, sizeof(enable_command), "echo %d > /sys/class/pwm/pwmchip%d/pwm%d/enable", duty_percent > 0 ? 1 : 0, PWM_CHIP, pin);

    /*
     * Export may fail when the PWM channel was already exported.
     */
    system(export_command);

    if (system(period_command) != 0) {
        return -1;
    }

    if (system(duty_command) != 0) {
        return -1;
    }

    if (system(enable_command) != 0) {
        return -1;
    }

    return 0;
}