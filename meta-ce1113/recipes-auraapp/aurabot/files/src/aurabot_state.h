#ifndef AURABOT_STATE_H
#define AURABOT_STATE_H

#include <time.h>

#include <aurabot.h>

#include "map.h"

typedef struct {
    aurabot_status_t public_status;
    aurabot_map_t map;
    unsigned char owner_token[AURABOT_OWNER_TOKEN_SIZE];
    int owner_active;
    struct timespec owner_last_heartbeat;
    struct timespec last_tick;
    struct timespec maneuver_started;
    struct timespec motion_last_tick;
    int left_distance_remainder;
    int right_distance_remainder;
    int maneuver_distance_mm;
    int maneuver_angle_mrad;
    int previous_heading_mrad;
    int turn_direction;
    int initialized;
} aurabot_state_t;

#endif
