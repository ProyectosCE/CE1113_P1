#include <errno.h>
#include <pthread.h>

#include <libgpio.h>

#include "fake_gpio.h"

static pthread_mutex_t fake_lock = PTHREAD_MUTEX_INITIALIZER;
static int current_level;
static int writes[2];
static int failures_remaining;

void fake_gpio_reset(void)
{
    pthread_mutex_lock(&fake_lock);
    current_level = 0;
    writes[0] = 0;
    writes[1] = 0;
    failures_remaining = 0;
    pthread_mutex_unlock(&fake_lock);
}

void fake_gpio_fail_next_writes(int count)
{
    pthread_mutex_lock(&fake_lock);
    failures_remaining = count;
    pthread_mutex_unlock(&fake_lock);
}

int fake_gpio_write_count(int level)
{
    pthread_mutex_lock(&fake_lock);
    int count = writes[level];
    pthread_mutex_unlock(&fake_lock);
    return count;
}

int fake_gpio_current_level(void)
{
    pthread_mutex_lock(&fake_lock);
    int level = current_level;
    pthread_mutex_unlock(&fake_lock);
    return level;
}

int pinMode(int pin, const char *mode)
{
    (void)pin;
    (void)mode;
    return 0;
}

int digitalWrite(int pin, int value)
{
    (void)pin;
    pthread_mutex_lock(&fake_lock);
    if (failures_remaining > 0) {
        --failures_remaining;
        pthread_mutex_unlock(&fake_lock);
        errno = EIO;
        return -1;
    }
    current_level = value;
    ++writes[value];
    pthread_mutex_unlock(&fake_lock);
    return 0;
}

int digitalRead(int pin)
{
    (void)pin;
    return fake_gpio_current_level();
}
