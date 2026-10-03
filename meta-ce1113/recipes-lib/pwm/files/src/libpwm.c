#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libgpio.h>
#include <libpwm.h>

#define MAX_GPIO 64
#define NSEC_PER_SEC INT64_C(1000000000)

typedef struct {
    int pin;
    int frequency_hz;
    int duty_percent;
    int running;
    int started;
} pwm_channel_t;

static pwm_channel_t channels[MAX_GPIO];
static pthread_t threads[MAX_GPIO];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int write_level(int pin, int level)
{
    if (digitalWrite(pin, level) == 0) {
        return 0;
    }
    if (pinMode(pin, "out") != 0) {
        return -1;
    }
    return digitalWrite(pin, level);
}

static int sleep_ns(int64_t duration_ns)
{
    struct timespec duration = {
        .tv_sec = duration_ns / NSEC_PER_SEC,
        .tv_nsec = duration_ns % NSEC_PER_SEC,
    };

    while (nanosleep(&duration, &duration) != 0) {
        if (errno != EINTR) {
            return -1;
        }
    }
    return 0;
}

static void report_gpio_error(int pin, int level)
{
    int saved_errno = errno;
    fprintf(stderr, "libpwm: GPIO%d no aceptó el nivel %d: %s\n",
            pin, level, strerror(saved_errno));
}

static void *pwm_thread(void *argument)
{
    pwm_channel_t *channel = argument;
    int gpio_failure_reported = 0;

    for (;;) {
        pthread_mutex_lock(&lock);
        int pin = channel->pin;
        int frequency_hz = channel->frequency_hz;
        int duty_percent = channel->duty_percent;
        int running = channel->running;
        pthread_mutex_unlock(&lock);

        if (!running) {
            (void)write_level(pin, 0);
            return NULL;
        }

        /* La imagen raspberrypi4 es ARM de 32 bits: long no alcanza para
         * period_ns * duty_percent a frecuencias bajas como 2 Hz. */
        int64_t period_ns = NSEC_PER_SEC / frequency_hz;
        int64_t high_ns = period_ns * duty_percent / 100;
        int64_t low_ns = period_ns - high_ns;

        if (high_ns > 0 && write_level(pin, 1) != 0) {
            /* sysfs puede fallar de forma transitoria mientras termina un
             * export/unexport. El PWM funcional de referencia seguía vivo;
             * aquí conservamos esa resiliencia sin ocultar el diagnóstico. */
            if (!gpio_failure_reported) {
                report_gpio_error(pin, 1);
                gpio_failure_reported = 1;
            }
            (void)sleep_ns(period_ns);
            continue;
        }
        if (high_ns > 0 && sleep_ns(high_ns) != 0) {
            perror("libpwm: falló la espera del nivel alto");
            continue;
        }
        if (low_ns > 0 && write_level(pin, 0) != 0) {
            if (!gpio_failure_reported) {
                report_gpio_error(pin, 0);
                gpio_failure_reported = 1;
            }
            (void)sleep_ns(period_ns);
            continue;
        }
        if (low_ns > 0 && sleep_ns(low_ns) != 0) {
            perror("libpwm: falló la espera del nivel bajo");
        }
        gpio_failure_reported = 0;
    }
}

int setPWM(int pin, int frequency_hz, int duty_percent)
{
    if (pin < 0 || pin >= MAX_GPIO || frequency_hz <= 0 ||
        frequency_hz > NSEC_PER_SEC ||
        duty_percent < 0 || duty_percent > 100) {
        errno = EINVAL;
        return -1;
    }
    if (pinMode(pin, "out") != 0 ||
        write_level(pin, duty_percent > 0 ? 1 : 0) != 0) {
        return -1;
    }

    pthread_mutex_lock(&lock);
    channels[pin].pin = pin;
    channels[pin].frequency_hz = frequency_hz;
    channels[pin].duty_percent = duty_percent;
    channels[pin].running = 1;

    if (!channels[pin].started) {
        channels[pin].started = 1;
        int result = pthread_create(&threads[pin], NULL,
                                    pwm_thread, &channels[pin]);
        if (result != 0) {
            channels[pin].started = 0;
            channels[pin].running = 0;
            pthread_mutex_unlock(&lock);
            errno = result;
            return -1;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int stopPWM(int pin)
{
    if (pin < 0 || pin >= MAX_GPIO) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&lock);
    if (!channels[pin].started) {
        pthread_mutex_unlock(&lock);
        return write_level(pin, 0);
    }
    channels[pin].running = 0;
    pthread_t thread = threads[pin];
    pthread_mutex_unlock(&lock);

    int result = pthread_join(thread, NULL);
    if (result != 0) {
        errno = result;
        return -1;
    }

    pthread_mutex_lock(&lock);
    channels[pin].started = 0;
    pthread_mutex_unlock(&lock);
    return write_level(pin, 0);
}
