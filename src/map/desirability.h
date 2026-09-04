#ifndef MAP_DESIRABILITY_H
#define MAP_DESIRABILITY_H

#include "core/buffer.h"

void map_desirability_clear(void);

void map_desirability_update(void);

int map_desirability_get(int grid_offset);

int map_desirability_get_max(int x, int y, int size);

void map_desirability_save_state(buffer *buf);

void map_desirability_load_state(buffer *buf);

#ifdef PANTHEON
#include <stdint.h>
/** Pantheon: zero-copy view of the desirability grid (GRID_SIZE * GRID_SIZE entries). */
const int8_t *map_desirability_grid_ptr(void);
#endif

#endif // MAP_DESIRABILITY_H
