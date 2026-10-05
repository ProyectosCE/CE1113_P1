#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libgpio.h>
#include <libleds.h>
#include <libpwm.h>
#include <libsensors.h>

#include "hardware.h"
#include "hardware_config.h"

#define AUDIO_CONTROL_FIFO "/run/aurabot-audio/control"
#define PLAYLIST_PATH "/media/audio/playlist.txt"

static pthread_t encoder_thread;
static pthread_mutex_t encoder_lock = PTHREAD_MUTEX_INITIALIZER;
static int encoder_running;
static unsigned int left_ticks;
static unsigned int right_ticks;
static int left_direction;
static int right_direction;
static int left_speed;
static int right_speed;
static struct timespec sample_started;

static long long elapsed_ms(const struct timespec *start,
                            const struct timespec *end)
{
    long long value = (long long)(end->tv_sec - start->tv_sec) * 1000LL +
        (end->tv_nsec - start->tv_nsec) / 1000000LL;
    return value > 0 ? value : 1;
}

static int all_pins_configured(void)
{
    return AURABOT_LEFT_PWM_PIN >= 0 && AURABOT_LEFT_IN1_PIN >= 0 &&
        AURABOT_LEFT_IN2_PIN >= 0 && AURABOT_RIGHT_PWM_PIN >= 0 &&
        AURABOT_RIGHT_IN1_PIN >= 0 && AURABOT_RIGHT_IN2_PIN >= 0 &&
        AURABOT_LEFT_SENSOR_PIN >= 0 && AURABOT_RIGHT_SENSOR_PIN >= 0 &&
        AURABOT_LEFT_ENCODER_PIN >= 0 && AURABOT_RIGHT_ENCODER_PIN >= 0 &&
        AURABOT_LED_POWER_PIN >= 0 && AURABOT_LED_MANUAL_PIN >= 0 &&
        AURABOT_LED_AUTO_PIN >= 0 && AURABOT_LED_OBSTACLE_PIN >= 0;
}

static void *encoder_worker(void *unused)
{
    int previous_left = 0;
    int previous_right = 0;
    struct timespec delay = { .tv_sec = 0,
        .tv_nsec = AURABOT_ENCODER_POLL_US * 1000L };
    (void)unused;
    previous_left = digitalRead(AURABOT_LEFT_ENCODER_PIN);
    previous_right = digitalRead(AURABOT_RIGHT_ENCODER_PIN);
    for (;;) {
        int keep_running;
        int current_left = digitalRead(AURABOT_LEFT_ENCODER_PIN);
        int current_right = digitalRead(AURABOT_RIGHT_ENCODER_PIN);
        pthread_mutex_lock(&encoder_lock);
        keep_running = encoder_running;
        if (current_left == 1 && previous_left == 0) ++left_ticks;
        if (current_right == 1 && previous_right == 0) ++right_ticks;
        pthread_mutex_unlock(&encoder_lock);
        if (!keep_running) break;
        if (current_left >= 0) previous_left = current_left;
        if (current_right >= 0) previous_right = current_right;
        nanosleep(&delay, NULL);
    }
    return NULL;
}

static int configure_motor_direction(int in1, int in2, int direction)
{
    int first = direction > 0 ? 1 : 0;
    int second = direction < 0 ? 1 : 0;
    if (pinMode(in1, "out") != 0 || pinMode(in2, "out") != 0)
        return -1;
    if (digitalWrite(in1, first) != 0 || digitalWrite(in2, second) != 0)
        return -1;
    return 0;
}

static int set_motor(int pwm_pin, int in1, int in2, int speed)
{
    int magnitude = speed < 0 ? -speed : speed;
    int direction = speed > 0 ? 1 : speed < 0 ? -1 : 0;
    if (stopPWM(pwm_pin) != 0) return -1;
    if (configure_motor_direction(in1, in2, direction) != 0) return -1;
    if (magnitude == 0) return 0;
    return setPWM(pwm_pin, AURABOT_PWM_FREQUENCY_HZ, magnitude);
}

