#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "json_response.h"

#define AUDIO_CONTROL_FIFO "/run/aurabot-audio/control"

static int send_audio_command(const char *command)
{
    char message[64];
    int fd = open(AUDIO_CONTROL_FIFO, O_WRONLY | O_NONBLOCK);
    int length = snprintf(message, sizeof(message), "%s\n", command);

    if (fd < 0 || length < 0 || (size_t)length >= sizeof(message)) {
        if (fd >= 0) {
            close(fd);
        }
        return -1;
    }

    /* Una sola escritura menor que PIPE_BUF mantiene cada orden atómica. */
    if (write(fd, message, (size_t)length) != length) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static void print_audio_devices(void)
{
    int first = 1;

    json_header();
    printf("{\"ok\":true,\"operacion\":\"audio-devices\",\"dispositivos\":[");
    for (int card = 0; card < 32; ++card) {
        char path[64];
        char card_id[64];
        FILE *file;

        snprintf(path, sizeof(path), "/proc/asound/card%d/id", card);
        file = fopen(path, "r");
        if (file == NULL) {
            continue;
        }
        if (fgets(card_id, sizeof(card_id), file) == NULL) {
            fclose(file);
            continue;
        }
        fclose(file);
        card_id[strcspn(card_id, "\r\n")] = '\0';
        for (size_t index = 0; card_id[index] != '\0'; ++index) {
            if (!isalnum((unsigned char)card_id[index]) &&
                card_id[index] != '_' && card_id[index] != '-') {
                card_id[index] = '_';
            }
        }

        printf("%s{\"card\":%d,\"id\":\"%s\",\"analog35\":%s}",
            first ? "" : ",", card, card_id,
            strstr(card_id, "Headphones") != NULL ||
            strstr(card_id, "headphones") != NULL ? "true" : "false");
        first = 0;
    }
    printf("]}\n");
}

static int get_selected_card(const char *query, int *card)
{
    const char *value = strstr(query, "card=");
    char *end_pointer;
    long parsed;

    if (value == NULL) {
        return -1;
    }
    value += 5;
    parsed = strtol(value, &end_pointer, 10);
    if ((*end_pointer != '\0' && *end_pointer != '&') || parsed < 0 || parsed > 31) {
        return -1;
    }
    *card = (int)parsed;
    return 0;
}

static int get_volume(const char *query, int *volume)
{
    const char *value = strstr(query, "volume=");
    char *end_pointer;
    long parsed;

    if (value == NULL) {
        return -1;
    }
    value += 7;
    parsed = strtol(value, &end_pointer, 10);
    if ((*end_pointer != '\0' && *end_pointer != '&') ||
        parsed < 0 || parsed > 100) {
        return -1;
    }
    *volume = (int)parsed;
    return 0;
}

int main(void)
{
    char *query;
    char op[32] = {0};
    float a = 0.0f;
    float b = 0.0f;
    float resultado = 0.0f;

    query = getenv("QUERY_STRING");

    if (query == NULL) {
        json_error("No se recibieron parametros");
        return 0;
    }

    sscanf(query, "op=%31[^&]&a=%f&b=%f", op, &a, &b);

    if (strcmp(op, "audio-devices") == 0) {
        print_audio_devices();
        return 0;
    }

    if (strcmp(op, "audio-device") == 0) {
        char command[32];
        int card;

        if (get_selected_card(query, &card) != 0) {
            json_error("Tarjeta ALSA no valida");
            return 0;
        }
        snprintf(command, sizeof(command), "DEVICE %d", card);
        if (send_audio_command(command) == 0) {
            json_text("audio-device", "Dispositivo enviado al servidor de audio");
        } else {
            json_error("Servidor de audio no disponible");
        }
        return 0;
    }

    if (strcmp(op, "audio-volume") == 0) {
        char command[32];
        int volume;

        if (get_volume(query, &volume) != 0) {
            json_error("Volumen no valido; debe estar entre 0 y 100");
            return 0;
        }
        snprintf(command, sizeof(command), "VOLUME %d", volume);
        if (send_audio_command(command) == 0) {
            json_text("audio-volume", "Volumen enviado al servidor de audio");
        } else {
            json_error("Servidor de audio no disponible");
        }
        return 0;
    }

    if (strcmp(op, "audio-play") == 0 ||
        strcmp(op, "audio-pause") == 0 ||
        strcmp(op, "audio-stop") == 0) {
        const char *command = strcmp(op, "audio-play") == 0 ? "PLAY" :
            (strcmp(op, "audio-pause") == 0 ? "PAUSE" : "STOP");

        if (send_audio_command(command) == 0) {
            json_text(op, "Comando enviado al servidor de audio");
        } else {
            json_error("Servidor de audio no disponible");
        }
        return 0;
    }

    if (strcmp(op, "suma") == 0) {
        resultado = a+b;
        json_result("suma", resultado);
    }
    else if (strcmp(op, "resta") == 0) {
        resultado = a-b;
        json_result("resta", resultado);
    }
    else if (strcmp(op, "mult") == 0) {
        resultado = a*b;
        json_result("mult", resultado);
    }
    else if (strcmp(op, "div") == 0) {
        resultado = a/b;
        json_result("div", resultado);
    }
    else if (strcmp(op, "sqrt") == 0) {
        resultado = sqrt(a);
        json_result("sqrt", resultado);
    }
    else if (strcmp(op, "mensaje") == 0) {
        json_text("mensaje", "Hola desde Raspberry Pi 4");
    }
    else {
        json_error("Operacion no valida");
    }

    return 0;
}
