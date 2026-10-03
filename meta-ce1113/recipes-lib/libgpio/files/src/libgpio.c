#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <libgpio.h>

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_BASE 512

static int write_text(const char *path, const char *text)
{
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    size_t length = strlen(text);
    ssize_t written = write(fd, text, length);
    int saved_errno = errno;
    if (close(fd) != 0 && written == (ssize_t)length) {
        return -1;
    }
    if (written != (ssize_t)length) {
        errno = written < 0 ? saved_errno : EIO;
        return -1;
    }
    return 0;
}

static int gpio_path(int pin, const char *attribute, char *path, size_t size)
{
    int length = snprintf(path, size, "%s/gpio%d/%s", GPIO_PATH,
                          pin + GPIO_BASE, attribute);
    if (length < 0 || (size_t)length >= size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static int wait_for_gpio_attribute(int pin, const char *attribute)
{
    char path[128];
    struct timespec delay = { .tv_sec = 0, .tv_nsec = 10000000L };

    if (gpio_path(pin, attribute, path, sizeof(path)) != 0) {
        return -1;
    }

    for (int attempt = 0; attempt < 100; ++attempt) {
        struct stat info;
        if (stat(path, &info) == 0 && S_ISREG(info.st_mode)) {
            return 0;
        }
        while (nanosleep(&delay, &delay) != 0) {
            if (errno != EINTR) {
                return -1;
            }
        }
        delay.tv_sec = 0;
        delay.tv_nsec = 10000000L;
    }
    errno = ENOENT;
    return -1;
}

int pinMode(int pin, const char *mode)
{
    char path[128];
    char number[24];

    if (pin < 0 || mode == NULL ||
        (strcmp(mode, "in") != 0 && strcmp(mode, "out") != 0)) {
        errno = EINVAL;
        return -1;
    }

    snprintf(path, sizeof(path), "%s/gpio%d", GPIO_PATH, pin + GPIO_BASE);
    if (access(path, F_OK) != 0) {
        snprintf(number, sizeof(number), "%d\n", pin + GPIO_BASE);
        if (write_text(GPIO_PATH "/export", number) != 0 && errno != EBUSY) {
            return -1;
        }
    }

    /* El directorio puede aparecer antes que sus atributos. Esperar el archivo
     * que realmente se va a usar evita una carrera durante export. */
    if (wait_for_gpio_attribute(pin, "direction") != 0) {
        return -1;
    }

    if (gpio_path(pin, "direction", path, sizeof(path)) != 0) {
        return -1;
    }
    char direction[8];
    snprintf(direction, sizeof(direction), "%s\n", mode);
    return write_text(path, direction);
}

int digitalWrite(int pin, int value)
{
    char path[128];
    char text[3];

    if (pin < 0 || (value != 0 && value != 1)) {
        errno = EINVAL;
        return -1;
    }
    if (gpio_path(pin, "value", path, sizeof(path)) != 0) {
        return -1;
    }
    snprintf(text, sizeof(text), "%d\n", value);
    return write_text(path, text);
}

int digitalRead(int pin)
{
    char path[128];
    char text[4];
    int fd;
    ssize_t count;

    if (pin < 0 || gpio_path(pin, "value", path, sizeof(path)) != 0) {
        errno = EINVAL;
        return -1;
    }
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    count = read(fd, text, sizeof(text) - 1);
    int saved_errno = errno;
    close(fd);
    if (count <= 0) {
        errno = saved_errno;
        return -1;
    }
    text[count] = '\0';
    return text[0] == '1' ? 1 : 0;
}
