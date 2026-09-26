#include <errno.h>
#include <stdio.h>

#include "../include/libgpio.h"
#include "gpio_sysfs.h"
#include "libpwm.h"
#include "sysfs_io.h"

int pinMode(int pin, const char *mode)
{
    return gpio_sysfs_configure(pin, mode);
}

int digitalWrite(int pin, int value)
{
    if (value != 0 && value != 1) {
        errno = EINVAL;
        return -1;
    }

    int number = gpio_sysfs_number(pin);

    if (number < 0) {
        return -1;
    }

    char path[128];

    snprintf(
        path, sizeof(path),
        GPIO_SYSFS_PATH "/gpio%d/value", number
    );

    return sysfs_write_text(path, value ? "1" : "0");
}

int digitalRead(int pin)
{
    int number = gpio_sysfs_number(pin);

    if (number < 0) {
        return -1;
    }

    char path[128];
    int value;

    snprintf(
        path, sizeof(path),
        GPIO_SYSFS_PATH "/gpio%d/value", number
    );

    if (sysfs_read_int(path, &value) != 0) {
        return -1;
    }

    if (value != 0 && value != 1) {
        errno = EIO;
        return -1;
    }

    return value;
}

int gpioRelease(int pin)
{
    return gpio_sysfs_release(pin);
}

/* Temporary compatibility with the existing motor code. */
int setPWM(int pin, int freq, int duty_percent)
{
    return pwm_set(pin, freq, duty_percent);
}