#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/libaudio.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define FIFO "/tmp/mpg123.cmd"
#define PID_FILE "/tmp/mpg123.pid"
#define PATH "/sys/class/gpio"
#define base 512

 
static pid_t g_pid = -1;
static int   g_fd  = -1;
static char  g_fifo_path[256];


int IniciarSonido(const char *audio_device)
{
    if (g_pid > 0)          
        return -1;
 
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
        if (fifo_fd < 0) { perror("open fifo (hijo)"); _exit(127); }
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
        perror("open fifo (padre)");
        return -1;
    }
 
    return 0;
}
 
int EnviarComando(const char *cmd)
{
    if (g_fd < 0)
        return -1;
 
    char buf[600];
    int n = snprintf(buf, sizeof(buf), "%s\n", cmd);
    if (n < 0 || (size_t)n >= sizeof(buf))
        return -1;
 
    if (write(g_fd, buf, (size_t)n) != n) {
        perror("write mpg123 cmd");
        return -1;
    }
    return 0;
}
 
int CargarSonido(const char *path)
{
    char cmd[560];
    snprintf(cmd, sizeof(cmd), "LOAD %s", path);
    return EnviarComando(cmd);
}


int Pausar(void) { return EnviarComando("PAUSE"); }
int Stop(void)  { return EnviarComando("STOP"); }



//falta meter mas comandos investigar los comandos remotos para mpg123

//ademas de poder cargar una playlist en vez de una cancion



