
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "libgpio.h"

#define IR_PIN 17
#define LED_PIN 27

static volatile sig_atomic_t running = 1;

static void stopTest(int signal_number)
{
    (void)signal_number;
    running = 0;
}

int main(void)
{
    int result = 0;
    int led_ready = 0;
    int last_value = -1;

    const struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 200000000L
    };

    if (signal(SIGINT, stopTest) == SIG_ERR ||
        signal(SIGTERM, stopTest) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    if (pinMode(IR_PIN, "in") != 0) {
        perror("pinMode IR");
        return 1;
    }

    if (pinMode(LED_PIN, "out") != 0) {
        perror("pinMode LED");
        result = 1;
        goto cleanup;
    }
    led_ready = 1;

    if (digitalWrite(LED_PIN, 0) != 0) {
        perror("LED apagado inicial");
        result = 1;
        goto cleanup;
    }

    puts("Prueba IR + LED iniciada. Presiona Ctrl+C para terminar.");

    while (running) {
        int ir_value = digitalRead(IR_PIN);
        if (ir_value < 0) {
            perror("digitalRead IR");
            result = 1;
            break;
        }

        int obstacle = (ir_value == 0);

        if (digitalWrite(LED_PIN, obstacle) != 0) {
            perror("digitalWrite LED");
            result = 1;
            break;
        }

        if (ir_value != last_value) {
            printf("IR=%d -> %s; LED=%s\n", ir_value,
                   obstacle ? "OBSTACULO" : "LIBRE",
                   obstacle ? "ENCENDIDO" : "APAGADO");

            last_value = ir_value;
        }

        if (nanosleep(&delay, NULL) != 0 && errno != EINTR) {
            perror("nanosleep");
            result = 1;
            break;
        }
    }

cleanup:
    if (led_ready && digitalWrite(LED_PIN, 0) != 0) {
        perror("apagar LED al salir");
        result = 1;
    }

    if (gpioRelease(LED_PIN) != 0) {
        perror("gpioRelease LED");
        result = 1;
    }

    if (gpioRelease(IR_PIN) != 0) {
        perror("gpioRelease IR");
        result = 1;
    }

    puts("Prueba finalizada.");
    return result;
}
