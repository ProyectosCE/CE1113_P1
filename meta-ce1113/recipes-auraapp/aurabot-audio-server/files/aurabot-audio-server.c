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

#ifndef CONTROL_DIRECTORY
#define CONTROL_DIRECTORY "/run/aurabot-audio"
#endif
#ifndef CONTROL_FIFO
#define CONTROL_FIFO CONTROL_DIRECTORY "/control"
#endif
#ifndef DEFAULT_AUDIO
#define DEFAULT_AUDIO "/media/audio/test.mp3"
#endif
#ifndef PLAYLIST_PATH
#define PLAYLIST_PATH "/media/audio/playlist.txt"
#endif
#ifndef SOUND_DIRECTORY
#define SOUND_DIRECTORY "/usr/share/aurabot/sounds"
#endif
#define EVENT_QUEUE_CAPACITY 16U

typedef enum {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING,
    MUSIC_PAUSED
} music_state_t;

static char audio_device[96] = "default";
static char music_path[512];
static music_state_t music_state = MUSIC_STOPPED;
static long music_resume_frame;
static unsigned long music_generation;

static const char *event_queue[EVENT_QUEUE_CAPACITY];
static unsigned int event_queue_head;
static unsigned int event_queue_count;
static int event_active;
static int obstacle_event_pending;
static const char *active_event_path;
static unsigned long active_event_generation;

static volatile sig_atomic_t running = 1;

static void select_default_audio_device(void)
{
    for (int card = 0; card < 32; ++card) {
        char card_path[64];
        char card_id[64];
        FILE *card_file;

        snprintf(card_path, sizeof(card_path), "/proc/asound/card%d/id", card);
        card_file = fopen(card_path, "r");
        if (card_file == NULL) continue;
        if (fgets(card_id, sizeof(card_id), card_file) == NULL) {
            fclose(card_file);
            continue;
        }
        fclose(card_file);
        card_id[strcspn(card_id, "\r\n")] = '\0';
        if (strstr(card_id, "Headphones") != NULL ||
            strstr(card_id, "headphones") != NULL) {
            snprintf(audio_device, sizeof(audio_device), "plughw:%s,0", card_id);
            return;
        }
    }
}

static int resolve_playlist_track(long requested_index, char *path,
                                  size_t capacity)
{
    FILE *playlist;
    long index = 0;

    if (requested_index < 0 || requested_index > 9999 ||
        path == NULL || capacity == 0U) {
        return -1;
    }
    playlist = fopen(PLAYLIST_PATH, "r");
    if (playlist == NULL) {
        return -1;
    }
    while (fgets(path, (int)capacity, playlist) != NULL) {
        path[strcspn(path, "\r\n")] = '\0';
        if (path[0] == '\0') continue;
        if (index++ != requested_index) continue;
        fclose(playlist);
        if (strncmp(path, "/media/audio/", 13) != 0 ||
            strstr(path, "..") != NULL || access(path, R_OK) != 0) {
            return -1;
        }
        return 0;
    }
    fclose(playlist);
    return -1;
}

static int request_music(const char *path)
{
    int result;

    if (path == NULL || strlen(path) >= sizeof(music_path)) return -1;
    snprintf(music_path, sizeof(music_path), "%s", path);
    music_state = MUSIC_PLAYING;
    music_resume_frame = 0;
    if (event_active) return 0;
    result = CargarSonido(music_path);
    if (result == 0) music_generation = GeneracionSolicitadaAudio();
    return result;
}

static const char *event_sound_path(const char *name)
{
    if (strcmp(name, "boot") == 0)
        return SOUND_DIRECTORY "/startup.mp3";
    if (strcmp(name, "manual") == 0)
        return SOUND_DIRECTORY "/modoManual.mp3";
    if (strcmp(name, "autonomous") == 0)
        return SOUND_DIRECTORY "/modoAuto.mp3";
    if (strcmp(name, "obstacle") == 0)
        return SOUND_DIRECTORY "/obstaculo.mp3";
    return NULL;
}

static int start_event(const char *path, int capture_music_position)
{
    if (ProcesarRespuestasAudio() != 0) return -1;
    if (capture_music_position && music_state != MUSIC_STOPPED) {
        music_resume_frame = FrameActualAudio();
        if (music_resume_frame < 0) music_resume_frame = 0;
    }
    active_event_generation = 0U;
    if (CargarSonido(path) != 0) return -1;
    active_event_generation = GeneracionSolicitadaAudio();
    event_active = 1;
    active_event_path = path;
    return 0;
}

