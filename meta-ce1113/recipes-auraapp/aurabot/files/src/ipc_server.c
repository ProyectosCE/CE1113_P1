#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <aurabot_protocol.h>

#include "audio_controller.h"
#include "hardware.h"
#include "control_lease.h"
#include "ipc_server.h"
#include "robot_controller.h"

static int set_nonblocking(int descriptor)
{
    int flags = fcntl(descriptor, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
}

int ipc_server_init(ipc_server_t *server)
{
    struct sockaddr_un address;
    unsigned int index;
    if (server == NULL) { errno = EINVAL; return -1; }
    server->listener = -1;
    for (index = 0; index < AURABOT_MAX_CLIENTS; ++index)
        server->clients[index] = -1;
    if (mkdir(AURABOT_RUNTIME_DIRECTORY, 0750) != 0 && errno != EEXIST)
        return -1;
    server->listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (server->listener < 0) return -1;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, AURABOT_SOCKET_PATH);
    unlink(AURABOT_SOCKET_PATH);
    if (bind(server->listener, (struct sockaddr *)&address, sizeof(address)) != 0) {
        int saved_errno = errno;
        ipc_server_shutdown(server);
        errno = saved_errno;
        return -1;
    }
    if (chmod(AURABOT_SOCKET_PATH, 0660) != 0) {
        int saved_errno = errno;
        ipc_server_shutdown(server);
        errno = saved_errno;
        return -1;
    }
    if (listen(server->listener, AURABOT_MAX_CLIENTS) != 0) {
        int saved_errno = errno;
        ipc_server_shutdown(server);
        errno = saved_errno;
        return -1;
    }
    if (set_nonblocking(server->listener) != 0) {
        int saved_errno = errno;
        ipc_server_shutdown(server);
        errno = saved_errno;
        return -1;
    }
    return 0;
}

void ipc_server_shutdown(ipc_server_t *server)
{
    unsigned int index;
    if (server == NULL) return;
    for (index = 0; index < AURABOT_MAX_CLIENTS; ++index) {
        if (server->clients[index] >= 0) close(server->clients[index]);
        server->clients[index] = -1;
    }
    if (server->listener >= 0) close(server->listener);
    server->listener = -1;
    unlink(AURABOT_SOCKET_PATH);
}

