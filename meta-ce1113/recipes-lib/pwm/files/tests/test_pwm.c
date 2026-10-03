#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <libpwm.h>

#include "fake_gpio.h"

static void sleep_ms(long milliseconds)
{
    struct timespec remaining = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = milliseconds % 1000 * 1000000L,
    };
    while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR) {
    }
}

static int require(int condition, const char *message)
{
    if (condition) {
        return 0;
    }
    fprintf(stderr, "test_pwm: %s\n", message);
    return -1;
}

int main(void)
{
    fake_gpio_reset();
    if (require(setPWM(17, 100, 50) == 0, "setPWM rechazó un canal válido") ||
        (sleep_ms(80), 0) ||
        require(fake_gpio_write_count(1) >= 4, "faltan transiciones altas") ||
        require(fake_gpio_write_count(0) >= 4, "faltan transiciones bajas") ||
        require(stopPWM(17) == 0, "stopPWM falló") ||
        require(fake_gpio_current_level() == 0, "stopPWM no dejó la salida baja")) {
        return 1;
    }

    fake_gpio_reset();
    if (require(setPWM(18, 100, 50) == 0, "no inició la prueba de recuperación")) {
        return 1;
    }
    sleep_ms(30);
    int writes_before_failure = fake_gpio_write_count(0) +
                                fake_gpio_write_count(1);
    fake_gpio_fail_next_writes(4);
    sleep_ms(120);
    int writes_after_failure = fake_gpio_write_count(0) +
                               fake_gpio_write_count(1);
    if (require(writes_after_failure > writes_before_failure + 2,
                "el hilo no se recuperó de errores GPIO transitorios") ||
        require(stopPWM(18) == 0, "stopPWM falló después de recuperarse")) {
        return 1;
    }

    /* El caso real usa 2 Hz. En ARM de 32 bits, period_ns * 50 desborda si
     * accidentalmente vuelve a calcularse con long en lugar de int64_t. */
    fake_gpio_reset();
    if (require(setPWM(20, 2, 50) == 0, "no inició PWM a 2 Hz")) {
        return 1;
    }
    sleep_ms(100);
    if (require(fake_gpio_current_level() == 1,
                "el pulso alto de 2 Hz terminó demasiado pronto")) {
        return 1;
    }
    sleep_ms(250);
    if (require(fake_gpio_current_level() == 0,
                "el PWM de 2 Hz no alcanzó el nivel bajo") ||
        require(stopPWM(20) == 0, "stopPWM falló a 2 Hz")) {
        return 1;
    }

    errno = 0;
    if (require(setPWM(19, 1000000001, 50) == -1 && errno == EINVAL,
                "aceptó una frecuencia que genera un período de cero")) {
        return 1;
    }

    return 0;
}
