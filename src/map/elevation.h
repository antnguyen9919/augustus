#ifndef MAP_ELEVATION_H
#define MAP_ELEVATION_H

#include "core/buffer.h"

int map_elevation_at(int grid_offset);

void map_elevation_set(int grid_offset, int value);

void map_elevation_clear(void);

void map_elevation_remove_cliffs(void);

void map_elevation_save_state(buffer *buf);

void map_elevation_load_state(buffer *buf);

#ifdef PANTHEON
#include <stdint.h>
/** Pantheon: zero-copy view of the elevation grid (GRID_SIZE * GRID_SIZE entries). */
const uint8_t *map_elevation_grid_ptr(void);
#endif

#endif // MAP_ELEVATION_H
