#ifndef AURABOT_AUDIO_H
#define AURABOT_AUDIO_H

#include "aurabot_hw_types.h"


typedef enum {
    AURABOT_AUDIO_STOPPED = 0,
    AURABOT_AUDIO_PLAYING,
    AURABOT_AUDIO_PAUSED
} aurabot_audio_state_t;


aurabot_hw_status_t aurabot_audio_play_file(
    const char *file_path
);

aurabot_hw_status_t aurabot_audio_pause(void);

aurabot_hw_status_t aurabot_audio_stop(void);

aurabot_hw_status_t aurabot_audio_set_volume(
    int volume_percent
);

aurabot_hw_status_t aurabot_audio_get_state(
    aurabot_audio_state_t *state
);

#endif