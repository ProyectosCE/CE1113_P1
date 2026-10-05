#ifndef AURABOT_H
#define AURABOT_H

#define AURABOT_OWNER_TOKEN_SIZE 32
#define AURABOT_MAP_WIDTH 40
#define AURABOT_MAP_HEIGHT 40
#define AURABOT_MAP_CELLS (AURABOT_MAP_WIDTH * AURABOT_MAP_HEIGHT)
#define AURABOT_TRACK_NAME_SIZE 128

typedef enum {
    AURABOT_MODE_INIT = 0,
    AURABOT_MODE_AUTONOMOUS,
    AURABOT_MODE_MANUAL,
    AURABOT_MODE_SAFE_STOP
} aurabot_mode_t;

typedef enum {
    AURABOT_AUTO_INACTIVE = 0,
    AURABOT_AUTO_FORWARD,
    AURABOT_AUTO_REVERSE,
    AURABOT_AUTO_TURN
} aurabot_auto_state_t;

typedef enum {
    AURABOT_CELL_UNKNOWN = 0,
    AURABOT_CELL_VISITED,
    AURABOT_CELL_OBSTACLE
} aurabot_cell_t;

typedef enum {
    AURABOT_AUDIO_STOPPED = 0,
    AURABOT_AUDIO_PLAYING,
    AURABOT_AUDIO_PAUSED,
    AURABOT_AUDIO_ERROR
} aurabot_audio_state_t;

typedef enum {
    AURABOT_OK = 0,
    AURABOT_ERR_INVALID_ARGUMENT = -1,
    AURABOT_ERR_NOT_OWNER = -2,
    AURABOT_ERR_WRONG_MODE = -3,
    AURABOT_ERR_BUSY = -4,
    AURABOT_ERR_HARDWARE = -5,
    AURABOT_ERR_PROTOCOL = -6,
    AURABOT_ERR_UNAVAILABLE = -7
} aurabot_result_t;

typedef struct {
    aurabot_mode_t mode;
    aurabot_auto_state_t auto_state;
    int left_speed;
    int right_speed;
    int left_obstacle;
    int right_obstacle;
    int x_mm;
    int y_mm;
    int heading_mrad;
    int manual_control_busy;
    unsigned int map_revision;
    aurabot_audio_state_t audio_state;
    int audio_volume_percent;
    int audio_track_index;
} aurabot_status_t;

int aurabot_connect(void);
void aurabot_disconnect(void);

int aurabot_get_status(aurabot_status_t *status);
int aurabot_set_mode(aurabot_mode_t mode);

int aurabot_claim_control(const unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE]);
int aurabot_control_heartbeat(const unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE]);
int aurabot_release_control(const unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE]);
int aurabot_drive(const unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE],
                  int left_percent, int right_percent);
int aurabot_stop(const unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE]);
int aurabot_emergency_stop(void);
/* Pruebas GPIO: solo pines libres, con el robot en modo manual. */
int aurabot_digital_write(int pin, int value);
int aurabot_pwm_set(int pin, int frequency, int duty);
int aurabot_pwm_stop(int pin);

int aurabot_get_map(unsigned char *cells, unsigned int capacity,
                    unsigned int *required_size);

int aurabot_audio_play(unsigned int track_index);
int aurabot_audio_pause(void);
int aurabot_audio_stop(void);
int aurabot_audio_set_volume(int volume_percent);
int aurabot_audio_get_track_count(unsigned int *track_count);
int aurabot_audio_get_track_name(unsigned int track_index, char *name,
                                 unsigned int capacity);

#endif
