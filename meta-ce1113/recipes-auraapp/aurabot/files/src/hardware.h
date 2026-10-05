#ifndef AURABOT_HARDWARE_H
#define AURABOT_HARDWARE_H

typedef struct {
    int left_obstacle;
    int right_obstacle;
} sensor_snapshot_t;

typedef struct {
    unsigned int left_ticks;
    unsigned int right_ticks;
    int left_direction;
    int right_direction;
    int left_angular_velocity_mrad_s;
    int right_angular_velocity_mrad_s;
    int sample_time_ms;
} encoder_snapshot_t;

int hardware_init(void);
void hardware_shutdown(void);
int hardware_drive(int left_percent, int right_percent);
int hardware_stop(void);
int hardware_read_sensors(sensor_snapshot_t *result);
int hardware_take_encoder_sample(encoder_snapshot_t *result);
int hardware_set_leds(int power, int manual, int autonomous, int obstacle);
int hardware_audio_event(const char *event_name);
int hardware_audio_play(unsigned int track_index);
int hardware_audio_pause(void);
int hardware_audio_stop(void);
int hardware_audio_set_volume(int volume_percent);
unsigned int hardware_audio_track_count(void);
int hardware_audio_track_name(unsigned int track_index, char *name,
                              unsigned int capacity);
unsigned int hardware_left_ticks_per_revolution(void);
unsigned int hardware_right_ticks_per_revolution(void);
unsigned int hardware_left_wheel_circumference_mm(void);
unsigned int hardware_right_wheel_circumference_mm(void);
unsigned int hardware_wheel_base_mm(void);

#endif
