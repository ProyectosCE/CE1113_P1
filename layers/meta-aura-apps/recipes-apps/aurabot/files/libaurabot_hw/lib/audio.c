#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libaudio.h>

#include "../include/aurabot_audio.h"
#include "aurabot_hw_internal.h"


#define DEFAULT_AUDIO_DEVICE "default"

static int audio_started = 0;
static char configured_audio_device[128];


static aurabot_hw_status_t audio_start_backend(void)
{
    if (audio_started) {
        return AURABOT_HW_OK;
    }

    /*
     * No se fija un índice plughw:N,0: los índices ALSA cambian cuando se
     * habilitan HDMI o dispositivos USB. El servicio que invoque la biblioteca
     * puede seleccionar un PCM estable con AURABOT_AUDIO_DEVICE.
     */
    const char *audio_device = configured_audio_device;

    if (audio_device[0] == '\0') {
        const char *environment_device = getenv("AURABOT_AUDIO_DEVICE");
        audio_device = environment_device != NULL && environment_device[0] != '\0'
            ? environment_device : DEFAULT_AUDIO_DEVICE;
    }

    if (IniciarSonido(audio_device) != 0) {
        return AURABOT_HW_ERROR;
    }

    audio_started = 1;

    return AURABOT_HW_OK;
}


aurabot_hw_status_t aurabot_audio_set_device(const char *audio_device)
{
    if (!aurabot_hw_is_initialized()) {
        return AURABOT_HW_NOT_INITIALIZED;
    }
    if (audio_device == NULL || audio_device[0] == '\0' ||
        strlen(audio_device) >= sizeof(configured_audio_device)) {
        return AURABOT_HW_INVALID_ARGUMENT;
    }

    if (audio_started && FinalizarSonido() != 0) {
        return AURABOT_HW_ERROR;
    }
    audio_started = 0;
    snprintf(configured_audio_device, sizeof(configured_audio_device), "%s",
        audio_device);
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

    const char *audio_device = configured_audio_device;

    if (audio_device[0] == '\0') {
        const char *environment_device = getenv("AURABOT_AUDIO_DEVICE");
        audio_device = environment_device != NULL && environment_device[0] != '\0'
            ? environment_device : DEFAULT_AUDIO_DEVICE;
    }

    if (strcmp(audio_device, DEFAULT_AUDIO_DEVICE) == 0) {
        return AURABOT_HW_NOT_AVAILABLE;
    }

    return AjustarVolumen(audio_device, volume_percent) == 0
        ? AURABOT_HW_OK : AURABOT_HW_ERROR;
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