int hardware_init(void)
{
    int result;
    if (!all_pins_configured()) {
        errno = ENODEV;
        fprintf(stderr, "aurabot: configure los pines en hardware_config.h\n");
        return -1;
    }
    if (pinMode(AURABOT_LEFT_SENSOR_PIN, "in") != 0 ||
        pinMode(AURABOT_RIGHT_SENSOR_PIN, "in") != 0 ||
        pinMode(AURABOT_LEFT_ENCODER_PIN, "in") != 0 ||
        pinMode(AURABOT_RIGHT_ENCODER_PIN, "in") != 0)
        return -1;
    if (hardware_stop() != 0 || hardware_set_leds(0, 0, 0, 0) != 0)
        return -1;
    left_ticks = right_ticks = 0U;
    left_direction = right_direction = 0;
    clock_gettime(CLOCK_MONOTONIC, &sample_started);
    encoder_running = 1;
    result = pthread_create(&encoder_thread, NULL, encoder_worker, NULL);
    if (result != 0) {
        encoder_running = 0;
        errno = result;
        return -1;
    }
    return 0;
}

void hardware_shutdown(void)
{
    if (encoder_running) {
        pthread_mutex_lock(&encoder_lock);
        encoder_running = 0;
        pthread_mutex_unlock(&encoder_lock);
        (void)pthread_join(encoder_thread, NULL);
    }
    (void)hardware_stop();
}

int hardware_drive(int left_percent, int right_percent)
{
    int new_left_direction;
    int new_right_direction;
    if (left_percent < -100 || left_percent > 100 ||
        right_percent < -100 || right_percent > 100) {
        errno = EINVAL;
        return -1;
    }
    if (set_motor(AURABOT_LEFT_PWM_PIN, AURABOT_LEFT_IN1_PIN,
                  AURABOT_LEFT_IN2_PIN, left_percent) != 0 ||
        set_motor(AURABOT_RIGHT_PWM_PIN, AURABOT_RIGHT_IN1_PIN,
                  AURABOT_RIGHT_IN2_PIN, right_percent) != 0) {
        (void)hardware_stop();
        return -1;
    }
    new_left_direction = left_percent > 0 ? 1 : left_percent < 0 ? -1 : 0;
    new_right_direction = right_percent > 0 ? 1 : right_percent < 0 ? -1 : 0;
    pthread_mutex_lock(&encoder_lock);
    left_direction = new_left_direction;
    right_direction = new_right_direction;
    left_speed = left_percent;
    right_speed = right_percent;
    pthread_mutex_unlock(&encoder_lock);
    return 0;
}

int hardware_stop(void)
{
    int result = 0;
    if (AURABOT_LEFT_PWM_PIN >= 0 && stopPWM(AURABOT_LEFT_PWM_PIN) != 0)
        result = -1;
    if (AURABOT_RIGHT_PWM_PIN >= 0 && stopPWM(AURABOT_RIGHT_PWM_PIN) != 0)
        result = -1;
    if (AURABOT_LEFT_IN1_PIN >= 0 &&
        configure_motor_direction(AURABOT_LEFT_IN1_PIN,
                                  AURABOT_LEFT_IN2_PIN, 0) != 0)
        result = -1;
    if (AURABOT_RIGHT_IN1_PIN >= 0 &&
        configure_motor_direction(AURABOT_RIGHT_IN1_PIN,
                                  AURABOT_RIGHT_IN2_PIN, 0) != 0)
        result = -1;
    pthread_mutex_lock(&encoder_lock);
    left_direction = right_direction = 0;
    left_speed = right_speed = 0;
    pthread_mutex_unlock(&encoder_lock);
    return result;
}

int hardware_read_sensors(sensor_snapshot_t *result)
{
    int left;
    int right;
    if (result == NULL) { errno = EINVAL; return -1; }
    left = sensorReadDigital(AURABOT_LEFT_SENSOR_PIN,
                             AURABOT_SENSOR_ACTIVE_LOW);
    right = sensorReadDigital(AURABOT_RIGHT_SENSOR_PIN,
                              AURABOT_SENSOR_ACTIVE_LOW);
    if (left < 0 || right < 0) return -1;
    result->left_obstacle = left;
    result->right_obstacle = right;
    return 0;
}

int hardware_take_encoder_sample(encoder_snapshot_t *result)
{
    struct timespec now;
    long long sample_ms;
    if (result == NULL) { errno = EINVAL; return -1; }
    clock_gettime(CLOCK_MONOTONIC, &now);
    pthread_mutex_lock(&encoder_lock);
    result->left_ticks = left_ticks;
    result->right_ticks = right_ticks;
    result->left_direction = left_direction;
    result->right_direction = right_direction;
    left_ticks = right_ticks = 0U;
    sample_ms = elapsed_ms(&sample_started, &now);
    sample_started = now;
    pthread_mutex_unlock(&encoder_lock);
    result->sample_time_ms = (int)sample_ms;
    result->left_angular_velocity_mrad_s = (int)(
        6283185LL * result->left_ticks /
        (AURABOT_LEFT_TICKS_PER_REVOLUTION * sample_ms));
    result->right_angular_velocity_mrad_s = (int)(
        6283185LL * result->right_ticks /
        (AURABOT_RIGHT_TICKS_PER_REVOLUTION * sample_ms));
    return 0;
}

