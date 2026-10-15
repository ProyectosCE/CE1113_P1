#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <aurabot_protocol.h>

#include "aurabot_config.h"
#include "ipc_server.h"
#include "robot_controller.h"

static volatile sig_atomic_t running = 1;

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    running = 0;
}

int main(void)
{
    aurabot_state_t state;
    ipc_server_t server;
    int exit_code = EXIT_SUCCESS;
    int lock_fd;
    aurabot_mode_t previous_mode;
    aurabot_auto_state_t previous_auto;

    lock_fd = open(AURABOT_RUNTIME_DIRECTORY ".lock", O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lock_fd < 0 || flock(lock_fd, LOCK_EX | LOCK_NB) != 0) {
        perror("aurabot: otro proceso activo o bloqueo no disponible");
        if (lock_fd >= 0) close(lock_fd);
        return 4;
    }

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);
    if (robot_controller_init(&state) != AURABOT_OK) {
        fprintf(stderr, "aurabot: no se pudo inicializar el hardware\n");
        if (state.initialized) robot_controller_shutdown(&state);
        close(lock_fd);
        return 2;
    }
    if (ipc_server_init(&server) != 0) {
        perror("aurabot: IPC");
        robot_controller_shutdown(&state);
        close(lock_fd);
        return 3;
    }
    fprintf(stderr, "aurabot: listo en %s\n", AURABOT_SOCKET_PATH);
    previous_mode = state.public_status.mode;
    previous_auto = state.public_status.auto_state;
    fprintf(stderr, "aurabot: modo=%d estado autonomo=%d\n", previous_mode, previous_auto);
    while (running) {
        if (ipc_server_process(&server, &state,
                               AURABOT_CONTROL_TICK_MS) != 0) {
            perror("aurabot: servidor IPC");
            exit_code = 3;
            break;
        }
        (void)robot_controller_tick(&state);
        if (previous_mode != state.public_status.mode ||
            previous_auto != state.public_status.auto_state) {
            fprintf(stderr, "aurabot: modo=%d auto=%d motores=%d,%d sensores=%d,%d\n",
                state.public_status.mode, state.public_status.auto_state,
                state.public_status.left_speed, state.public_status.right_speed,
                state.public_status.left_obstacle, state.public_status.right_obstacle);
            previous_mode = state.public_status.mode;
            previous_auto = state.public_status.auto_state;
        }
    }
    ipc_server_shutdown(&server);
    robot_controller_shutdown(&state);
    close(lock_fd);
    return exit_code;
}
