#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libaudio.h>


static pid_t g_pid = -1;
static int g_fd = -1;
static int g_response_fd = -1;
static char g_response_buffer[4096];
static size_t g_response_length;
static long g_current_frame;
static int g_playback_status;
static int g_end_reported;
static unsigned long g_requested_generation;
static unsigned long g_started_generation;
static unsigned long g_finished_generation;
static unsigned long g_current_generation;
static unsigned long g_pending_generations[32];
static unsigned int g_pending_head;
static unsigned int g_pending_count;


static int EnviarComando(const char *command)
{
    if (command == NULL || g_fd < 0) {
        return -1;
    }

    char buffer[600];

    int length = snprintf(buffer, sizeof(buffer), "%s\n", command);

    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        return -1;
    }

    if (write(g_fd, buffer, (size_t)length) != length) {
        perror("write mpg123 command");
        return -1;
    }

    return 0;
}


int IniciarSonido(const char *audio_device)
{
    int command_pipe[2] = {-1, -1};
    int response_pipe[2] = {-1, -1};

    if (audio_device == NULL) {
        return -1;
    }

    if (g_pid > 0) {
        return -1;
    }

    if (pipe(command_pipe) != 0 || pipe(response_pipe) != 0) {
        perror("pipe mpg123");
        if (command_pipe[0] >= 0) close(command_pipe[0]);
        if (command_pipe[1] >= 0) close(command_pipe[1]);
        if (response_pipe[0] >= 0) close(response_pipe[0]);
        if (response_pipe[1] >= 0) close(response_pipe[1]);
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        close(command_pipe[0]);
        close(command_pipe[1]);
        close(response_pipe[0]);
        close(response_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        close(command_pipe[1]);
        close(response_pipe[0]);
        if (dup2(command_pipe[0], STDIN_FILENO) < 0 ||
            dup2(response_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(response_pipe[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(command_pipe[0]);
        close(response_pipe[1]);

        execlp("mpg123", "mpg123", "-R", "--remote-err", "-o", "alsa",
               "-a", audio_device, (char *)NULL);

        perror("execlp mpg123");
        _exit(127);
    }

    g_pid = pid;
    close(command_pipe[0]);
    close(response_pipe[1]);
    g_fd = command_pipe[1];
    g_response_fd = response_pipe[0];
    if (fcntl(g_response_fd, F_SETFL,
              fcntl(g_response_fd, F_GETFL) | O_NONBLOCK) < 0) {
        perror("fcntl mpg123 response");
        close(g_fd);
        close(g_response_fd);
        g_fd = -1;
        g_response_fd = -1;
        kill(g_pid, SIGTERM);
        waitpid(g_pid, NULL, 0);
        g_pid = -1;
        return -1;
    }
    g_response_length = 0U;
    g_current_frame = 0;
    g_playback_status = 0;
    g_end_reported = 0;
    g_requested_generation = 0U;
    g_started_generation = 0U;
    g_finished_generation = 0U;
    g_current_generation = 0U;
    g_pending_head = 0U;
    g_pending_count = 0U;

    return 0;
}


int CargarSonido(const char *archivo)
{
    int result;
    unsigned int tail;

    if (archivo == NULL) {
        return -1;
    }
    if (g_pending_count >= sizeof(g_pending_generations) /
        sizeof(g_pending_generations[0])) {
        errno = ENOSPC;
        return -1;
    }

    char command[560];

    int length = snprintf(command, sizeof(command), "LOAD %s", archivo);

    if (length < 0 || (size_t)length >= sizeof(command)) {
        return -1;
    }

    result = EnviarComando(command);
    if (result == 0) {
        ++g_requested_generation;
        tail = (g_pending_head + g_pending_count) %
            (sizeof(g_pending_generations) / sizeof(g_pending_generations[0]));
        g_pending_generations[tail] = g_requested_generation;
        ++g_pending_count;
    }
    return result;
}

int SaltarAFrame(long frame)
{
    char command[64];
    int length;

    if (frame < 0) {
        return -1;
    }
    length = snprintf(command, sizeof(command), "JUMP %ld", frame);
    if (length < 0 || (size_t)length >= sizeof(command)) {
        return -1;
    }
    return EnviarComando(command);
}

static void ProcesarLineaRespuesta(const char *line)
{
    long frame;
    int status;

    if (sscanf(line, "@F %ld", &frame) == 1) {
        g_current_frame = frame;
        return;
    }
    if (sscanf(line, "@J %ld", &frame) == 1) {
        g_current_frame = frame;
        return;
    }
    if (strncmp(line, "@S ", 3) == 0 && g_pending_count > 0U) {
        g_current_generation = g_pending_generations[g_pending_head];
        g_pending_head = (g_pending_head + 1U) %
            (sizeof(g_pending_generations) / sizeof(g_pending_generations[0]));
        --g_pending_count;
        g_started_generation = g_current_generation;
        g_current_frame = 0;
        g_end_reported = 0;
        return;
    }
    if (sscanf(line, "@P %d", &status) != 1) {
        return;
    }
    if (status == 2) {
        g_end_reported = 0;
    } else if (status == 3) {
        g_finished_generation = g_current_generation;
        g_end_reported = 1;
    } else if (status == 0 && g_playback_status == 2 && !g_end_reported) {
        /* Compatibilidad con mpg123 anteriores al estado remoto @P 3. */
        g_finished_generation = g_current_generation;
        g_end_reported = 1;
    }
    g_playback_status = status;
}

int ProcesarRespuestasAudio(void)
{
    char input[1024];
    ssize_t bytes;

    if (g_response_fd < 0) {
        return -1;
    }
    for (;;) {
        bytes = read(g_response_fd, input, sizeof(input));
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (bytes == 0) {
            break;
        }
        for (ssize_t index = 0; index < bytes; ++index) {
            char character = input[index];
            if (character == '\n') {
                g_response_buffer[g_response_length] = '\0';
                ProcesarLineaRespuesta(g_response_buffer);
                g_response_length = 0U;
            } else if (character != '\r') {
                if (g_response_length + 1U < sizeof(g_response_buffer)) {
                    g_response_buffer[g_response_length++] = character;
                } else {
                    g_response_length = 0U;
                }
            }
        }
    }
    return 0;
}

int DescriptorRespuestasAudio(void)
{
    return g_response_fd;
}

long FrameActualAudio(void)
{
    return g_current_frame;
}

unsigned long GeneracionIniciadaAudio(void)
{
    return g_started_generation;
}

unsigned long GeneracionSolicitadaAudio(void)
{
    return g_requested_generation;
}

unsigned long GeneracionFinalizadaAudio(void)
{
    return g_finished_generation;
}


int Pausar(void)
{
    return EnviarComando("PAUSE");
}


int Stop(void)
{
    return EnviarComando("STOP");
}

int CargarListaReproduccion(const char *playlist_path)
{
    char command[560];
    int length;

    if (playlist_path == NULL || playlist_path[0] == '\0') {
        return -1;
    }
    length = snprintf(command, sizeof(command), "LOADLIST 1 %s", playlist_path);
    if (length < 0 || (size_t)length >= sizeof(command)) {
        return -1;
    }
    return EnviarComando(command);
}



int AjustarVolumen(const char *audio_device, int volume_percent)
{
    char mixer_device[128];
    char volume[8];
    const char *device_start;
    const char *device_end;
    pid_t pid;
    int status;

    if (audio_device == NULL || volume_percent < 0 || volume_percent > 100) {
        return -1;
    }

    /* Convierte plughw:Headphones,0 en el dispositivo de control
     * hw:Headphones. amixer controla el jack mediante el elemento PCM. */
    device_start = strncmp(audio_device, "plughw:", 7) == 0
        ? audio_device + 7 : audio_device;
    device_end = strchr(device_start, ',');
    if (device_end == NULL) {
        device_end = device_start + strlen(device_start);
    }
    if (device_end == device_start ||
        (size_t)(device_end - device_start) > sizeof(mixer_device) - 4) {
        return -1;
    }
    snprintf(mixer_device, sizeof(mixer_device), "hw:%.*s",
        (int)(device_end - device_start), device_start);
    snprintf(volume, sizeof(volume), "%d%%", volume_percent);

    pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        execlp("amixer", "amixer", "-q", "-D", mixer_device,
            "sset", "PCM", volume, (char *)NULL);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}


int FinalizarSonido(void)
{
    if (g_pid <= 0) {
        return 0;
    }

    EnviarComando("QUIT");
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
    if (g_response_fd >= 0) {
        close(g_response_fd);
        g_response_fd = -1;
    }

    if (waitpid(g_pid, NULL, 0) < 0 && errno != ECHILD) {
        return -1;
    }

    g_pid = -1;
    g_response_length = 0U;
    return 0;
}
