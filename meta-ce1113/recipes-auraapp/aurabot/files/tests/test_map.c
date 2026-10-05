#include <assert.h>
#include <stdio.h>

#include "map.h"

int main(void)
{
    aurabot_map_t map;
    unsigned int center = (AURABOT_MAP_HEIGHT / 2U) * AURABOT_MAP_WIDTH +
                          AURABOT_MAP_WIDTH / 2U;
    map_init(&map);
    assert(map.cells[center] == AURABOT_CELL_VISITED);
    map_update_pose(&map, 0, 0, 0, 1, 0);
    assert(map.cells[center + AURABOT_MAP_WIDTH + 1U] ==
           AURABOT_CELL_OBSTACLE);
    map_update_pose(&map, 0, 0, 0, 0, 1);
    assert(map.cells[center - AURABOT_MAP_WIDTH + 1U] ==
           AURABOT_CELL_OBSTACLE);
    puts("map tests passed");
    return 0;
}
