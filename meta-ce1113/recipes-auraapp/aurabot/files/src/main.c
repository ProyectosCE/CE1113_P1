#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libgpio.h>
#include <libpwm.h>

#define CONTROL_DIRECTORY "/run/aurabot"
#define CONTROL_FIFO CONTROL_DIRECTORY "/control"

static volatile sig_atomic_t running = 1;

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static int execute_command(char *command)
{
    int pin, frequency, duty, value;
    char extra;

    command[strcspn(command, "\r\n")] = '\0';
    if (sscanf(command, "PWM %d %d %d %c", &pin, &frequency, &duty, &extra) == 3) {
        return setPWM(pin, frequency, duty);
    }
    if (sscanf(command, "PWMSTOP %d %c", &pin, &extra) == 1) {
        return stopPWM(pin);
    }
    if (sscanf(command, "DIGITAL %d %d %c", &pin, &value, &extra) == 2 &&
        (value == 0 || value == 1)) {
        if (stopPWM(pin) != 0) return -1;
        return pinMode(pin, "out") == 0 ? digitalWrite(pin, value) : -1;
    }
    errno = EINVAL;
    return -1;
}

int main(void)
{
    struct pollfd poll_fd;
    char commands[256];

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);
    if (mkdir(CONTROL_DIRECTORY, 0755) != 0 && errno != EEXIST) {
        perror("aurabot: mkdir");
        return EXIT_FAILURE;
    }
    unlink(CONTROL_FIFO);
    if (mkfifo(CONTROL_FIFO, 0666) != 0 || chmod(CONTROL_FIFO, 0666) != 0) {
        perror("aurabot: fifo");
        return EXIT_FAILURE;
    }
    poll_fd.fd = open(CONTROL_FIFO, O_RDWR | O_NONBLOCK);
    poll_fd.events = POLLIN;
    if (poll_fd.fd < 0) {
        perror("aurabot: open");
        unlink(CONTROL_FIFO);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Servidor de hardware listo en %s\n", CONTROL_FIFO);
    while (running) {
        int ready = poll(&poll_fd, 1, 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("aurabot: poll");
            break;
        }
        if (ready > 0 && (poll_fd.revents & POLLIN)) {
            ssize_t count = read(poll_fd.fd, commands, sizeof(commands) - 1);
            if (count > 0) {
                char *save_pointer = NULL;
                char *command;
                commands[count] = '\0';
                command = strtok_r(commands, "\r\n", &save_pointer);
                while (command != NULL) {
                    if (execute_command(command) != 0) {
                        fprintf(stderr, "Orden de hardware fallida: %s (%s)\n",
                                command, strerror(errno));
                    }
                    command = strtok_r(NULL, "\r\n", &save_pointer);
                }
            }
        }
    }
    close(poll_fd.fd);
    unlink(CONTROL_FIFO);
    return EXIT_SUCCESS;
}
