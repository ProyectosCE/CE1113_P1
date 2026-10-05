#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <aurabot.h>
#include <aurabot_protocol.h>

#define AURABOT_IO_TIMEOUT_MS 1000

static int connection_fd = -1;
static unsigned int next_request_id = 1U;

static int map_errno_to_result(void)
{
    if (errno == EINVAL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (errno == ETIMEDOUT || errno == ENOENT || errno == ECONNREFUSED)
        return AURABOT_ERR_UNAVAILABLE;
    return AURABOT_ERR_PROTOCOL;
}

static int wait_for_io(short events)
{
    struct pollfd descriptor;
    int result;

    descriptor.fd = connection_fd;
    descriptor.events = events;
    descriptor.revents = 0;
    do {
        result = poll(&descriptor, 1, AURABOT_IO_TIMEOUT_MS);
    } while (result < 0 && errno == EINTR);
    if (result == 0) {
        errno = ETIMEDOUT;
        return -1;
    }
    if (result < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) {
        if (result >= 0) errno = ECONNRESET;
        return -1;
    }
    return 0;
}

int aurabot_connect(void)
{
    struct sockaddr_un address;

    if (connection_fd >= 0) return AURABOT_OK;
    connection_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (connection_fd < 0) return map_errno_to_result();

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    if (strlen(AURABOT_SOCKET_PATH) >= sizeof(address.sun_path)) {
        close(connection_fd);
        connection_fd = -1;
        return AURABOT_ERR_PROTOCOL;
    }
    strcpy(address.sun_path, AURABOT_SOCKET_PATH);
    if (connect(connection_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        int result = map_errno_to_result();
        close(connection_fd);
        connection_fd = -1;
        return result;
    }
    return AURABOT_OK;
}

void aurabot_disconnect(void)
{
    if (connection_fd >= 0) close(connection_fd);
    connection_fd = -1;
}

static int transact(unsigned int command, const unsigned char *payload,
                    unsigned int payload_size, unsigned char *response_payload,
                    unsigned int response_capacity,
                    unsigned int *response_size)
{
    unsigned char request[AURABOT_REQUEST_HEADER_SIZE + AURABOT_MAX_REQUEST_PAYLOAD];
    unsigned char response[AURABOT_RESPONSE_HEADER_SIZE + AURABOT_MAX_RESPONSE_PAYLOAD];
    unsigned int request_id;
    unsigned int received_payload_size;
    int received;
    int status;

    if (payload_size > AURABOT_MAX_REQUEST_PAYLOAD ||
        (payload_size > 0U && payload == NULL))
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (connection_fd < 0) {
        int result = aurabot_connect();
        if (result != AURABOT_OK) return result;
    }

    request_id = next_request_id++;
    if (next_request_id == 0U) next_request_id = 1U;
    aurabot_wire_put_uint(request + 0, AURABOT_PROTOCOL_MAGIC);
    aurabot_wire_put_uint(request + 4, AURABOT_PROTOCOL_VERSION);
    aurabot_wire_put_uint(request + 8, command);
    aurabot_wire_put_uint(request + 12, request_id);
    aurabot_wire_put_uint(request + 16, payload_size);
    if (payload_size > 0U)
        memcpy(request + AURABOT_REQUEST_HEADER_SIZE, payload, payload_size);

    if (wait_for_io(POLLOUT) != 0 ||
        send(connection_fd, request, AURABOT_REQUEST_HEADER_SIZE + payload_size,
             MSG_NOSIGNAL) != (int)(AURABOT_REQUEST_HEADER_SIZE + payload_size)) {
        aurabot_disconnect();
        return map_errno_to_result();
    }
    if (wait_for_io(POLLIN) != 0) {
        aurabot_disconnect();
        return map_errno_to_result();
    }
    received = (int)recv(connection_fd, response, sizeof(response), 0);
    if (received < (int)AURABOT_RESPONSE_HEADER_SIZE) {
        aurabot_disconnect();
        return AURABOT_ERR_PROTOCOL;
    }
    received_payload_size = aurabot_wire_get_uint(response + 20);
    if (aurabot_wire_get_uint(response + 0) != AURABOT_PROTOCOL_MAGIC ||
        aurabot_wire_get_uint(response + 4) != AURABOT_PROTOCOL_VERSION ||
        aurabot_wire_get_uint(response + 8) != command ||
        aurabot_wire_get_uint(response + 12) != request_id ||
        received_payload_size > AURABOT_MAX_RESPONSE_PAYLOAD ||
        (unsigned int)received != AURABOT_RESPONSE_HEADER_SIZE + received_payload_size) {
        aurabot_disconnect();
        return AURABOT_ERR_PROTOCOL;
    }
    status = aurabot_wire_get_int(response + 16);
    if (status != AURABOT_OK) return status;
    if (received_payload_size > response_capacity) {
        if (response_size != NULL) *response_size = received_payload_size;
        return AURABOT_ERR_INVALID_ARGUMENT;
    }
    if (received_payload_size > 0U && response_payload != NULL)
        memcpy(response_payload, response + AURABOT_RESPONSE_HEADER_SIZE,
               received_payload_size);
    if (response_size != NULL) *response_size = received_payload_size;
    return AURABOT_OK;
}

static int token_command(unsigned int command,
                         const unsigned char token[AURABOT_OWNER_TOKEN_SIZE])
{
    if (token == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    return transact(command, token, AURABOT_OWNER_TOKEN_SIZE, NULL, 0, NULL);
}

int aurabot_get_status(aurabot_status_t *status)
{
    unsigned char payload[AURABOT_STATUS_PAYLOAD_SIZE];
    unsigned int size;
    unsigned int offset = 0;
    int result;

    if (status == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    result = transact(AURABOT_CMD_GET_STATUS, NULL, 0, payload, sizeof(payload), &size);
    if (result != AURABOT_OK) return result;
    if (size != sizeof(payload)) return AURABOT_ERR_PROTOCOL;

#define READ_STATUS_INT(field) do { \
    status->field = aurabot_wire_get_int(payload + offset); offset += 4U; \
} while (0)
    status->mode = (aurabot_mode_t)aurabot_wire_get_int(payload + offset); offset += 4U;
    status->auto_state = (aurabot_auto_state_t)aurabot_wire_get_int(payload + offset); offset += 4U;
    READ_STATUS_INT(left_speed);
    READ_STATUS_INT(right_speed);
    READ_STATUS_INT(left_obstacle);
    READ_STATUS_INT(right_obstacle);
    READ_STATUS_INT(x_mm);
    READ_STATUS_INT(y_mm);
    READ_STATUS_INT(heading_mrad);
    READ_STATUS_INT(manual_control_busy);
    status->map_revision = aurabot_wire_get_uint(payload + offset); offset += 4U;
    status->audio_state = (aurabot_audio_state_t)aurabot_wire_get_int(payload + offset); offset += 4U;
    READ_STATUS_INT(audio_volume_percent);
    READ_STATUS_INT(audio_track_index);
#undef READ_STATUS_INT
    return AURABOT_OK;
}

int aurabot_set_mode(aurabot_mode_t mode)
{
    unsigned char payload[4];
    aurabot_wire_put_int(payload, (int)mode);
    return transact(AURABOT_CMD_SET_MODE, payload, sizeof(payload), NULL, 0, NULL);
}

int aurabot_claim_control(const unsigned char token[AURABOT_OWNER_TOKEN_SIZE])
{
    return token_command(AURABOT_CMD_CLAIM_CONTROL, token);
}

int aurabot_control_heartbeat(const unsigned char token[AURABOT_OWNER_TOKEN_SIZE])
{
    return token_command(AURABOT_CMD_CONTROL_HEARTBEAT, token);
}

int aurabot_release_control(const unsigned char token[AURABOT_OWNER_TOKEN_SIZE])
{
    return token_command(AURABOT_CMD_RELEASE_CONTROL, token);
}

int aurabot_drive(const unsigned char token[AURABOT_OWNER_TOKEN_SIZE],
                  int left_percent, int right_percent)
{
    unsigned char payload[AURABOT_OWNER_TOKEN_SIZE + 8];
    if (token == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    memcpy(payload, token, AURABOT_OWNER_TOKEN_SIZE);
    aurabot_wire_put_int(payload + AURABOT_OWNER_TOKEN_SIZE, left_percent);
    aurabot_wire_put_int(payload + AURABOT_OWNER_TOKEN_SIZE + 4, right_percent);
    return transact(AURABOT_CMD_DRIVE, payload, sizeof(payload), NULL, 0, NULL);
}

int aurabot_stop(const unsigned char token[AURABOT_OWNER_TOKEN_SIZE])
{
    return token_command(AURABOT_CMD_STOP, token);
}

int aurabot_emergency_stop(void)
{
    return transact(AURABOT_CMD_EMERGENCY_STOP, NULL, 0, NULL, 0, NULL);
}

static int gpio_command(unsigned int command, int pin, int value, int duty,
                         unsigned int size)
{
    unsigned char payload[12];
    aurabot_wire_put_int(payload, pin);
    aurabot_wire_put_int(payload + 4, value);
    aurabot_wire_put_int(payload + 8, duty);
    return transact(command, payload, size, NULL, 0, NULL);
}

int aurabot_digital_write(int pin, int value)
{ return gpio_command(AURABOT_CMD_DIGITAL_WRITE, pin, value, 0, 8U); }
int aurabot_pwm_set(int pin, int frequency, int duty)
{ return gpio_command(AURABOT_CMD_PWM_SET, pin, frequency, duty, 12U); }
int aurabot_pwm_stop(int pin)
{ return gpio_command(AURABOT_CMD_PWM_STOP, pin, 0, 0, 4U); }

int aurabot_get_map(unsigned char *cells, unsigned int capacity,
                    unsigned int *required_size)
{
    unsigned char payload[12 + AURABOT_MAP_CELLS];
    unsigned int size;
    unsigned int required;
    int result;

    if (required_size == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    required = AURABOT_MAP_CELLS;
    *required_size = required;
    if (cells == NULL || capacity < required) return AURABOT_ERR_INVALID_ARGUMENT;
    result = transact(AURABOT_CMD_GET_MAP, NULL, 0, payload, sizeof(payload), &size);
    if (result != AURABOT_OK) return result;
    if (size != sizeof(payload) ||
        aurabot_wire_get_uint(payload) != AURABOT_MAP_WIDTH ||
        aurabot_wire_get_uint(payload + 4) != AURABOT_MAP_HEIGHT)
        return AURABOT_ERR_PROTOCOL;
    memcpy(cells, payload + 12, required);
    return AURABOT_OK;
}

static int uint_command(unsigned int command, unsigned int value)
{
    unsigned char payload[4];
    aurabot_wire_put_uint(payload, value);
    return transact(command, payload, sizeof(payload), NULL, 0, NULL);
}

int aurabot_audio_play(unsigned int track_index)
{
    return uint_command(AURABOT_CMD_AUDIO_PLAY, track_index);
}

int aurabot_audio_pause(void)
{
    return transact(AURABOT_CMD_AUDIO_PAUSE, NULL, 0, NULL, 0, NULL);
}

int aurabot_audio_stop(void)
{
    return transact(AURABOT_CMD_AUDIO_STOP, NULL, 0, NULL, 0, NULL);
}

int aurabot_audio_set_volume(int volume_percent)
{
    unsigned char payload[4];
    aurabot_wire_put_int(payload, volume_percent);
    return transact(AURABOT_CMD_AUDIO_SET_VOLUME, payload, sizeof(payload), NULL, 0, NULL);
}

int aurabot_audio_get_track_count(unsigned int *track_count)
{
    unsigned char payload[4];
    unsigned int size;
    int result;
    if (track_count == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    result = transact(AURABOT_CMD_AUDIO_GET_TRACK_COUNT, NULL, 0,
                      payload, sizeof(payload), &size);
    if (result != AURABOT_OK) return result;
    if (size != 4U) return AURABOT_ERR_PROTOCOL;
    *track_count = aurabot_wire_get_uint(payload);
    return AURABOT_OK;
}

int aurabot_audio_get_track_name(unsigned int track_index, char *name,
                                 unsigned int capacity)
{
    unsigned char request[4];
    unsigned char response[AURABOT_TRACK_NAME_SIZE];
    unsigned int size;
    int result;
    if (name == NULL || capacity == 0U) return AURABOT_ERR_INVALID_ARGUMENT;
    aurabot_wire_put_uint(request, track_index);
    result = transact(AURABOT_CMD_AUDIO_GET_TRACK_NAME, request, sizeof(request),
                      response, sizeof(response), &size);
    if (result != AURABOT_OK) return result;
    if (size == 0U || size > capacity || response[size - 1U] != '\0')
        return AURABOT_ERR_INVALID_ARGUMENT;
    memcpy(name, response, size);
    return AURABOT_OK;
}
