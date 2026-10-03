#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libaudio.h>

#define CONTROL_DIRECTORY "/run/aurabot-audio"
#define CONTROL_FIFO CONTROL_DIRECTORY "/control"
#define DEFAULT_AUDIO "/media/audio/test.mp3"

static char audio_device[96] = "default";

static volatile sig_atomic_t running = 1;

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static int execute_command(char *command)
{
    command[strcspn(command, "\r\n")] = '\0';

    if (strcmp(command, "PLAY") == 0) {
        return CargarSonido(DEFAULT_AUDIO);
    }
    if (strcmp(command, "PAUSE") == 0) {
        return Pausar();
    }
    if (strcmp(command, "STOP") == 0) {
        return Stop();
    }
    if (strncmp(command, "VOLUME ", 7) == 0) {
        char *end_pointer;
        long volume = strtol(command + 7, &end_pointer, 10);

        if (*end_pointer != '\0' || volume < 0 || volume > 100) {
            return -1;
        }
        return AjustarVolumen(audio_device, (int)volume);
    }
    if (strncmp(command, "DEVICE ", 7) == 0) {
        char card_path[64];
        char card_id[64];
        char selected_device[96];
        char *end_pointer;
        long card = strtol(command + 7, &end_pointer, 10);
        FILE *card_file;

        if (*end_pointer != '\0' || card < 0 || card > 31) {
            return -1;
        }
        snprintf(card_path, sizeof(card_path), "/proc/asound/card%ld/id", card);
        card_file = fopen(card_path, "r");
        if (card_file == NULL || fgets(card_id, sizeof(card_id), card_file) == NULL) {
            if (card_file != NULL) {
                fclose(card_file);
            }
            return -1;
        }
        fclose(card_file);
        card_id[strcspn(card_id, "\r\n")] = '\0';
        for (size_t index = 0; card_id[index] != '\0'; ++index) {
            if (!isalnum((unsigned char)card_id[index]) &&
                card_id[index] != '_' && card_id[index] != '-') {
                return -1;
            }
        }
        snprintf(selected_device, sizeof(selected_device), "plughw:%s,0", card_id);
        if (FinalizarSonido() != 0 || IniciarSonido(selected_device) != 0) {
            return -1;
        }
        snprintf(audio_device, sizeof(audio_device), "%s", selected_device);
        fprintf(stderr, "Seleccionando dispositivo ALSA %s\n", audio_device);
        return 0;
    }

    fprintf(stderr, "Comando de audio desconocido: %s\n", command);
    return -1;
}

int main(void)
{
    struct pollfd poll_fd;
    char command[64];

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);

    if (mkdir(CONTROL_DIRECTORY, 0755) != 0 && errno != EEXIST) {
        perror("mkdir control audio");
        return 1;
    }

    unlink(CONTROL_FIFO);
    if (mkfifo(CONTROL_FIFO, 0666) != 0) {
        perror("mkfifo control audio");
        return 1;
    }
    if (chmod(CONTROL_FIFO, 0666) != 0) {
        perror("chmod control audio");
        unlink(CONTROL_FIFO);
        return 1;
    }

    poll_fd.fd = open(CONTROL_FIFO, O_RDWR | O_NONBLOCK);
    poll_fd.events = POLLIN;
    if (poll_fd.fd < 0) {
        perror("open control audio");
        unlink(CONTROL_FIFO);
        return 1;
    }

    const char *configured_device = getenv("AURABOT_AUDIO_DEVICE");
    if (configured_device != NULL && configured_device[0] != '\0') {
        snprintf(audio_device, sizeof(audio_device), "%s", configured_device);
    }
    if (IniciarSonido(audio_device) != 0) {
        fprintf(stderr, "No se pudo inicializar el audio AuraBot\n");
        close(poll_fd.fd);
        unlink(CONTROL_FIFO);
        return 1;
    }

    fprintf(stderr, "Servidor de audio listo en %s\n", CONTROL_FIFO);
    while (running) {
        int ready = poll(&poll_fd, 1, 1000);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll control audio");
            break;
        }
        if (ready == 0 || !(poll_fd.revents & POLLIN)) {
            continue;
        }

        ssize_t bytes = read(poll_fd.fd, command, sizeof(command) - 1);
        if (bytes > 0) {
            char *save_pointer = NULL;
            char *line;

            command[bytes] = '\0';
            line = strtok_r(command, "\r\n", &save_pointer);
            while (line != NULL) {
                execute_command(line);
                line = strtok_r(NULL, "\r\n", &save_pointer);
            }
        }
    }

    Stop();
    FinalizarSonido();
    close(poll_fd.fd);
    unlink(CONTROL_FIFO);
    return 0;
}
#include <ctype.h>
