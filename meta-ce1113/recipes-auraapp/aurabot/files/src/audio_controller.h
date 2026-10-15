#ifndef AURABOT_AUDIO_CONTROLLER_H
#define AURABOT_AUDIO_CONTROLLER_H

#include "aurabot_state.h"

int audio_controller_play(aurabot_state_t *state, unsigned int track_index);
int audio_controller_pause(aurabot_state_t *state);
int audio_controller_stop(aurabot_state_t *state);
int audio_controller_set_volume(aurabot_state_t *state, int volume_percent);
unsigned int audio_controller_track_count(void);
int audio_controller_track_name(unsigned int track_index, char *name,
                                unsigned int capacity);
void audio_controller_event(const char *event_name);

#endif
