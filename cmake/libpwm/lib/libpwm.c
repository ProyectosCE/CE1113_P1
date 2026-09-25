#include <stdio.h>
#include <stdlib.h>

#include "libpwm.h"

#define PWM_CHIP 0
#define NSEC_PER_SEC 1000000000LL

/*
 * Existing PWM implementation.
 * Direct sysfs file operations will be implemented after
 */
int pwm_set(int pin, int freq, int duty_percent)
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

    snprintf(export_command, sizeof(export_command),
             "echo %d > /sys/class/pwm/pwmchip%d/export",
             pin, PWM_CHIP);

    snprintf(period_command, sizeof(period_command),
             "echo %lld > /sys/class/pwm/pwmchip%d/pwm%d/period",
             period_ns, PWM_CHIP, pin);

    snprintf(duty_command, sizeof(duty_command),
             "echo %lld > /sys/class/pwm/pwmchip%d/pwm%d/duty_cycle",
             duty_ns, PWM_CHIP, pin);

    snprintf(enable_command, sizeof(enable_command),
             "echo %d > /sys/class/pwm/pwmchip%d/pwm%d/enable",
             duty_percent > 0 ? 1 : 0, PWM_CHIP, pin);

    int export_status = system(export_command);
    (void)export_status;

    if (system(period_command) != 0 ||
        system(duty_command) != 0 ||
        system(enable_command) != 0) {
        return -1;
    }

    return 0;
}