static void accept_clients(ipc_server_t *server)
{
    int client;
    unsigned int index;
    for (;;) {
        client = accept4(server->listener, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (client < 0) return;
        for (index = 0; index < AURABOT_MAX_CLIENTS; ++index)
            if (server->clients[index] < 0) break;
        if (index == AURABOT_MAX_CLIENTS) close(client);
        else server->clients[index] = client;
    }
}

static void serialize_status(const aurabot_status_t *status,
                             unsigned char *payload)
{
    unsigned int offset = 0U;
#define WRITE_INT(value) do { aurabot_wire_put_int(payload + offset, (int)(value)); offset += 4U; } while (0)
    WRITE_INT(status->mode);
    WRITE_INT(status->auto_state);
    WRITE_INT(status->left_speed);
    WRITE_INT(status->right_speed);
    WRITE_INT(status->left_motor_movement);
    WRITE_INT(status->right_motor_movement);
    WRITE_INT(status->left_motor_direction);
    WRITE_INT(status->right_motor_direction);
    WRITE_INT(status->left_obstacle);
    WRITE_INT(status->right_obstacle);
    WRITE_INT(status->x_mm);
    WRITE_INT(status->y_mm);
    WRITE_INT(status->heading_mrad);
    WRITE_INT(status->manual_control_busy);
    aurabot_wire_put_uint(payload + offset, status->map_revision); offset += 4U;
    WRITE_INT(status->audio_state);
    WRITE_INT(status->audio_volume_percent);
    WRITE_INT(status->audio_track_index);
#undef WRITE_INT
}

static int dispatch(aurabot_state_t *state, unsigned int command,
                    const unsigned char *request, unsigned int request_size,
                    unsigned char *response, unsigned int *response_size)
{
    int result;
    *response_size = 0U;
    switch (command) {
    case AURABOT_CMD_GET_CAPABILITIES: {
        aurabot_capabilities_t capabilities;
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        hardware_get_capabilities(&capabilities);
        aurabot_wire_put_int(response, capabilities.left_motor);
        aurabot_wire_put_int(response + 4, capabilities.right_motor);
        aurabot_wire_put_int(response + 8, capabilities.left_encoder);
        aurabot_wire_put_int(response + 12, capabilities.right_encoder);
        aurabot_wire_put_int(response + 16, capabilities.left_sensor);
        aurabot_wire_put_int(response + 20, capabilities.right_sensor);
        aurabot_wire_put_int(response + 24, capabilities.audio);
        *response_size = 28U;
        return AURABOT_OK;
    }
    case AURABOT_CMD_AUDIO_SET_DEVICE:
        if (request_size != 4U) return AURABOT_ERR_PROTOCOL;
        result = aurabot_wire_get_int(request);
        if (result < 0 || result > 31) return AURABOT_ERR_INVALID_ARGUMENT;
        return hardware_audio_set_device(result) == 0 ? AURABOT_OK : AURABOT_ERR_HARDWARE;
    case AURABOT_CMD_GET_STATUS:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        serialize_status(&state->public_status, response);
        *response_size = AURABOT_STATUS_PAYLOAD_SIZE;
        return AURABOT_OK;
    case AURABOT_CMD_SET_MODE:
        if (request_size != 4U && request_size != 4U + AURABOT_OWNER_TOKEN_SIZE)
            return AURABOT_ERR_PROTOCOL;
        if (state->owner_active && (request_size == 4U ||
            !control_lease_is_owner(state, request + 4))) return AURABOT_ERR_BUSY;
        return robot_controller_set_mode(state,
            (aurabot_mode_t)aurabot_wire_get_int(request));
    case AURABOT_CMD_DIGITAL_WRITE:
    case AURABOT_CMD_PWM_SET:
    case AURABOT_CMD_PWM_STOP:
        if (request_size != (command == AURABOT_CMD_PWM_SET ? 12U :
            command == AURABOT_CMD_DIGITAL_WRITE ? 8U : 4U))
            return AURABOT_ERR_PROTOCOL;
        if (state->public_status.mode != AURABOT_MODE_MANUAL)
            return AURABOT_ERR_WRONG_MODE;
        return hardware_test_gpio(aurabot_wire_get_int(request),
            request_size >= 8U ? aurabot_wire_get_int(request + 4) : 0,
            request_size == 12U ? aurabot_wire_get_int(request + 8) : 0,
            command == AURABOT_CMD_PWM_SET ? 1 :
            command == AURABOT_CMD_PWM_STOP ? 2 : 0);
    case AURABOT_CMD_CLAIM_CONTROL:
        if (request_size != AURABOT_OWNER_TOKEN_SIZE) return AURABOT_ERR_PROTOCOL;
        return robot_controller_claim(state, request);
    case AURABOT_CMD_CONTROL_HEARTBEAT:
        if (request_size != AURABOT_OWNER_TOKEN_SIZE) return AURABOT_ERR_PROTOCOL;
        return robot_controller_heartbeat(state, request);
    case AURABOT_CMD_RELEASE_CONTROL:
        if (request_size != AURABOT_OWNER_TOKEN_SIZE) return AURABOT_ERR_PROTOCOL;
        return robot_controller_release(state, request);
    case AURABOT_CMD_DRIVE:
        if (request_size != AURABOT_OWNER_TOKEN_SIZE + 8U)
            return AURABOT_ERR_PROTOCOL;
        return robot_controller_drive(state, request,
            aurabot_wire_get_int(request + AURABOT_OWNER_TOKEN_SIZE),
            aurabot_wire_get_int(request + AURABOT_OWNER_TOKEN_SIZE + 4U));
    case AURABOT_CMD_STOP:
        if (request_size != AURABOT_OWNER_TOKEN_SIZE) return AURABOT_ERR_PROTOCOL;
        return robot_controller_stop(state, request);
    case AURABOT_CMD_EMERGENCY_STOP:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        return robot_controller_emergency_stop(state);
    case AURABOT_CMD_GET_MAP:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        aurabot_wire_put_uint(response, AURABOT_MAP_WIDTH);
        aurabot_wire_put_uint(response + 4, AURABOT_MAP_HEIGHT);
        aurabot_wire_put_uint(response + 8, state->map.revision);
        result = map_copy(&state->map, response + 12, AURABOT_MAP_CELLS,
                          response_size);
        if (result == AURABOT_OK) *response_size += 12U;
        return result;
    case AURABOT_CMD_AUDIO_PLAY:
        if (request_size != 4U) return AURABOT_ERR_PROTOCOL;
        return audio_controller_play(state, aurabot_wire_get_uint(request));
    case AURABOT_CMD_AUDIO_PAUSE:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        return audio_controller_pause(state);
    case AURABOT_CMD_AUDIO_STOP:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        return audio_controller_stop(state);
    case AURABOT_CMD_AUDIO_SET_VOLUME:
        if (request_size != 4U) return AURABOT_ERR_PROTOCOL;
        result = aurabot_wire_get_int(request);
        return audio_controller_set_volume(state, result);
    case AURABOT_CMD_AUDIO_GET_TRACK_COUNT:
        if (request_size != 0U) return AURABOT_ERR_PROTOCOL;
        aurabot_wire_put_uint(response, audio_controller_track_count());
        *response_size = 4U;
        return AURABOT_OK;
    case AURABOT_CMD_AUDIO_GET_TRACK_NAME:
        if (request_size != 4U) return AURABOT_ERR_PROTOCOL;
        if (audio_controller_track_name(aurabot_wire_get_uint(request),
            (char *)response, AURABOT_TRACK_NAME_SIZE) != AURABOT_OK)
            return AURABOT_ERR_INVALID_ARGUMENT;
        *response_size = (unsigned int)strlen((char *)response) + 1U;
        return AURABOT_OK;
    default:
        return AURABOT_ERR_PROTOCOL;
    }
}

static int process_client(int descriptor, aurabot_state_t *state)
{
    unsigned char request[AURABOT_REQUEST_HEADER_SIZE + AURABOT_MAX_REQUEST_PAYLOAD];
    unsigned char response[AURABOT_RESPONSE_HEADER_SIZE + AURABOT_MAX_RESPONSE_PAYLOAD];
    unsigned int command = 0U;
    unsigned int request_id = 0U;
    unsigned int payload_size = 0U;
    unsigned int response_size = 0U;
    int received;
    int result = AURABOT_ERR_PROTOCOL;

    received = (int)recv(descriptor, request, sizeof(request), 0);
    if (received == 0) return -1;
    if (received < 0) return errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1;
    if (received >= (int)AURABOT_REQUEST_HEADER_SIZE) {
        command = aurabot_wire_get_uint(request + 8);
        request_id = aurabot_wire_get_uint(request + 12);
        payload_size = aurabot_wire_get_uint(request + 16);
        if (aurabot_wire_get_uint(request) == AURABOT_PROTOCOL_MAGIC &&
            aurabot_wire_get_uint(request + 4) == AURABOT_PROTOCOL_VERSION &&
            payload_size <= AURABOT_MAX_REQUEST_PAYLOAD &&
            (unsigned int)received == AURABOT_REQUEST_HEADER_SIZE + payload_size)
            result = dispatch(state, command,
                request + AURABOT_REQUEST_HEADER_SIZE, payload_size,
                response + AURABOT_RESPONSE_HEADER_SIZE, &response_size);
    }
    aurabot_wire_put_uint(response, AURABOT_PROTOCOL_MAGIC);
    aurabot_wire_put_uint(response + 4, AURABOT_PROTOCOL_VERSION);
    aurabot_wire_put_uint(response + 8, command);
    aurabot_wire_put_uint(response + 12, request_id);
    aurabot_wire_put_int(response + 16, result);
    aurabot_wire_put_uint(response + 20, response_size);
    if (send(descriptor, response, AURABOT_RESPONSE_HEADER_SIZE + response_size,
             MSG_NOSIGNAL) < 0)
        return -1;
    return 0;
}

int ipc_server_process(ipc_server_t *server, aurabot_state_t *state,
                       int timeout_ms)
{
    struct pollfd descriptors[AURABOT_MAX_CLIENTS + 1];
    unsigned int index;
    int ready;
    if (server == NULL || state == NULL) { errno = EINVAL; return -1; }
    descriptors[0].fd = server->listener;
    descriptors[0].events = POLLIN;
    for (index = 0; index < AURABOT_MAX_CLIENTS; ++index) {
        descriptors[index + 1].fd = server->clients[index];
        descriptors[index + 1].events = POLLIN;
    }
    ready = poll(descriptors, AURABOT_MAX_CLIENTS + 1, timeout_ms);
    if (ready < 0) return errno == EINTR ? 0 : -1;
    if (descriptors[0].revents & POLLIN) accept_clients(server);
    for (index = 0; index < AURABOT_MAX_CLIENTS; ++index) {
        if (server->clients[index] < 0) continue;
        if (descriptors[index + 1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            close(server->clients[index]);
            server->clients[index] = -1;
        } else if ((descriptors[index + 1].revents & POLLIN) &&
                   process_client(server->clients[index], state) != 0) {
            close(server->clients[index]);
            server->clients[index] = -1;
        }
    }
    return 0;
}
