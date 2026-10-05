#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aurabot.h>

static unsigned char development_token[AURABOT_OWNER_TOKEN_SIZE] = {
    'a','u','r','a','b','o','t','c','t','l'
};

static int print_error(int result)
{
    fprintf(stderr, "aurabotctl: error %d\n", result);
    return EXIT_FAILURE;
}

static const char *movement_state(int enabled, int moving)
{
    if (!enabled) return "DESHABILITADO";
    return moving ? "AVANZANDO" : "QUIETO";
}

static int show_status(void)
{
    aurabot_status_t status;
    aurabot_capabilities_t capabilities;
    int result = aurabot_get_status(&status);
    if (result == AURABOT_OK)
        result = aurabot_get_capabilities(&capabilities);
    if (result != AURABOT_OK) return print_error(result);
    printf("mode=%d auto=%d motors=%d,%d motor_movement=%d,%d "
           "movement_state=%s,%s motor_direction=%d,%d sensors=%d,%d "
           "pose=%d,%d,%d owner=%d map=%u audio=%d volume=%d track=%d\n",
           status.mode, status.auto_state, status.left_speed,
           status.right_speed,
           status.left_motor_movement, status.right_motor_movement,
           movement_state(capabilities.left_encoder,
                          status.left_motor_movement),
           movement_state(capabilities.right_encoder,
                          status.right_motor_movement),
           status.left_motor_direction, status.right_motor_direction,
           status.left_obstacle, status.right_obstacle,
           status.x_mm, status.y_mm, status.heading_mrad,
           status.manual_control_busy, status.map_revision,
           status.audio_state, status.audio_volume_percent,
           status.audio_track_index);
    return EXIT_SUCCESS;
}

static int show_map(void)
{
    unsigned char cells[AURABOT_MAP_CELLS];
    unsigned int required;
    int result = aurabot_get_map(cells, sizeof(cells), &required);
    unsigned int row, column;
    if (result != AURABOT_OK) return print_error(result);
    for (row = 0; row < AURABOT_MAP_HEIGHT; ++row) {
        for (column = 0; column < AURABOT_MAP_WIDTH; ++column) {
            unsigned char cell = cells[row * AURABOT_MAP_WIDTH + column];
            putchar(cell == AURABOT_CELL_OBSTACLE ? '#' :
                    cell == AURABOT_CELL_VISITED ? '.' : '?');
        }
        putchar('\n');
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    int result = AURABOT_ERR_INVALID_ARGUMENT;
    if (argc < 2) {
        fprintf(stderr, "uso: aurabotctl status|mode|claim|heartbeat|drive|stop|"
                        "release|emergency-stop|map|play|pause|audio-stop|volume\n");
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "status") == 0) return show_status();
    if (strcmp(argv[1], "map") == 0) return show_map();
    if (strcmp(argv[1], "mode") == 0 && argc == 3) {
        if (strcmp(argv[2], "manual") == 0)
            result = aurabot_set_mode(AURABOT_MODE_MANUAL);
        else if (strcmp(argv[2], "autonomous") == 0)
            result = aurabot_set_mode(AURABOT_MODE_AUTONOMOUS);
    } else if (strcmp(argv[1], "claim") == 0) {
        result = aurabot_claim_control(development_token);
    } else if (strcmp(argv[1], "heartbeat") == 0) {
        result = aurabot_control_heartbeat(development_token);
    } else if (strcmp(argv[1], "drive") == 0 && argc == 4) {
        result = aurabot_drive(development_token, atoi(argv[2]), atoi(argv[3]));
    } else if (strcmp(argv[1], "stop") == 0) {
        result = aurabot_stop(development_token);
    } else if (strcmp(argv[1], "release") == 0) {
        result = aurabot_release_control(development_token);
    } else if (strcmp(argv[1], "emergency-stop") == 0) {
        result = aurabot_emergency_stop();
    } else if (strcmp(argv[1], "play") == 0 && argc == 3) {
        result = aurabot_audio_play((unsigned int)atoi(argv[2]));
    } else if (strcmp(argv[1], "pause") == 0) {
        result = aurabot_audio_pause();
    } else if (strcmp(argv[1], "audio-stop") == 0) {
        result = aurabot_audio_stop();
    } else if (strcmp(argv[1], "volume") == 0 && argc == 3) {
        result = aurabot_audio_set_volume(atoi(argv[2]));
    }
    aurabot_disconnect();
    return result == AURABOT_OK ? EXIT_SUCCESS : print_error(result);
}
