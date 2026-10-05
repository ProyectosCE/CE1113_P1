#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hardware.h"
#include "fake_hardware.h"

#define FAKE_TICKS_PER_REVOLUTION 35U
#define FAKE_WHEEL_CIRCUMFERENCE_MM 204U
#define FAKE_WHEEL_BASE_MM 160U

static int initialized;
static int left_speed;
static int right_speed;
static int left_obstacle;
static int right_obstacle;
static struct timespec sample_started;
static int left_tick_remainder;
static int right_tick_remainder;
static int audio_volume = 50;

static long long elapsed_ms(const struct timespec *start,
                            const struct timespec *end)
{
    long long value = (long long)(end->tv_sec - start->tv_sec) * 1000LL +
        (end->tv_nsec - start->tv_nsec) / 1000000LL;
    return value > 0 ? value : 1;
}

int hardware_init(void)
{
    initialized = 1;
    left_speed = right_speed = 0;
    clock_gettime(CLOCK_MONOTONIC, &sample_started);
    return 0;
}

void hardware_shutdown(void) { initialized = 0; left_speed = right_speed = 0; }

int hardware_drive(int left_percent, int right_percent)
{
    if (!initialized || left_percent < -100 || left_percent > 100 ||
        right_percent < -100 || right_percent > 100) {
        errno = EINVAL;
        return -1;
    }
    left_speed = left_percent;
    right_speed = right_percent;
    return 0;
}

int hardware_stop(void) { left_speed = right_speed = 0; return 0; }

int hardware_read_sensors(sensor_snapshot_t *result)
{
    if (result == NULL) return -1;
    result->left_obstacle = left_obstacle;
    result->right_obstacle = right_obstacle;
    return 0;
}

static unsigned int calculate_ticks(int speed, long long milliseconds,
                                    int *remainder)
{
    long long numerator;
    unsigned int ticks;
    int magnitude = speed < 0 ? -speed : speed;
    /* 100 % equivale aproximadamente a 500 mm/s en el simulador. */
    numerator = *remainder + (long long)magnitude * 5LL * milliseconds *
        FAKE_TICKS_PER_REVOLUTION;
    ticks = (unsigned int)(numerator /
        (1000LL * FAKE_WHEEL_CIRCUMFERENCE_MM));
    *remainder = (int)(numerator %
        (1000LL * FAKE_WHEEL_CIRCUMFERENCE_MM));
    return ticks;
}

int hardware_take_encoder_sample(encoder_snapshot_t *result)
{
    struct timespec now;
    long long milliseconds;
    if (result == NULL) return -1;
    clock_gettime(CLOCK_MONOTONIC, &now);
    milliseconds = elapsed_ms(&sample_started, &now);
    sample_started = now;
    memset(result, 0, sizeof(*result));
    result->left_direction = left_speed > 0 ? 1 : left_speed < 0 ? -1 : 0;
    result->right_direction = right_speed > 0 ? 1 : right_speed < 0 ? -1 : 0;
    result->left_ticks = calculate_ticks(left_speed, milliseconds,
                                         &left_tick_remainder);
    result->right_ticks = calculate_ticks(right_speed, milliseconds,
                                          &right_tick_remainder);
    result->sample_time_ms = (int)milliseconds;
    result->left_angular_velocity_mrad_s = (int)(6283185LL * result->left_ticks /
        (FAKE_TICKS_PER_REVOLUTION * milliseconds));
    result->right_angular_velocity_mrad_s = (int)(6283185LL * result->right_ticks /
        (FAKE_TICKS_PER_REVOLUTION * milliseconds));
    return 0;
}

int hardware_set_leds(int power, int manual, int autonomous, int obstacle)
{
    (void)power; (void)manual; (void)autonomous; (void)obstacle;
    return 0;
}

int hardware_audio_event(const char *event_name)
{ return event_name == NULL ? -1 : 0; }
int hardware_audio_play(unsigned int track_index)
{ return track_index < 4U ? 0 : -1; }
int hardware_audio_pause(void) { return 0; }
int hardware_audio_stop(void) { return 0; }
int hardware_audio_set_volume(int volume_percent)
{
    if (volume_percent < 0 || volume_percent > 100) return -1;
    audio_volume = volume_percent;
    return 0;
}
unsigned int hardware_audio_track_count(void) { return 4U; }
int hardware_audio_track_name(unsigned int track_index, char *name,
                              unsigned int capacity)
{
    static const char *names[] = { "boot.mp3", "manual.mp3",
                                   "autonomous.mp3", "obstacle.mp3" };
    unsigned int length;
    if (track_index >= 4U || name == NULL) return -1;
    length = (unsigned int)strlen(names[track_index]) + 1U;
    if (length > capacity) return -1;
    memcpy(name, names[track_index], length);
    return 0;
}
unsigned int hardware_left_ticks_per_revolution(void)
{ return FAKE_TICKS_PER_REVOLUTION; }
unsigned int hardware_right_ticks_per_revolution(void)
{ return FAKE_TICKS_PER_REVOLUTION; }
unsigned int hardware_left_wheel_circumference_mm(void)
{ return FAKE_WHEEL_CIRCUMFERENCE_MM; }
unsigned int hardware_right_wheel_circumference_mm(void)
{ return FAKE_WHEEL_CIRCUMFERENCE_MM; }
unsigned int hardware_wheel_base_mm(void) { return FAKE_WHEEL_BASE_MM; }

void fake_hardware_set_obstacles(int left, int right)
{
    left_obstacle = !!left;
    right_obstacle = !!right;
}

void fake_hardware_get_motor_speeds(int *left, int *right)
{
    if (left != NULL) *left = left_speed;
    if (right != NULL) *right = right_speed;
}
