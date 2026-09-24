#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/libaudio.h"


static pid_t g_pid = -1;
static int g_fd = -1;
static char g_fifo_path[256];


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
    if (audio_device == NULL) {
        return -1;
    }

    if (g_pid > 0) {
        return -1;
    }

    snprintf(g_fifo_path, sizeof(g_fifo_path), "/tmp/mpg123_%d.cmd", (int)getpid());

    if (mkfifo(g_fifo_path, 0666) != 0 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        int fifo_fd = open(g_fifo_path, O_RDONLY);

        if (fifo_fd < 0) {
            perror("open fifo");
            _exit(127);
        }

        dup2(fifo_fd, STDIN_FILENO);
        close(fifo_fd);

        int log_fd = open("/tmp/mpg123.log", O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            close(log_fd);
        }

        execlp("mpg123", "mpg123", "-R", "-o", "alsa", "-a", audio_device, (char *)NULL);

        perror("execlp mpg123");
        _exit(127);
    }

    g_pid = pid;

    g_fd = open(g_fifo_path, O_WRONLY);

    if (g_fd < 0) {
        perror("open fifo");
        return -1;
    }

    return 0;
}


int CargarSonido(const char *archivo)
{
    if (archivo == NULL) {
        return -1;
    }

    char command[560];

    int length = snprintf(command, sizeof(command), "LOAD %s", archivo);

    if (length < 0 || (size_t)length >= sizeof(command)) {
        return -1;
    }

    return EnviarComando(command);
}


int Pausar(void)
{
    return EnviarComando("PAUSE");
}


int Stop(void)
{
    return EnviarComando("STOP");
}