#include <string.h>

#include "map.h"

#define HALF_TURN_MRAD 3142
#define FULL_TURN_MRAD 6283
#define SENSOR_OFFSET_MRAD 785

/* Seno y coseno escalados por 1000 para ángulos en pasos de 15 grados. */
static const int cosine_table[24] = {
    1000, 966, 866, 707, 500, 259, 0, -259, -500, -707, -866, -966,
    -1000, -966, -866, -707, -500, -259, 0, 259, 500, 707, 866, 966
};
static const int sine_table[24] = {
    0, 259, 500, 707, 866, 966, 1000, 966, 866, 707, 500, 259,
    0, -259, -500, -707, -866, -966, -1000, -966, -866, -707, -500, -259
};

static int normalize_heading(int heading_mrad)
{
    while (heading_mrad > HALF_TURN_MRAD) heading_mrad -= FULL_TURN_MRAD;
    while (heading_mrad <= -HALF_TURN_MRAD) heading_mrad += FULL_TURN_MRAD;
    return heading_mrad;
}

static int direction_index(int heading_mrad)
{
    int normalized = normalize_heading(heading_mrad);
    int index;
    if (normalized < 0) normalized += FULL_TURN_MRAD;
    index = (normalized + 131) / 262;
    return index % 24;
}

static int coordinate_to_cell(int millimeters, int limit)
{
    int cell = millimeters / AURABOT_MAP_CELL_MM + limit / 2;
    if (millimeters < 0 && millimeters % AURABOT_MAP_CELL_MM != 0) --cell;
    return cell;
}

static void set_cell(aurabot_map_t *map, int column, int row,
                     unsigned char value)
{
    unsigned int index;
    if (column < 0 || column >= AURABOT_MAP_WIDTH ||
        row < 0 || row >= AURABOT_MAP_HEIGHT)
        return;
    index = (unsigned int)(row * AURABOT_MAP_WIDTH + column);
    if (map->cells[index] != value) {
        map->cells[index] = value;
        ++map->revision;
    }
}

static void set_obstacle(aurabot_map_t *map, int x_mm, int y_mm,
                         int heading_mrad)
{
    int direction = direction_index(heading_mrad);
    int column = coordinate_to_cell(x_mm, AURABOT_MAP_WIDTH);
    int row = coordinate_to_cell(y_mm, AURABOT_MAP_HEIGHT);
    if (cosine_table[direction] > 100) ++column;
    else if (cosine_table[direction] < -100) --column;
    if (sine_table[direction] > 100) ++row;
    else if (sine_table[direction] < -100) --row;
    set_cell(map, column, row, AURABOT_CELL_OBSTACLE);
}

void map_init(aurabot_map_t *map)
{
    if (map == NULL) return;
    memset(map, AURABOT_CELL_UNKNOWN, sizeof(*map));
    map->revision = 0U;
    set_cell(map, AURABOT_MAP_WIDTH / 2, AURABOT_MAP_HEIGHT / 2,
             AURABOT_CELL_VISITED);
}

void map_update_pose(aurabot_map_t *map, int x_mm, int y_mm,
                     int heading_mrad, int left_obstacle,
                     int right_obstacle)
{
    int column;
    int row;
    if (map == NULL) return;
    column = coordinate_to_cell(x_mm, AURABOT_MAP_WIDTH);
    row = coordinate_to_cell(y_mm, AURABOT_MAP_HEIGHT);
    set_cell(map, column, row, AURABOT_CELL_VISITED);
    if (left_obstacle)
        set_obstacle(map, x_mm, y_mm, heading_mrad + SENSOR_OFFSET_MRAD);
    if (right_obstacle)
        set_obstacle(map, x_mm, y_mm, heading_mrad - SENSOR_OFFSET_MRAD);
}

int map_copy(const aurabot_map_t *map, unsigned char *cells,
             unsigned int capacity, unsigned int *required_size)
{
    if (required_size == NULL) return AURABOT_ERR_INVALID_ARGUMENT;
    *required_size = AURABOT_MAP_CELLS;
    if (map == NULL || cells == NULL || capacity < AURABOT_MAP_CELLS)
        return AURABOT_ERR_INVALID_ARGUMENT;
    memcpy(cells, map->cells, AURABOT_MAP_CELLS);
    return AURABOT_OK;
}
