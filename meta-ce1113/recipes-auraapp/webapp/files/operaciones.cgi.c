#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "json_response.h"
#include <aurabot.h>

#define AUDIO_CONTROL_FIFO "/run/aurabot-audio/control"
#define PLAYLIST_PATH "/media/audio/playlist.txt"

static int send_command(const char *fifo, const char *command)
{
    char message[64];
    int fd = open(fifo, O_WRONLY | O_NONBLOCK);
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

static int get_integer(const char *query, const char *name, long minimum,
                       long maximum, int *result)
{
    char key[32];
    const char *value;
    char *end_pointer;
    long parsed;

    snprintf(key, sizeof(key), "%s=", name);
    value = strstr(query, key);
    if (value == NULL) return -1;
    value += strlen(key);
    parsed = strtol(value, &end_pointer, 10);
    if ((*end_pointer != '\0' && *end_pointer != '&') ||
        parsed < minimum || parsed > maximum) return -1;
    *result = (int)parsed;
    return 0;
}

static void print_json_string(const char *text)
{
    putchar('"');
    for (; *text != '\0'; ++text) {
        unsigned char character = (unsigned char)*text;
        if (character == '"' || character == '\\') printf("\\%c", character);
        else if (character >= 32) putchar(character);
        else printf("\\u%04x", character);
    }
    putchar('"');
}

static void print_playlist(void)
{
    FILE *playlist = fopen(PLAYLIST_PATH, "r");
    char path[512];
    int first = 1;

    json_header();
    printf("{\"ok\":true,\"operacion\":\"audio-playlist\",\"canciones\":[");
    if (playlist != NULL) {
        while (fgets(path, sizeof(path), playlist) != NULL) {
            const char *name;
            path[strcspn(path, "\r\n")] = '\0';
            if (path[0] == '\0') continue;
            name = strrchr(path, '/');
            name = name == NULL ? path : name + 1;
            printf("%s", first ? "" : ",");
            print_json_string(name);
            first = 0;
        }
        fclose(playlist);
    }
    printf("]}\n");
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
    return get_integer(query, "card", 0, 31, card);
}

static int get_volume(const char *query, int *volume)
{
    return get_integer(query, "volume", 0, 100, volume);
}

static void robot_result(const char *operation, int result)
{
    aurabot_disconnect();
    if (result == AURABOT_OK) json_text(operation, "Comando aplicado");
    else if (result == AURABOT_ERR_WRONG_MODE)
        json_error("Cambie a modo manual para probar GPIO");
    else if (result == AURABOT_ERR_BUSY)
        json_error("GPIO reservado para motores, sensores, encoders o LEDs de estado");
    else if (result == AURABOT_ERR_UNAVAILABLE)
        json_error("AuraBot no disponible en /run/aurabot/control.sock");
    else if (result == AURABOT_ERR_INVALID_ARGUMENT)
        json_error("Parametros GPIO no validos");
    else if (result == AURABOT_ERR_HARDWARE)
        json_error("Error al escribir el GPIO; revise el registro de AuraBot");
    else json_error("Error de protocolo o permisos al conectar con AuraBot");
}

static void print_robot_status(void)
{
    aurabot_status_t status;
    int result = aurabot_get_status(&status);
    aurabot_disconnect();
    if (result != AURABOT_OK) {
        robot_result("robot-status", result);
        return;
    }
    json_header();
    printf("{\"ok\":true,\"operacion\":\"robot-status\",\"mode\":%d,"
        "\"auto_state\":%d,\"motors\":[%d,%d],\"sensors\":[%d,%d]}\n",
        status.mode, status.auto_state, status.left_speed, status.right_speed,
        status.left_obstacle, status.right_obstacle);
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

    if (strcmp(op, "robot-status") == 0) {
        print_robot_status();
        return 0;
    }
    if (strcmp(op, "robot-mode") == 0) {
        int mode;
        if (get_integer(query, "mode", 1, 2, &mode) != 0)
            json_error("Modo no valido");
        else robot_result(op, aurabot_set_mode((aurabot_mode_t)mode));
        return 0;
    }
    if (strcmp(op, "robot-emergency-stop") == 0) {
        robot_result(op, aurabot_emergency_stop());
        return 0;
    }

    if (strcmp(op, "audio-devices") == 0) {
        print_audio_devices();
        return 0;
    }

    if (strcmp(op, "audio-playlist") == 0) {
        print_playlist();
        return 0;
    }

    if (strcmp(op, "audio-track") == 0) {
        char command[32];
        int track;
        if (get_integer(query, "track", 0, 9999, &track) != 0) {
            json_error("Cancion no valida");
            return 0;
        }
        snprintf(command, sizeof(command), "TRACK %d", track);
        if (send_command(AUDIO_CONTROL_FIFO, command) == 0)
            json_text("audio-track", "Cancion enviada al servidor de audio");
        else json_error("Servidor de audio no disponible");
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
        if (send_command(AUDIO_CONTROL_FIFO, command) == 0) {
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
        /* La escala web 0..100 corresponde al rango útil ALSA 50..100. */
        snprintf(command, sizeof(command), "VOLUME %d", 50 + (volume + 1) / 2);
        if (send_command(AUDIO_CONTROL_FIFO, command) == 0) {
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

        if (send_command(AUDIO_CONTROL_FIFO, command) == 0) {
            json_text(op, "Comando enviado al servidor de audio");
        } else {
            json_error("Servidor de audio no disponible");
        }
        return 0;
    }

    if (strcmp(op, "pwm-set") == 0 || strcmp(op, "pwm-stop") == 0) {
        int pin;
        if (get_integer(query, "pin", 0, 27, &pin) != 0) {
            json_error("PIN PWM no valido; use GPIO 0 a 27");
            return 0;
        }
        if (strcmp(op, "pwm-set") == 0) {
            int frequency, duty;
            if (get_integer(query, "frequency", 1, 10000, &frequency) != 0 ||
                get_integer(query, "duty", 0, 100, &duty) != 0) {
                json_error("Frecuencia o ciclo de trabajo no valido");
                return 0;
            }
            robot_result(op, aurabot_pwm_set(pin, frequency, duty));
        } else robot_result(op, aurabot_pwm_stop(pin));
        return 0;
    }

    if (strcmp(op, "digital-write") == 0) {
        int pin, value;
        if (get_integer(query, "pin", 0, 27, &pin) != 0 ||
            get_integer(query, "value", 0, 1, &value) != 0) {
            json_error("PIN digital o estado no valido");
            return 0;
        }
        robot_result(op, aurabot_digital_write(pin, value));
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
