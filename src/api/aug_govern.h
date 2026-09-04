#ifndef AUG_GOVERN_H
#define AUG_GOVERN_H

#include "api/aug_api.h"

#include <stdint.h>

/**
 * Governor API: sensors and actuators for an AI (or scripted) governor driving a city.
 * Building placement mirrors what a player can do, charges the model cost and refreshes
 * routing; nothing here bypasses the simulation's own rules.
 *
 * Grid views are zero-copy pointers into the engine's heap: GRID_SIZE * GRID_SIZE entries,
 * indexed by grid offset (offset = start_offset + x + y * GRID_SIZE). They stay valid for the
 * life of the instance; re-read them after every tick batch, never cache values across ticks.
 */

/** Grid side length (162) and the offset of tile (0,0) for the loaded map. */
AUG_EXPORT int aug_grid_size(void);
AUG_EXPORT int aug_grid_start_offset(void);
/** Terrain bit flags per tile (TERRAIN_* from map/terrain.h). */
AUG_EXPORT const uint32_t *aug_grid_terrain(void);
/** Building id per tile, 0 where empty. */
AUG_EXPORT const uint32_t *aug_grid_buildings(void);
/** Desirability per tile, -100..100. */
AUG_EXPORT const int8_t *aug_grid_desirability(void);
/** Road network id per tile, 0 where there is no road. */
AUG_EXPORT const uint8_t *aug_grid_road_network(void);
/** Elevation per tile. */
AUG_EXPORT const uint8_t *aug_grid_elevation(void);
/** Writes entry x, entry y, exit x, exit y; returns 4. */
AUG_EXPORT int aug_map_points(int32_t *out);

/** Cost of one building of this type (or of one tile for roads, walls, clearing). */
AUG_EXPORT int aug_build_cost(int type);
/** Footprint side length of this building type, 0 for unknown types. */
AUG_EXPORT int aug_build_size(int type);
/**
 * Dry run: 1 if a building of this type could be placed with its top-left tile at (x,y):
 * inside the map, tiles clear of terrain, buildings and figures, the type's terrain
 * requirement met (meadow for farms, rock for mines, ...) and the treasury not exhausted.
 * Special buildings that need more context (forts, docks, gatehouses, towers, bridges,
 * reservoirs) are not fully validated here; aug_build is authoritative.
 */
AUG_EXPORT int aug_can_build(int type, int x, int y);
/** Places the building at exact coordinates and charges its cost. Returns the building id or 0. */
AUG_EXPORT int aug_build(int type, int x, int y);
/**
 * Builds a road from (x0,y0) to (x1,y1) along the engine's own route, like dragging in the game.
 * With measure_only the map is untouched and nothing is charged. Returns the number of tiles.
 */
AUG_EXPORT int aug_build_road(int x0, int y0, int x1, int y1, int measure_only);
/** Clears the rectangle (trees, rubble, buildings). Same measure_only contract. Returns tiles cleared. */
AUG_EXPORT int aug_clear(int x0, int y0, int x1, int y1, int measure_only);
/** Demolishes one building by id (its whole footprint). Returns 1 on success. */
AUG_EXPORT int aug_demolish(int building_id);

/** City policy. */
AUG_EXPORT void aug_set_tax(int percentage);
AUG_EXPORT void aug_change_wages(int delta);
AUG_EXPORT void aug_set_labor_priority(int category, int priority);
/** Schedules a festival for god (0..4) of size 1 small, 2 large, 3 grand. Returns 1 on success. */
AUG_EXPORT int aug_festival(int god, int size);

#endif // AUG_GOVERN_H
