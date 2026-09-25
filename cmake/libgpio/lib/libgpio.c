#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/libgpio.h"

#ifndef GPIO_PATH
#define GPIO_PATH "/sys/class/gpio"
#endif

#define GPIO_LABEL "pinctrl-bcm2711"
#define GPIO_MAX_PINS 58

#define PWM_CHIP 0
#define NSEC_PER_SEC 1000000000LL

static int gpio_base = -1;
static int gpio_count = 0;
static unsigned char exported_by_us[GPIO_MAX_PINS];

/* Write a small text value directly to a sysfs attribute. */
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

    return fclose(file) == 0 ? 0 : -1;
}

/* Read one integer from a sysfs attribute. */
static int readNumberFile(const char *path, int *number)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }

    if (fscanf(file, "%d", number) != 1) {
        (void)fclose(file);
        errno = EIO;
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}

/* Discover the main BCM2711 GPIO controller, then cache its base. */
static int findGpioController(void)
{
    if (gpio_base >= 0) {
        return 0;
    }

    DIR *directory = opendir(GPIO_PATH);
    if (directory == NULL) {
        return -1;
    }

    struct dirent *entry;
    char path[256];
    char label[64];

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, "gpiochip", 8) != 0) {
            continue;
        }

        int length = snprintf(path, sizeof(path), "%s/%s/label",
                              GPIO_PATH, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(path)) {
            continue;
        }

        FILE *file = fopen(path, "r");
        if (file == NULL) {
            continue;
        }

        int has_label = fgets(label, sizeof(label), file) != NULL;
        (void)fclose(file);
        if (!has_label) {
            continue;
        }

        label[strcspn(label, "\r\n")] = '\0';

        if (strcmp(label, GPIO_LABEL) != 0) {
            continue;
        }

        length = snprintf(path, sizeof(path), "%s/%s/base",
                          GPIO_PATH, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(path)) {
            continue;
        }

        if (readNumberFile(path, &gpio_base) != 0) {
            break;
        }

        length = snprintf(path, sizeof(path), "%s/%s/ngpio",
                          GPIO_PATH, entry->d_name);
        if (length < 0 || (size_t)length >= sizeof(path)) {
            gpio_base = -1;
            continue;
        }

        if (readNumberFile(path, &gpio_count) != 0 ||
            gpio_base < 0 || gpio_count <= 0) {
            gpio_base = -1;
            break;
        }

        (void)closedir(directory);
        return 0;
    }

    (void)closedir(directory);
    gpio_base = -1;
    errno = ENODEV;
    return -1;
}

/* Translate a logical GPIO number to its sysfs number. */
static int globalPin(int pin)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        errno = EINVAL;
        return -1;
    }

    if (findGpioController() != 0) {
        return -1;
    }

    if (pin >= gpio_count) {
        errno = EINVAL;
        return -1;
    }

    return gpio_base + pin;
}

int pinMode(int pin, const char *mode)
{
    if (mode == NULL ||
        (strcmp(mode, "in") != 0 && strcmp(mode, "out") != 0)) {
        errno = EINVAL;
        return -1;
    }

    int number = globalPin(pin);
    if (number < 0) {
        return -1;
    }

    char path[128];
    char text[16];
    int newly_exported = 0;

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d", number);

    if (!exported_by_us[pin] && access(path, F_OK) != 0) {
        if (errno != ENOENT) {
            return -1;
        }

        snprintf(text, sizeof(text), "%d", number);

        if (writeTextFile(GPIO_PATH "/export", text) == 0) {
            exported_by_us[pin] = 1;
            newly_exported = 1;
        } else {
            int saved_errno = errno;

            if (saved_errno != EBUSY || access(path, F_OK) != 0) {
                errno = saved_errno;
                return -1;
            }
        }
    }

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", number);

    if (writeTextFile(path, mode) != 0) {
        int saved_errno = errno;

        if (newly_exported) {
            (void)gpioRelease(pin);
        }

        errno = saved_errno;
        return -1;
    }

    return 0;
}

int digitalWrite(int pin, int value)
{
    if (value != 0 && value != 1) {
        errno = EINVAL;
        return -1;
    }

    int number = globalPin(pin);
    if (number < 0) {
        return -1;
    }

    char path[128];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", number);

    return writeTextFile(path, value ? "1" : "0");
}

int digitalRead(int pin)
{
    int number = globalPin(pin);
    if (number < 0) {
        return -1;
    }

    char path[128];
    int value;

    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", number);

    if (readNumberFile(path, &value) != 0) {
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
    int number = globalPin(pin);
    if (number < 0) {
        return -1;
    }

    if (!exported_by_us[pin]) {
        return 0;
    }

    char text[16];
    snprintf(text, sizeof(text), "%d", number);

    if (writeTextFile(GPIO_PATH "/unexport", text) != 0) {
        return -1;
    }

    exported_by_us[pin] = 0;
    return 0;
}

/* PWM remains unchanged until commit 3. */
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
