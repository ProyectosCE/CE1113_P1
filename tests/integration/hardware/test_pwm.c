#include <stdio.h>
#include <unistd.h>

#include "libgpio.h"
#include "libpwm.h"

#ifndef PWM_TEST_SYSFS_ROOT
#define PWM_TEST_SYSFS_ROOT "/sys/class/pwm"
#endif

/* Read one PWM attribute to display the applied configuration. */
static int read_attribute(
    const char *attribute,
    int *value
)
{
    char path[256];

    int length = snprintf(
        path,
        sizeof(path),
        PWM_TEST_SYSFS_ROOT "/pwmchip0/pwm0/%s",
        attribute
    );

    if (length < 0 || (size_t)length >= sizeof(path)) {
        return -1;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return -1;
    }

    if (fscanf(file, "%d", value) != 1) {
        (void)fclose(file);
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}

/* Display the configuration currently reported by sysfs. */
static int print_state(void)
{
    int period;
    int duty;
    int enabled;

    if (read_attribute("period", &period) != 0 ||
        read_attribute("duty_cycle", &duty) != 0 ||
        read_attribute("enable", &enabled) != 0) {

        fprintf(stderr, "Error leyendo el estado PWM\n");
        return -1;
    }

    printf(
        "Periodo: %d ns | Duty: %d ns | Enable: %d\n",
        period,
        duty,
        enabled
    );

    return 0;
}

int main(void)
{
    int result = 1;
    int started = 0;

    puts("Iniciando prueba PWM en el canal 0");

    /* Test 1: configure PWM at 1000 Hz and 25%. */
    if (pwm_set(0, 1000, 25) != 0) {
        perror("Error configurando PWM al 25%");
        return 1;
    }

    started = 1;

    puts("\nPrueba 1: 1000 Hz, 25%");

    if (print_state() != 0) {
        goto cleanup;
    }

    sleep(1);

    /*
     * Test 2: exercise the compatibility function
     * provided by libgpio.
     */
    if (setPWM(0, 500, 75) != 0) {
        perror("Error utilizando setPWM");
        goto cleanup;
    }

    puts("\nPrueba 2: 500 Hz, 75%");

    if (print_state() != 0) {
        goto cleanup;
    }

    sleep(1);

    /*
     * Test 3: return to a shorter period and stop.
     * The previous duty cycle exceeds the new period.
     */
    if (pwm_set(0, 1000, 0) != 0) {
        perror("Error deteniendo PWM");
        goto cleanup;
    }

    puts("\nPrueba 3: 1000 Hz, 0%");

    if (print_state() != 0) {
        goto cleanup;
    }

    result = 0;

cleanup:
    if (started && result != 0) {
        (void)pwm_set(0, 1000, 0);
    }

    if (started && pwm_release(0) != 0) {
        perror("Error liberando PWM");
        result = 1;
    }

    return result;
}