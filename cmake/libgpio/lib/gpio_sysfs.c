#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gpio_sysfs.h"
#include "sysfs_io.h"

#define GPIO_LABEL "pinctrl-bcm2711"
#define GPIO_MAX_PINS 58

static int gpio_base = -1;
static int gpio_count = 0;

static unsigned char exported_by_us[GPIO_MAX_PINS];

/* Find the BCM2711 GPIO controller. */
static int find_controller(void)
{
    if (gpio_base >= 0) {
        return 0;
    }

    DIR *directory = opendir(GPIO_SYSFS_PATH);

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

        int length = snprintf(
            path, sizeof(path), "%s/%s/label",
            GPIO_SYSFS_PATH, entry->d_name
        );

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

        length = snprintf(
            path, sizeof(path), "%s/%s/base",
            GPIO_SYSFS_PATH, entry->d_name
        );

        if (length < 0 || (size_t)length >= sizeof(path) ||
            sysfs_read_int(path, &gpio_base) != 0) {
            gpio_base = -1;
            continue;
        }

        length = snprintf(
            path, sizeof(path), "%s/%s/ngpio",
            GPIO_SYSFS_PATH, entry->d_name
        );

        if (length < 0 || (size_t)length >= sizeof(path) ||
            sysfs_read_int(path, &gpio_count) != 0 ||
            gpio_base < 0 || gpio_count <= 0) {
            gpio_base = -1;
            continue;
        }

        (void)closedir(directory);
        return 0;
    }

    (void)closedir(directory);

    gpio_base = -1;
    errno = ENODEV;

    return -1;
}

/* Convert a BCM GPIO number into its global sysfs number. */
int gpio_sysfs_number(int pin)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        errno = EINVAL;
        return -1;
    }

    if (find_controller() != 0) {
        return -1;
    }

    if (pin >= gpio_count) {
        errno = EINVAL;
        return -1;
    }

    return gpio_base + pin;
}

/* Export the GPIO when needed and configure its direction. */
int gpio_sysfs_configure(int pin, const char *mode)
{
    if (mode == NULL ||
        (strcmp(mode, "in") != 0 && strcmp(mode, "out") != 0)) {
        errno = EINVAL;
        return -1;
    }

    int number = gpio_sysfs_number(pin);

    if (number < 0) {
        return -1;
    }

    char path[128];
    char text[16];

    int newly_exported = 0;

    snprintf(
        path, sizeof(path),
        GPIO_SYSFS_PATH "/gpio%d", number
    );

    if (access(path, F_OK) != 0) {
        if (errno != ENOENT) {
            return -1;
        }

        exported_by_us[pin] = 0;

        snprintf(text, sizeof(text), "%d", number);

        if (sysfs_write_text(
                GPIO_SYSFS_PATH "/export", text) == 0) {

            exported_by_us[pin] = 1;
            newly_exported = 1;

        } else {
            int saved_errno = errno;

            if (saved_errno != EBUSY ||
                access(path, F_OK) != 0) {

                errno = saved_errno;
                return -1;
            }
        }
    }

    snprintf(
        path, sizeof(path),
        GPIO_SYSFS_PATH "/gpio%d/direction", number
    );

    if (sysfs_write_text(path, mode) != 0) {
        int saved_errno = errno;

        if (newly_exported) {
            (void)gpio_sysfs_release(pin);
        }

        errno = saved_errno;
        return -1;
    }

    return 0;
}

/* Release only GPIOs exported by this process. */
int gpio_sysfs_release(int pin)
{
    int number = gpio_sysfs_number(pin);

    if (number < 0) {
        return -1;
    }

    if (!exported_by_us[pin]) {
        return 0;
    }

    char text[16];

    snprintf(text, sizeof(text), "%d", number);

    if (sysfs_write_text(
            GPIO_SYSFS_PATH "/unexport", text) != 0) {
        return -1;
    }

    exported_by_us[pin] = 0;

    return 0;
}