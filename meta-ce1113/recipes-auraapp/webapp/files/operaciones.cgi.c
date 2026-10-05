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
#include "robot_web.h"

static int get_integer(const char *query, const char *name, long minimum,
                       long maximum, int *result)
{
    return web_get_integer(query, name, minimum, maximum, result);
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
    unsigned int count, index;
    char name[AURABOT_TRACK_NAME_SIZE];
    int result = aurabot_audio_get_track_count(&count);
    if (result != AURABOT_OK) { web_robot_result("audio-playlist", result); return; }

    json_header();
    printf("{\"ok\":true,\"operacion\":\"audio-playlist\",\"canciones\":[");
    for (index = 0; index < count; ++index) {
        result = aurabot_audio_get_track_name(index, name, sizeof(name));
        printf("%s", index == 0 ? "" : ",");
        print_json_string(result == AURABOT_OK ? name : "Pista no disponible");
    }
    printf("]}\n");
    aurabot_disconnect();
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
    web_robot_result(operation, result);
}

int main(void)
{
    char *query;
    char post_body[1025];
    const char *method = getenv("REQUEST_METHOD");
    int is_post = method != NULL && strcmp(method, "POST") == 0;
    char op[32] = {0};
    float a = 0.0f;
    float b = 0.0f;
    float resultado = 0.0f;

    query = getenv("QUERY_STRING");
    if (is_post) {
        const char *length_text = getenv("CONTENT_LENGTH");
        char *end;
        long length;
        if (length_text == NULL) { json_error("Falta CONTENT_LENGTH"); return 0; }
        errno = 0;
        length = strtol(length_text, &end, 10);
        if (errno != 0 || end == length_text || *end != '\0' || length < 1 || length > 1024) {
            json_error("Cuerpo POST no valido"); return 0;
        }
        if (fread(post_body, 1, (size_t)length, stdin) != (size_t)length) {
            json_error("Cuerpo POST incompleto"); return 0;
        }
        post_body[length] = '\0';
        query = post_body;
    }

    if (query == NULL) {
        json_error("No se recibieron parametros");
        return 0;
    }

    sscanf(query, "op=%31[^&]&a=%f&b=%f", op, &a, &b);

    if (web_robot_handle(op, query, is_post)) return 0;

    if (strcmp(op, "audio-devices") == 0) {
        print_audio_devices();
        return 0;
    }

    if (strcmp(op, "audio-playlist") == 0) {
        print_playlist();
        return 0;
    }

    if (strcmp(op, "audio-track") == 0) {
        int track;
        if (get_integer(query, "track", 0, 9999, &track) != 0) {
            json_error("Cancion no valida");
            return 0;
        }
        robot_result(op, aurabot_audio_play((unsigned int)track));
        return 0;
    }

    if (strcmp(op, "audio-device") == 0) {
        int card;

        if (get_selected_card(query, &card) != 0) {
            json_error("Tarjeta ALSA no valida");
            return 0;
        }
        robot_result(op, aurabot_audio_set_device(card));
        return 0;
    }

    if (strcmp(op, "audio-volume") == 0) {
        int volume;

        if (get_volume(query, &volume) != 0) {
            json_error("Volumen no valido; debe estar entre 0 y 100");
            return 0;
        }
        /* La escala web 0..100 corresponde al rango útil ALSA 50..100. */
        robot_result(op, aurabot_audio_set_volume(50 + (volume + 1) / 2));
        return 0;
    }

    if (strcmp(op, "audio-play") == 0 ||
        strcmp(op, "audio-pause") == 0 ||
        strcmp(op, "audio-stop") == 0) {
        if (strcmp(op, "audio-play") == 0) {
            aurabot_status_t status;
            int result = aurabot_get_status(&status);
            if (result == AURABOT_OK)
                result = aurabot_audio_play(status.audio_track_index < 0 ? 0U :
                    (unsigned int)status.audio_track_index);
            robot_result(op, result);
        } else robot_result(op, strcmp(op, "audio-pause") == 0 ?
            aurabot_audio_pause() : aurabot_audio_stop());
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
