#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <aurabot_protocol.h>

#include "fsm.h"
#include "ipc_server.h"

#define CONTROL_TICK_MS 25

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

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);
    if (fsm_init(&state) != AURABOT_OK) {
        fprintf(stderr, "aurabot: no se pudo inicializar el hardware\n");
        return 2;
    }
    if (ipc_server_init(&server) != 0) {
        perror("aurabot: IPC");
        fsm_shutdown(&state);
        return 3;
    }
    fprintf(stderr, "aurabot: listo en %s\n", AURABOT_SOCKET_PATH);
    while (running) {
        if (ipc_server_process(&server, &state, CONTROL_TICK_MS) != 0) {
            perror("aurabot: servidor IPC");
            exit_code = 3;
            break;
        }
        (void)fsm_tick(&state);
    }
    ipc_server_shutdown(&server);
    fsm_shutdown(&state);
    return exit_code;
}