static int enqueue_event(const char *name)
{
    const char *path = event_sound_path(name);
    unsigned int tail;
    int obstacle_event = strcmp(name, "obstacle") == 0;

    if (path == NULL || access(path, R_OK) != 0) return -1;
    if (obstacle_event && obstacle_event_pending) return 0;
    if (!event_active) {
        int result = start_event(path, 1);
        if (result == 0 && obstacle_event) obstacle_event_pending = 1;
        return result;
    }
    if (event_queue_count >= EVENT_QUEUE_CAPACITY) {
        errno = ENOSPC;
        return -1;
    }
    tail = (event_queue_head + event_queue_count) % EVENT_QUEUE_CAPACITY;
    event_queue[tail] = path;
    ++event_queue_count;
    if (obstacle_event) obstacle_event_pending = 1;
    return 0;
}

static const char *dequeue_event(void)
{
    const char *path;

    if (event_queue_count == 0U) return NULL;
    path = event_queue[event_queue_head];
    event_queue_head = (event_queue_head + 1U) % EVENT_QUEUE_CAPACITY;
    --event_queue_count;
    return path;
}

static int restore_music(void)
{
    int result;

    event_active = 0;
    active_event_path = NULL;
    active_event_generation = 0U;
    if (music_state == MUSIC_STOPPED || music_path[0] == '\0') return 0;
    result = CargarSonido(music_path);
    if (result == 0) music_generation = GeneracionSolicitadaAudio();
    if (result == 0 && music_resume_frame > 0)
        result = SaltarAFrame(music_resume_frame);
    if (result == 0 && music_state == MUSIC_PAUSED)
        result = Pausar();
    return result;
}

static void process_audio_responses(void)
{
    const char *next_event;

    if (ProcesarRespuestasAudio() != 0) return;
    if (!event_active) {
        if (music_state == MUSIC_PLAYING && music_generation != 0U &&
            GeneracionFinalizadaAudio() >= music_generation) {
            music_state = MUSIC_STOPPED;
            music_generation = 0U;
        }
        return;
    }
    if (active_event_generation == 0U ||
        GeneracionFinalizadaAudio() < active_event_generation) {
        return;
    }

    if (active_event_path != NULL &&
        strcmp(active_event_path, event_sound_path("obstacle")) == 0)
        obstacle_event_pending = 0;

    next_event = dequeue_event();
    if (next_event != NULL) {
        if (start_event(next_event, 0) != 0) {
            if (strcmp(next_event, event_sound_path("obstacle")) == 0)
                obstacle_event_pending = 0;
            fprintf(stderr, "No se pudo reproducir el siguiente evento de audio\n");
            if (restore_music() != 0)
                fprintf(stderr, "No se pudo reanudar la canción tras el evento\n");
        }
    } else if (restore_music() != 0) {
        fprintf(stderr, "No se pudo reanudar la canción tras el evento\n");
    }
}

