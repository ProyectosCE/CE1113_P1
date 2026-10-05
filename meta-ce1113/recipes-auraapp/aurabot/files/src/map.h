#ifndef AURABOT_MAP_H
#define AURABOT_MAP_H

#include <aurabot.h>

typedef struct {
    unsigned char cells[AURABOT_MAP_CELLS];
    unsigned int revision;
} aurabot_map_t;

void map_init(aurabot_map_t *map);
void map_update_pose(aurabot_map_t *map, int x_mm, int y_mm,
                     int heading_mrad, int left_obstacle,
                     int right_obstacle);
int map_copy(const aurabot_map_t *map, unsigned char *cells,
             unsigned int capacity, unsigned int *required_size);

#endif
