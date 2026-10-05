#include <stddef.h>

#include "audio_controller.h"
#include "hardware.h"

int audio_controller_play(aurabot_state_t *state, unsigned int track_index)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (track_index >= hardware_audio_track_count())
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_audio_play(track_index) != 0) {
        state->public_status.audio_state = AURABOT_AUDIO_ERROR;
        return AURABOT_ERR_HARDWARE;
    }
    state->public_status.audio_state = AURABOT_AUDIO_PLAYING;
    state->public_status.audio_track_index = (int)track_index;
    return AURABOT_OK;
}

int audio_controller_pause(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_audio_pause() != 0) return AURABOT_ERR_HARDWARE;
    state->public_status.audio_state = AURABOT_AUDIO_PAUSED;
    return AURABOT_OK;
}

int audio_controller_stop(aurabot_state_t *state)
{
    if (state == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_audio_stop() != 0) return AURABOT_ERR_HARDWARE;
    state->public_status.audio_state = AURABOT_AUDIO_STOPPED;
    state->public_status.audio_track_index = -1;
    return AURABOT_OK;
}

int audio_controller_set_volume(aurabot_state_t *state, int volume_percent)
{
    if (state == NULL || volume_percent < 0 || volume_percent > 100)
        return AURABOT_ERR_INVALID_ARGUMENT;
    if (hardware_audio_set_volume(volume_percent) != 0)
        return AURABOT_ERR_HARDWARE;
    state->public_status.audio_volume_percent = volume_percent;
    return AURABOT_OK;
}

unsigned int audio_controller_track_count(void)
{
    return hardware_audio_track_count();
}

int audio_controller_track_name(unsigned int track_index, char *name,
                                unsigned int capacity)
{
    return hardware_audio_track_name(track_index, name, capacity) == 0
        ? AURABOT_OK : AURABOT_ERR_INVALID_ARGUMENT;
}

void audio_controller_event(const char *event_name)
{
    (void)hardware_audio_event(event_name);
}