static void request_shutdown(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static int select_audio_device(long card)
{
    char card_path[64];
    char card_id[64];
    char selected_device[96];
    FILE *card_file;
    int result;

    if (card < 0 || card > 31) return -1;
    snprintf(card_path, sizeof(card_path), "/proc/asound/card%ld/id", card);
    card_file = fopen(card_path, "r");
    if (card_file == NULL || fgets(card_id, sizeof(card_id), card_file) == NULL) {
        if (card_file != NULL) fclose(card_file);
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
    if (FinalizarSonido() != 0 || IniciarSonido(selected_device) != 0)
        return -1;
    snprintf(audio_device, sizeof(audio_device), "%s", selected_device);
    fprintf(stderr, "Seleccionando dispositivo ALSA %s\n", audio_device);

    if (event_active && active_event_path != NULL) {
        active_event_generation = 0U;
        result = CargarSonido(active_event_path);
        if (result == 0)
            active_event_generation = GeneracionSolicitadaAudio();
    } else {
        result = restore_music();
    }
    return result;
}

static int execute_command(char *command)
{
    command[strcspn(command, "\r\n")] = '\0';

    if (strcmp(command, "PLAY") == 0) return request_music(DEFAULT_AUDIO);
    if (strncmp(command, "TRACK ", 6) == 0) {
        char path[512];
        char *end_pointer;
        long index = strtol(command + 6, &end_pointer, 10);
        return *end_pointer == '\0' &&
            resolve_playlist_track(index, path, sizeof(path)) == 0
            ? request_music(path) : -1;
    }
    if (strncmp(command, "EVENT ", 6) == 0)
        return enqueue_event(command + 6);
    if (strcmp(command, "PAUSE") == 0) {
        if (music_state == MUSIC_PAUSED) return 0;
        music_state = MUSIC_PAUSED;
        return event_active ? 0 : Pausar();
    }
    if (strcmp(command, "STOP") == 0) {
        music_state = MUSIC_STOPPED;
        music_path[0] = '\0';
        music_resume_frame = 0;
        music_generation = 0U;
        event_active = 0;
        active_event_path = NULL;
        event_queue_head = 0U;
        event_queue_count = 0U;
        obstacle_event_pending = 0;
        return Stop();
    }
    if (strncmp(command, "VOLUME ", 7) == 0) {
        char *end_pointer;
        long volume = strtol(command + 7, &end_pointer, 10);

        if (*end_pointer != '\0' || volume < 0 || volume > 100) return -1;
        return AjustarVolumen(audio_device, (int)volume);
    }
    if (strncmp(command, "DEVICE ", 7) == 0) {
        char *end_pointer;
        long card = strtol(command + 7, &end_pointer, 10);
        return *end_pointer == '\0' ? select_audio_device(card) : -1;
    }

    fprintf(stderr, "Comando de audio desconocido: %s\n", command);
    return -1;
}

static void process_control_input(int descriptor)
{
    static char command[1024];
    static size_t command_length;
    char input[256];
    ssize_t bytes;

    while ((bytes = read(descriptor, input, sizeof(input))) > 0) {
        for (ssize_t index = 0; index < bytes; ++index) {
            char character = input[index];
            if (character == '\n' || character == '\r') {
                if (command_length > 0U) {
                    command[command_length] = '\0';
                    if (execute_command(command) != 0)
                        fprintf(stderr, "Falló el comando de audio: %s\n", command);
                    command_length = 0U;
                }
            } else if (command_length + 1U < sizeof(command)) {
                command[command_length++] = character;
            } else {
                command_length = 0U;
            }
        }
    }
}

int main(void)
{
    struct pollfd poll_fds[2];
    const char *configured_device;

    signal(SIGINT, request_shutdown);
    signal(SIGTERM, request_shutdown);

    if (mkdir(CONTROL_DIRECTORY, 0750) != 0 && errno != EEXIST) {
        perror("mkdir control audio");
        return 1;
    }

    unlink(CONTROL_FIFO);
    if (mkfifo(CONTROL_FIFO, 0660) != 0) {
        perror("mkfifo control audio");
        return 1;
    }
    if (chmod(CONTROL_FIFO, 0660) != 0) {
        perror("chmod control audio");
        unlink(CONTROL_FIFO);
        return 1;
    }

    poll_fds[0].fd = open(CONTROL_FIFO, O_RDWR | O_NONBLOCK);
    poll_fds[0].events = POLLIN;
    if (poll_fds[0].fd < 0) {
        perror("open control audio");
        unlink(CONTROL_FIFO);
        return 1;
    }

    configured_device = getenv("AURABOT_AUDIO_DEVICE");
    if (configured_device != NULL && configured_device[0] != '\0')
        snprintf(audio_device, sizeof(audio_device), "%s", configured_device);
    else
        select_default_audio_device();
    if (IniciarSonido(audio_device) != 0) {
        fprintf(stderr, "No se pudo inicializar el audio AuraBot\n");
        close(poll_fds[0].fd);
        unlink(CONTROL_FIFO);
        return 1;
    }

    fprintf(stderr, "Servidor de audio listo en %s\n", CONTROL_FIFO);
    while (running) {
        int ready;
        poll_fds[1].fd = DescriptorRespuestasAudio();
        poll_fds[1].events = POLLIN;
        poll_fds[0].revents = poll_fds[1].revents = 0;
        ready = poll(poll_fds, 2, 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll control audio");
            break;
        }
        if (poll_fds[1].revents & (POLLIN | POLLHUP | POLLERR))
            process_audio_responses();
        if (poll_fds[0].revents & POLLIN)
            process_control_input(poll_fds[0].fd);
        process_audio_responses();
    }

    Stop();
    FinalizarSonido();
    close(poll_fds[0].fd);
    unlink(CONTROL_FIFO);
    return 0;
}
