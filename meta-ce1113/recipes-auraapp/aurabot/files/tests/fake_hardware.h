#ifndef AURABOT_FAKE_HARDWARE_H
#define AURABOT_FAKE_HARDWARE_H

void fake_hardware_set_obstacles(int left, int right);
void fake_hardware_get_motor_speeds(int *left, int *right);
void fake_hardware_get_leds(int *power, int *manual, int *autonomous, int *obstacle);
unsigned int fake_hardware_audio_event_count(void);
const char *fake_hardware_audio_event(unsigned int index);

#endif
