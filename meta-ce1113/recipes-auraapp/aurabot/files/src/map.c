#include <string.h>

#include "angle_math.h"
#include "aurabot_config.h"
#include "map.h"

static int coordinate_to_cell(int millimeters, int limit)
{
    int cell = millimeters / AURABOT_MAP_CELL_MM + limit / 2;
    if (millimeters < 0 && millimeters % AURABOT_MAP_CELL_MM != 0) --cell;
    return cell;
}

static int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void expand_bounds(aurabot_map_t *map, int column, int row)
{
    int min_column = clamp(column - AURABOT_MAP_GROWTH_MARGIN, 0,
                           AURABOT_MAP_MAX_WIDTH - 1);
    int max_column = clamp(column + AURABOT_MAP_GROWTH_MARGIN, 0,
                           AURABOT_MAP_MAX_WIDTH - 1);
    int min_row = clamp(row - AURABOT_MAP_GROWTH_MARGIN, 0,
                        AURABOT_MAP_MAX_HEIGHT - 1);
    int max_row = clamp(row + AURABOT_MAP_GROWTH_MARGIN, 0,
                        AURABOT_MAP_MAX_HEIGHT - 1);
    int changed = 0;

    if (min_column < map->min_column) {
        map->min_column = min_column;
        changed = 1;
    }
    if (max_column > map->max_column) {
        map->max_column = max_column;
        changed = 1;
    }
    if (min_row < map->min_row) {
        map->min_row = min_row;
        changed = 1;
    }
    if (max_row > map->max_row) {
        map->max_row = max_row;
        changed = 1;
    }
    if (changed) ++map->revision;
}

static void set_cell(aurabot_map_t *map, int column, int row,
                     unsigned char value)
{
    unsigned int index;
    if (column < 0 || column >= AURABOT_MAP_MAX_WIDTH ||
        row < 0 || row >= AURABOT_MAP_MAX_HEIGHT)
        return;
    expand_bounds(map, column, row);
    index = (unsigned int)(row * AURABOT_MAP_MAX_WIDTH + column);
    if (map->cells[index] != value) {
        map->cells[index] = value;
        ++map->revision;
    }
}

static void set_obstacle(aurabot_map_t *map, int x_mm, int y_mm,
                         int heading_mrad)
{
    int column = coordinate_to_cell(x_mm, AURABOT_MAP_MAX_WIDTH);
    int row = coordinate_to_cell(y_mm, AURABOT_MAP_MAX_HEIGHT);
    int cosine = angle_cosine_scaled(heading_mrad);
    int sine = angle_sine_scaled(heading_mrad);
    if (cosine > 100) ++column;
    else if (cosine < -100) --column;
    if (sine > 100) ++row;
    else if (sine < -100) --row;
    set_cell(map, column, row, AURABOT_CELL_OBSTACLE);
}

void map_init(aurabot_map_t *map)
{
    int center_column;
    int center_row;
    if (map == NULL) return;
    memset(map->cells, AURABOT_CELL_UNKNOWN, sizeof(map->cells));
    map->revision = 0U;
    center_column = AURABOT_MAP_MAX_WIDTH / 2;
    center_row = AURABOT_MAP_MAX_HEIGHT / 2;
    map->min_column = center_column - AURABOT_MAP_INITIAL_RADIUS;
    map->max_column = center_column + AURABOT_MAP_INITIAL_RADIUS;
    map->min_row = center_row - AURABOT_MAP_INITIAL_RADIUS;
    map->max_row = center_row + AURABOT_MAP_INITIAL_RADIUS;
    set_cell(map, center_column, center_row, AURABOT_CELL_VISITED);
}

void map_update_pose(aurabot_map_t *map, int x_mm, int y_mm,
                     int heading_mrad, int left_obstacle,
                     int right_obstacle)
{
    int column;
    int row;
    if (map == NULL) return;
    column = coordinate_to_cell(x_mm, AURABOT_MAP_MAX_WIDTH);
    row = coordinate_to_cell(y_mm, AURABOT_MAP_MAX_HEIGHT);
    set_cell(map, column, row, AURABOT_CELL_VISITED);
    if (left_obstacle)
        set_obstacle(map, x_mm, y_mm,
                     heading_mrad + AURABOT_SENSOR_OFFSET_MRAD);
    if (right_obstacle)
        set_obstacle(map, x_mm, y_mm,
                     heading_mrad - AURABOT_SENSOR_OFFSET_MRAD);
}

int map_copy(const aurabot_map_t *map, unsigned char *cells,
             unsigned int capacity, unsigned int *required_size,
             unsigned int *width, unsigned int *height,
             int *origin_x_mm, int *origin_y_mm)
{
    unsigned int output_width;
    unsigned int output_height;
    unsigned int output_size;
    unsigned int output_index = 0U;

    if (map == NULL || required_size == NULL || width == NULL ||
        height == NULL || origin_x_mm == NULL || origin_y_mm == NULL)
        return AURABOT_ERR_INVALID_ARGUMENT;
    output_width = (unsigned int)(map->max_column - map->min_column + 1);
    output_height = (unsigned int)(map->max_row - map->min_row + 1);
    output_size = output_width * output_height;
    *required_size = output_size;
    *width = output_width;
    *height = output_height;
    *origin_x_mm = (map->min_column - AURABOT_MAP_MAX_WIDTH / 2) *
        AURABOT_MAP_CELL_MM;
    *origin_y_mm = (map->min_row - AURABOT_MAP_MAX_HEIGHT / 2) *
        AURABOT_MAP_CELL_MM;
    if (cells == NULL || capacity < output_size)
        return AURABOT_ERR_INVALID_ARGUMENT;

    for (int row = map->min_row; row <= map->max_row; ++row) {
        for (int column = map->min_column;
             column <= map->max_column; ++column) {
            cells[output_index++] = map->cells[
                (unsigned int)(row * AURABOT_MAP_MAX_WIDTH + column)];
        }
    }
    return AURABOT_OK;
}
