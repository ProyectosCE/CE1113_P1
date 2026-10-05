#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aurabot.h>

#include "json_response.h"
#include "robot_web.h"

static int parameter(const char *query, const char *name, char *output, size_t capacity)
{
    size_t name_size = strlen(name);
    while (*query != '\0') {
        const char *end = strchr(query, '&');
        size_t size = end == NULL ? strlen(query) : (size_t)(end - query);
        if (size > name_size && strncmp(query, name, name_size) == 0 && query[name_size] == '=') {
            size -= name_size + 1;
            if (size == 0 || size >= capacity) return -1;
            memcpy(output, query + name_size + 1, size);
            output[size] = '\0';
            return 0;
        }
        if (end == NULL) break;
        query = end + 1;
    }
    return -1;
}

int web_get_integer(const char *query, const char *name, long minimum,
                    long maximum, int *result)
{
    char value[32], *end;
    long parsed;
    if (parameter(query, name, value, sizeof(value)) != 0) return -1;
    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno == ERANGE || end == value || *end != '\0' || parsed < minimum || parsed > maximum)
        return -1;
    *result = (int)parsed;
    return 0;
}

static int read_token(const char *query, unsigned char *token)
{
    char value[AURABOT_OWNER_TOKEN_SIZE * 2 + 1];
    unsigned int index;
    if (parameter(query, "token", value, sizeof(value)) != 0 ||
        strlen(value) != AURABOT_OWNER_TOKEN_SIZE * 2) return -1;
    for (index = 0; index < AURABOT_OWNER_TOKEN_SIZE; ++index) {
        char pair[3] = {value[index * 2], value[index * 2 + 1], '\0'};
        char *end;
        if (strspn(pair, "0123456789abcdefABCDEF") != 2) return -1;
        token[index] = (unsigned char)strtol(pair, &end, 16);
    }
    return 0;
}

void web_robot_result(const char *operation, int result)
{
    const char *error;
    int gpio = strncmp(operation, "pwm-", 4) == 0 || strcmp(operation, "digital-write") == 0;
    aurabot_disconnect();
    if (result == AURABOT_OK) { json_text(operation, "Comando aplicado"); return; }
    switch (result) {
    case AURABOT_ERR_NOT_OWNER: error = "Esta pestana no tiene el control manual"; break;
    case AURABOT_ERR_WRONG_MODE: error = "El robot debe estar en modo manual"; break;
    case AURABOT_ERR_BUSY: error = gpio ? "GPIO reservado para el robot" : "Otra pestana tiene el control manual"; break;
    case AURABOT_ERR_HARDWARE: error = "Error de hardware o dispositivo deshabilitado"; break;
    case AURABOT_ERR_UNAVAILABLE: error = "AuraBot no disponible"; break;
    case AURABOT_ERR_INVALID_ARGUMENT: error = "Parametros no validos"; break;
    default: error = "Error de protocolo o permisos de AuraBot"; break;
    }
    json_header();
    printf("{\"ok\":false,\"code\":%d,\"error\":\"%s\"}\n", result, error);
}

static void status_response(void)
{
    aurabot_status_t status;
    aurabot_capabilities_t capabilities;
    int result = aurabot_get_status(&status);
    if (result == AURABOT_OK) result = aurabot_get_capabilities(&capabilities);
    if (result != AURABOT_OK) { web_robot_result("robot-status", result); return; }
    json_header();
    printf("{\"ok\":true,\"operacion\":\"robot-status\",\"mode\":%d,\"auto_state\":%d,"
        "\"motors\":[%d,%d],\"motor_movement\":[%d,%d],\"motor_direction\":[%d,%d],"
        "\"sensors\":[%d,%d],\"pose\":{\"x_mm\":%d,\"y_mm\":%d,\"heading_mrad\":%d},"
        "\"control_busy\":%s,\"lease_ms\":%d,\"map_revision\":%u,"
        "\"audio\":{\"state\":%d,\"volume\":%d,\"track\":%d},"
        "\"capabilities\":{\"motors\":[%d,%d],\"encoders\":[%d,%d],\"sensors\":[%d,%d],\"audio\":%d}}\n",
        status.mode, status.auto_state, status.left_speed, status.right_speed,
        status.left_motor_movement, status.right_motor_movement,
        status.left_motor_direction, status.right_motor_direction,
        status.left_obstacle, status.right_obstacle, status.x_mm, status.y_mm, status.heading_mrad,
        status.manual_control_busy ? "true" : "false", AURABOT_CONTROL_LEASE_MS, status.map_revision,
        status.audio_state, status.audio_volume_percent, status.audio_track_index,
        capabilities.left_motor, capabilities.right_motor, capabilities.left_encoder,
        capabilities.right_encoder, capabilities.left_sensor, capabilities.right_sensor, capabilities.audio);
    aurabot_disconnect();
}

static void map_response(void)
{
    aurabot_map_snapshot_t map;
    unsigned int index;
    int result = aurabot_get_map_snapshot(&map);
    if (result != AURABOT_OK) { web_robot_result("robot-map", result); return; }
    json_header();
    printf("{\"ok\":true,\"operacion\":\"robot-map\",\"width\":%d,\"height\":%d,\"revision\":%u,\"cells\":[",
        AURABOT_MAP_WIDTH, AURABOT_MAP_HEIGHT, map.revision);
    for (index = 0; index < AURABOT_MAP_CELLS; ++index)
        printf("%s%u", index == 0 ? "" : ",", map.cells[index]);
    printf("]}\n");
    aurabot_disconnect();
}

int web_robot_handle(const char *operation, const char *query, int is_post)
{
    unsigned char token[AURABOT_OWNER_TOKEN_SIZE];
    int result = AURABOT_ERR_INVALID_ARGUMENT;
    int left, right, mode;
    if (strcmp(operation, "robot-status") == 0) { status_response(); return 1; }
    if (strcmp(operation, "robot-map") == 0) { map_response(); return 1; }
    if (strncmp(operation, "robot-", 6) != 0) return 0;
    if (!is_post) {
        json_error("Use POST para cambiar el estado del robot");
        return 1;
    }
    if (strcmp(operation, "robot-mode") == 0) {
        if (web_get_integer(query, "mode", 1, 2, &mode) == 0 && read_token(query, token) == 0)
            result = aurabot_set_mode_controlled((aurabot_mode_t)mode, token);
    } else if (strcmp(operation, "robot-emergency-stop") == 0) {
        result = aurabot_emergency_stop();
    } else if (read_token(query, token) == 0) {
        if (strcmp(operation, "robot-claim") == 0) result = aurabot_claim_control(token);
        else if (strcmp(operation, "robot-heartbeat") == 0) result = aurabot_control_heartbeat(token);
        else if (strcmp(operation, "robot-release") == 0) result = aurabot_release_control(token);
        else if (strcmp(operation, "robot-stop") == 0) result = aurabot_stop(token);
        else if (strcmp(operation, "robot-drive") == 0 &&
            web_get_integer(query, "left", -100, 100, &left) == 0 &&
            web_get_integer(query, "right", -100, 100, &right) == 0)
            result = aurabot_drive(token, left, right);
    }
    web_robot_result(operation, result);
    return 1;
}
