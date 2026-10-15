#include <assert.h>
#include <stdio.h>

#include "map.h"

static unsigned char cell_at(const unsigned char *cells, unsigned int width,
                             int origin_x_mm, int origin_y_mm,
                             int x_mm, int y_mm)
{
    unsigned int column = (unsigned int)((x_mm - origin_x_mm) /
        AURABOT_MAP_CELL_MM);
    unsigned int row = (unsigned int)((y_mm - origin_y_mm) /
        AURABOT_MAP_CELL_MM);
    return cells[row * width + column];
}

int main(void)
{
    aurabot_map_t map;
    unsigned char cells[AURABOT_MAP_CELLS];
    unsigned int required;
    unsigned int width;
    unsigned int height;
    int origin_x_mm;
    int origin_y_mm;

    map_init(&map);
    assert(map_copy(&map, cells, sizeof(cells), &required, &width, &height,
                    &origin_x_mm, &origin_y_mm) == AURABOT_OK);
    assert(width == 21U && height == 21U && required == width * height);
    assert(cell_at(cells, width, origin_x_mm, origin_y_mm, 0, 0) ==
           AURABOT_CELL_VISITED);

    map_update_pose(&map, 0, 0, 0, 1, 0);
    assert(map_copy(&map, cells, sizeof(cells), &required, &width, &height,
                    &origin_x_mm, &origin_y_mm) == AURABOT_OK);
    assert(cell_at(cells, width, origin_x_mm, origin_y_mm, 100, 100) ==
           AURABOT_CELL_OBSTACLE);
    map_update_pose(&map, 0, 0, 0, 0, 1);
    assert(map_copy(&map, cells, sizeof(cells), &required, &width, &height,
                    &origin_x_mm, &origin_y_mm) == AURABOT_OK);
    assert(cell_at(cells, width, origin_x_mm, origin_y_mm, 100, -100) ==
           AURABOT_CELL_OBSTACLE);

    /* El rectángulo transmitido crece al acercarse a un borde activo. */
    map_update_pose(&map, 1800, 0, 0, 0, 0);
    assert(map_copy(&map, cells, sizeof(cells), &required, &width, &height,
                    &origin_x_mm, &origin_y_mm) == AURABOT_OK);
    assert(width > 21U && height == 21U);
    assert(cell_at(cells, width, origin_x_mm, origin_y_mm, 1800, 0) ==
           AURABOT_CELL_VISITED);
    puts("map tests passed");
    return 0;
}
