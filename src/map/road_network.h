#ifndef MAP_ROAD_NETWORK_H
#define MAP_ROAD_NETWORK_H

void map_road_network_clear(void);

int map_road_network_get(int grid_offset);

void map_road_network_update(void);

#ifdef PANTHEON
#include <stdint.h>
/** Pantheon: zero-copy view of the road-network-id grid (GRID_SIZE * GRID_SIZE entries). */
const uint8_t *map_road_network_grid_ptr(void);
#endif

#endif // MAP_ROAD_NETWORK_H
