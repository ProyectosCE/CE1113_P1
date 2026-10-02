#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "../include/libgpio.h"

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_BASE 512

#define PWM_CHIP 0
#define NSEC_PER_SEC 1000000000LL
#define MAX_GPIO 64

static PWMData pwmData[MAX_GPIO];
static pthread_t pwmThreads[MAX_GPIO];
static int pwmStarted[MAX_GPIO] = {0};
static pthread_mutex_t pwmMutex = PTHREAD_MUTEX_INITIALIZER;


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
    int export_status = system(export_command);
    (void)export_status;

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


void *pwmThread(void *arg)
{
    PWMData *data = (PWMData *)arg;

    while (1) {
        pthread_mutex_lock(&pwmMutex);

        int freq = data->freq;
        int duty = data->duty;
        int pin = data->pin;

        pthread_mutex_unlock(&pwmMutex);

        long period_ns = 1000000000L / freq;
        long high_ns = period_ns * duty / 100;
        long low_ns = period_ns - high_ns;

        if (duty == 0) {
            digitalWrite(pin, 0);
            struct timespec t = {0, period_ns};
            nanosleep(&t, NULL);
        }
        else if (duty == 100) {
            digitalWrite(pin, 1);
            struct timespec t = {0, period_ns};
            nanosleep(&t, NULL);
        }
        else {
            digitalWrite(pin, 1);
            struct timespec high = {
                high_ns / 1000000000L,
                high_ns % 1000000000L
            };
            nanosleep(&high, NULL);

            digitalWrite(pin, 0);
            struct timespec low = {
                low_ns / 1000000000L,
                low_ns % 1000000000L
            };
            nanosleep(&low, NULL);
        }
    }

    return NULL;
}

int setPWM(int pin, int freq, int duty_percent)
{
    if (pin < 0 || pin >= MAX_GPIO || freq <= 0) {
        return -1;
    }

    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;

    if (pinMode(pin, "out") != 0) {
        return -1;
    }

    pthread_mutex_lock(&pwmMutex);

    pwmData[pin].pin = pin;
    pwmData[pin].freq = freq;
    pwmData[pin].duty = duty_percent;

    if (!pwmStarted[pin]) {
        pwmStarted[pin] = 1;

        if (pthread_create(&pwmThreads[pin], NULL,
                           pwmThread, &pwmData[pin]) != 0) {
            pwmStarted[pin] = 0;
            pthread_mutex_unlock(&pwmMutex);
            return -1;
        }

        pthread_detach(pwmThreads[pin]);
    }

    pthread_mutex_unlock(&pwmMutex);

    return 0;
}
