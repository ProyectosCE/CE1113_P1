#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/libgpio.h"

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_BASE 512
#define GPIO_COUNT 58

#define PWM_CHIP 0
#define NSEC_PER_SEC 1000000000LL

/* Write text to a sysfs file. Return 0 on success and -1 on error. */
static int writeTextFile(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return -1;
    }

    if (fputs(text, file) == EOF) {
        int saved_errno = errno;
        (void)fclose(file);
        errno = saved_errno;
        return -1;
    }

    if (fclose(file) == EOF) {
        return -1;
    }

    return 0;
}

int pinMode(int pin, const char *mode)
{
    if (pin < 0 || pin >= GPIO_COUNT || mode == NULL || (strcmp(mode, "in") != 0 && strcmp(mode, "out") != 0)) {
        errno = EINVAL;
        return -1;
    }

    char number[16];
    char path[128];
    int global_pin = GPIO_BASE + pin;

    snprintf(number, sizeof(number), "%d", global_pin);

    if (writeTextFile(GPIO_PATH "/export", number) != 0 && errno != EBUSY) {
        return -1;
    }

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", global_pin);
    return writeTextFile(path, mode);
}

int digitalWrite(int pin, int value)
{
    if (pin < 0 || pin >= GPIO_COUNT || (value != 0 && value != 1)) {
        errno = EINVAL;
        return -1;
    }

    char path[128];
    int global_pin = GPIO_BASE + pin;

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", global_pin);
    return writeTextFile(path, value == 1 ? "1" : "0");
}

int digitalRead(int pin)
{
    if (pin < 0 || pin >= GPIO_COUNT) {
        errno = EINVAL;
        return -1;
    }

    char path[128];
    int value;
    int global_pin = GPIO_BASE + pin;

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", global_pin);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }

    if (fscanf(file, "%d", &value) != 1) {
        (void)fclose(file);
        errno = EIO;
        return -1;
    }

    if (fclose(file) == EOF) {
        return -1;
    }

    if (value != 0 && value != 1) {
        errno = EIO;
        return -1;
    }

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
