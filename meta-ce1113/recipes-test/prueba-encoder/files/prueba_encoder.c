#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#include <libgpio.h>

#define ENCODER_GPIO_PIN 2
#define MOVEMENT_TIMEOUT_MS 50LL
#define POLL_INTERVAL_US 1000L

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static long long elapsed_milliseconds(const struct timespec *start,
                                      const struct timespec *end)
{
    return (long long)(end->tv_sec - start->tv_sec) * 1000LL +
        (end->tv_nsec - start->tv_nsec) / 1000000LL;
}

static int install_signal_handlers(void)
{
    struct sigaction action = {0};

    action.sa_handler = handle_signal;
    if (sigemptyset(&action.sa_mask) != 0 ||
        sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        return -1;
    }
    return 0;
}

int main(void)
{
    struct timespec last_pulse = {0};
    const struct timespec poll_delay = {
        .tv_sec = 0,
        .tv_nsec = POLL_INTERVAL_US * 1000L,
    };
    int previous_level;
    int previous_movement = -1;
    int pulse_received = 0;
    int exit_status = 0;

    if (install_signal_handlers() != 0) {
        perror("Error configurando las senales");
        return 1;
    }

    if (pinMode(ENCODER_GPIO_PIN, "in") != 0) {
        perror("Error configurando el GPIO del encoder");
        return 1;
    }

    previous_level = digitalRead(ENCODER_GPIO_PIN);
    if (previous_level < 0) {
        perror("Error leyendo el GPIO del encoder");
        return 1;
    }

    printf("Prueba del encoder iniciada en GPIO BCM %d... "
           "(Ctrl+C para salir)\n", ENCODER_GPIO_PIN);

    while (running) {
        struct timespec now;
        int current_level;
        int is_moving;

        current_level = digitalRead(ENCODER_GPIO_PIN);
        if (current_level < 0) {
            perror("Error leyendo el GPIO del encoder");
            exit_status = 1;
            break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("Error obteniendo el tiempo monotono");
            exit_status = 1;
            break;
        }

        /* El encoder es activo en bajo: cada transición 1 -> 0 es un pulso. */
        if (previous_level == 1 && current_level == 0) {
            last_pulse = now;
            pulse_received = 1;
        }

        is_moving = pulse_received &&
            elapsed_milliseconds(&last_pulse, &now) < MOVEMENT_TIMEOUT_MS;
        if (is_moving != previous_movement) {
            printf("Estado: %s\n", is_moving ? "AVANZANDO" : "QUIETO");
            fflush(stdout);
            previous_movement = is_moving;
        }

        previous_level = current_level;
        if (nanosleep(&poll_delay, NULL) != 0 && errno != EINTR) {
            perror("Error esperando la siguiente lectura");
            exit_status = 1;
            break;
        }
    }

    printf("Fin.\n");
    return exit_status;
}
