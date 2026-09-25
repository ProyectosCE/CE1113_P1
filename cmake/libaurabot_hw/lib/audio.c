#include <stddef.h>
#include <libaudio.h>

#include "../include/aurabot_audio.h"
#include "aurabot_hw_internal.h"


/*
 * Current ALSA device used by the existing audio implementation
 * Update this value if the final Raspberry Pi audio device changes
 */
#define AUDIO_DEVICE "plughw:2,0"

static int audio_started = 0;


static aurabot_hw_status_t audio_start_backend(void)
{
    if (audio_started) {
        return AURABOT_HW_OK;
    }

    if (IniciarSonido(AUDIO_DEVICE) != 0) {
        return AURABOT_HW_ERROR;
    }

    audio_started = 1;

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_audio_play_file(
    const char *file_path
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (file_path == NULL || file_path[0] == '\0') {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    aurabot_hw_status_t status = audio_start_backend();

    if (status != AURABOT_HW_OK) {
        return status;
    }

    if (CargarSonido(file_path) != 0) {
        return AURABOT_HW_ERROR;
    }

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_audio_pause(void)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!audio_started) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    if (Pausar() != 0) {
        return AURABOT_HW_ERROR;
    }

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_audio_stop(void)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (!audio_started) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    if (Stop() != 0) {
        return AURABOT_HW_ERROR;
    }

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_audio_set_volume(
    int volume_percent
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (volume_percent < 0 || volume_percent > 100) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    return AURABOT_HW_NOT_IMPLEMENTED;
}


aurabot_hw_status_t aurabot_audio_get_state(
    aurabot_audio_state_t *state
)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }

    if (state == NULL) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    return AURABOT_HW_NOT_IMPLEMENTED;
}