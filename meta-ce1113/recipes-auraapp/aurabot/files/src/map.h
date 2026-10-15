#ifndef AURABOT_MAP_H
#define AURABOT_MAP_H

#include <aurabot.h>

typedef struct {
    unsigned char cells[AURABOT_MAP_CELLS];
    unsigned int revision;
    int min_column;
    int max_column;
    int min_row;
    int max_row;
} aurabot_map_t;

void map_init(aurabot_map_t *map);
void map_update_pose(aurabot_map_t *map, int x_mm, int y_mm,
                     int heading_mrad, int left_obstacle,
                     int right_obstacle);
int map_copy(const aurabot_map_t *map, unsigned char *cells,
             unsigned int capacity, unsigned int *required_size,
             unsigned int *width, unsigned int *height,
             int *origin_x_mm, int *origin_y_mm);

#endif
