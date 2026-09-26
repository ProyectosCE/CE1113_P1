#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "libpwm.h"
#include "sysfs_io.h"

#ifndef PWM_SYSFS_ROOT
#define PWM_SYSFS_ROOT "/sys/class/pwm"
#endif

#define PWM_CHIP_PATH PWM_SYSFS_ROOT "/pwmchip0"
#define PWM_MAX_CHANNELS 2
#define NSEC_PER_SEC 1000000000LL

static bool exported_by_us[PWM_MAX_CHANNELS];

/* Build the sysfs path for a PWM channel or one of its attributes. */
static int pwm_path(
    int channel,
    const char *field,
    char *path,
    size_t size
)
{
    int length;

    if (field == NULL) {
        length = snprintf(path, size, PWM_CHIP_PATH "/pwm%d", channel);
    } else {
        length = snprintf(path, size, PWM_CHIP_PATH "/pwm%d/%s", channel, field);
    }

    if (length < 0 || (size_t)length >= size) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}

/* Verify that the requested channel exists. */
static int validate_channel(int channel)
{
    int npwm;

    if (channel < 0 || channel >= PWM_MAX_CHANNELS) {
        errno = EINVAL;
        return -1;
    }

    if (sysfs_read_int(PWM_CHIP_PATH "/npwm", &npwm) != 0) {
        return -1;
    }

    if (npwm <= channel) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

/* Write one PWM attribute. */
static int write_field(
    int channel,
    const char *field,
    const char *text
)
{
    char path[256];

    if (pwm_path(channel, field, path, sizeof(path)) != 0) {
        return -1;
    }

    return sysfs_write_text(path, text);
}

/* Read one numeric PWM attribute. */
static int read_field(
    int channel,
    const char *field,
    int *value
)
{
    char path[256];

    if (pwm_path(channel, field, path, sizeof(path)) != 0) {
        return -1;
    }

    return sysfs_read_int(path, value);
}

/* Export the channel if it is not already exported. */
static int export_channel(
    int channel,
    bool *newly_exported
)
{
    char path[256];
    char number[12];

    *newly_exported = false;

    if (pwm_path(channel, NULL, path, sizeof(path)) != 0) {
        return -1;
    }

    if (access(path, F_OK) == 0) {
        return 0;
    }

    if (errno != ENOENT) {
        return -1;
    }

    (void)snprintf(number, sizeof(number), "%d", channel);

    if (sysfs_write_text(PWM_CHIP_PATH "/export", number) != 0) {

        int saved_errno = errno;

        /*
         * Another process may have exported the channel
         * between our existence check and the export.
         */
        if (saved_errno == EBUSY && access(path, F_OK) == 0) {
            return 0;
        }

        errno = saved_errno;
        return -1;
    }

    exported_by_us[channel] = true;
    *newly_exported = true;

    return 0;
}

/* Release only channels exported by our process. */
int pwm_release(int channel)
{
    char number[12];
    int disable_error = 0;

    if (validate_channel(channel) != 0) {
        return -1;
    }

    if (!exported_by_us[channel]) {
        return 0;
    }

    if (write_field(channel, "enable", "0") != 0) {
        disable_error = errno;
    }

    (void)snprintf(number, sizeof(number), "%d", channel);

    if (sysfs_write_text(PWM_CHIP_PATH "/unexport", number) != 0) {
        return -1;
    }

    exported_by_us[channel] = false;

    if (disable_error != 0) {
        errno = disable_error;
        return -1;
    }

    return 0;
}

/* Configure frequency and duty cycle using sysfs. */
int pwm_set(int channel, int freq, int duty_percent)
{
    bool newly_exported;
    int current_period;
    int current_duty;
    int current_enabled;
    char text[24];

    if (freq <= 0 || NSEC_PER_SEC / freq == 0) {
        errno = EINVAL;
        return -1;
    }

    if (validate_channel(channel) != 0) {
        return -1;
    }

    if (duty_percent < 0) {
        duty_percent = 0;
    }

    if (duty_percent > 100) {
        duty_percent = 100;
    }

    int period_ns = (int)(NSEC_PER_SEC / freq);
    int duty_ns = (int)(
        ((int64_t)period_ns * duty_percent) / 100
    );

    if (export_channel(channel, &newly_exported) != 0) {
        return -1;
    }

    if (read_field(channel, "period", &current_period) != 0 ||
        read_field(channel, "duty_cycle", &current_duty) != 0) {
        goto fail;
    }

    /*
     * A newly exported PWM may have period = 0.
     * Do not change its enable state until a valid
     * period has been configured.
     */
    if (current_period > 0) {

        if (read_field(
                channel, "enable", &current_enabled
            ) != 0) {
            goto fail;
        }

        if (current_enabled != 0 &&
            write_field(channel, "enable", "0") != 0) {
            goto fail;
        }

        /*
         * The previous duty cycle must not exceed
         * the new period.
         */
        if (current_duty > period_ns &&
            write_field(channel, "duty_cycle", "0") != 0) {
            goto fail;
        }

    } else if (current_duty != 0) {
        errno = EINVAL;
        goto fail;
    }

    (void)snprintf(
        text, sizeof(text), "%d", period_ns
    );

    if (write_field(channel, "period", text) != 0) {
        goto fail;
    }

    (void)snprintf(
        text, sizeof(text), "%d", duty_ns
    );

    if (write_field(channel, "duty_cycle", text) != 0) {
        goto fail;
    }

    if (write_field(
            channel,
            "enable",
            duty_ns > 0 ? "1" : "0"
        ) != 0) {
        goto fail;
    }

    return 0;

fail:
    {
        int saved_errno = errno;

        if (newly_exported) {
            (void)pwm_release(channel);
        } else {
            (void)write_field(channel, "enable", "0");
        }

        errno = saved_errno;
        return -1;
    }
}