int hardware_set_leds(int power, int manual, int autonomous, int obstacle)
{
    if (ledSet(AURABOT_LED_POWER_PIN, AURABOT_LED_ACTIVE_HIGH, power) != 0 ||
        ledSet(AURABOT_LED_MANUAL_PIN, AURABOT_LED_ACTIVE_HIGH, manual) != 0 ||
        ledSet(AURABOT_LED_AUTO_PIN, AURABOT_LED_ACTIVE_HIGH, autonomous) != 0 ||
        ledSet(AURABOT_LED_OBSTACLE_PIN, AURABOT_LED_ACTIVE_HIGH, obstacle) != 0)
        return -1;
    return 0;
}

static int send_audio_command(const char *command)
{
    int descriptor;
    unsigned int length;
    int result = 0;
    if (command == NULL) { errno = EINVAL; return -1; }
    descriptor = open(AUDIO_CONTROL_FIFO, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (descriptor < 0) return -1;
    length = (unsigned int)strlen(command);
    if (write(descriptor, command, length) != (int)length ||
        write(descriptor, "\n", 1) != 1)
        result = -1;
    close(descriptor);
    return result;
}

int hardware_audio_event(const char *event_name)
{
    if (event_name == NULL) return -1;
    if (strcmp(event_name, "boot") == 0) return hardware_audio_play(0U);
    if (strcmp(event_name, "manual") == 0) return hardware_audio_play(1U);
    if (strcmp(event_name, "autonomous") == 0) return hardware_audio_play(2U);
    if (strcmp(event_name, "obstacle") == 0) return hardware_audio_play(3U);
    errno = EINVAL;
    return -1;
}

int hardware_audio_play(unsigned int track_index)
{
    char command[48];
    snprintf(command, sizeof(command), "TRACK %u", track_index);
    return send_audio_command(command);
}

int hardware_audio_pause(void) { return send_audio_command("PAUSE"); }
int hardware_audio_stop(void) { return send_audio_command("STOP"); }

int hardware_audio_set_volume(int volume_percent)
{
    char command[48];
    if (volume_percent < 0 || volume_percent > 100) { errno = EINVAL; return -1; }
    snprintf(command, sizeof(command), "VOLUME %d", volume_percent);
    return send_audio_command(command);
}

unsigned int hardware_audio_track_count(void)
{
    FILE *playlist = fopen(PLAYLIST_PATH, "r");
    char line[512];
    unsigned int count = 0U;
    if (playlist == NULL) return 0U;
    while (fgets(line, sizeof(line), playlist) != NULL)
        if (line[0] != '\n' && line[0] != '\r' && line[0] != '\0') ++count;
    fclose(playlist);
    return count;
}

int hardware_audio_track_name(unsigned int track_index, char *name,
                              unsigned int capacity)
{
    FILE *playlist;
    char line[512];
    unsigned int current = 0U;
    const char *base;
    unsigned int length;
    if (name == NULL || capacity == 0U) { errno = EINVAL; return -1; }
    playlist = fopen(PLAYLIST_PATH, "r");
    if (playlist == NULL) return -1;
    while (fgets(line, sizeof(line), playlist) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;
        if (current++ != track_index) continue;
        base = strrchr(line, '/');
        base = base == NULL ? line : base + 1;
        length = (unsigned int)strlen(base) + 1U;
        if (length > capacity) { fclose(playlist); errno = ENOSPC; return -1; }
        memcpy(name, base, length);
        fclose(playlist);
        return 0;
    }
    fclose(playlist);
    errno = ENOENT;
    return -1;
}

unsigned int hardware_left_ticks_per_revolution(void)
{ return AURABOT_LEFT_TICKS_PER_REVOLUTION; }
unsigned int hardware_right_ticks_per_revolution(void)
{ return AURABOT_RIGHT_TICKS_PER_REVOLUTION; }
unsigned int hardware_left_wheel_circumference_mm(void)
{ return AURABOT_LEFT_WHEEL_CIRCUMFERENCE_MM; }
unsigned int hardware_right_wheel_circumference_mm(void)
{ return AURABOT_RIGHT_WHEEL_CIRCUMFERENCE_MM; }
unsigned int hardware_wheel_base_mm(void)
{ return AURABOT_WHEEL_BASE_MM; }
