#ifndef AURABOT_IPC_SERVER_H
#define AURABOT_IPC_SERVER_H

#include "aurabot_state.h"

#define AURABOT_MAX_CLIENTS 8

typedef struct {
    int listener;
    int clients[AURABOT_MAX_CLIENTS];
} ipc_server_t;

int ipc_server_init(ipc_server_t *server);
void ipc_server_shutdown(ipc_server_t *server);
int ipc_server_process(ipc_server_t *server, aurabot_state_t *state,
                       int timeout_ms);

#endif
