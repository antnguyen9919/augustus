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

/** Number of histogram slots aug_house_blockers writes. */
#define AUG_HOUSE_BLOCKER_SLOTS 128
/**
 * Augustus adds its own reasons on top of the original game's 0..65, as translation keys far outside
 * that range. They are folded into slots of their own here so the governor can read them.
 */
#define AUG_BLOCKER_LATRINES 100
#define AUG_BLOCKER_LATRINES_EVOLVE 101
#define AUG_BLOCKER_FOURTH_FOOD 102
#define AUG_BLOCKER_FOURTH_FOOD_EVOLVE 103
#define AUG_BLOCKER_FIFTH_FOOD 104
#define AUG_BLOCKER_FIFTH_FOOD_EVOLVE 105
#define AUG_BLOCKER_FOURTH_GOOD 106
#define AUG_BLOCKER_FOURTH_GOOD_EVOLVE 107
#define AUG_BLOCKER_FIFTH_GOOD 108
#define AUG_BLOCKER_FIFTH_GOOD_EVOLVE 109
/** Number of building types aug_house_bad_neighbours writes (BUILDING_TYPE_MAX). */
#define AUG_BUILDING_TYPES 213
/** Number of house levels aug_house_levels writes (HOUSE_SMALL_TENT .. HOUSE_LUXURY_PALACE). */
#define AUG_HOUSE_LEVELS 20
/** Number of values aug_coverage writes. */
#define AUG_COVERAGE_COUNT 16

/**
 * "Why won't this house evolve" over every occupied house: out[code]++ for the engine's evolve text
 * code (0..29 devolve reasons, 30..59 evolve blockers, 60 max level, 62 desirability, 65 second wine;
 * Augustus's own reasons in slots 100..109, anything else in 127). Returns the number of houses
 * counted. Read-only:
 * the codes are computed on the side and the saved field is left as it was.
 */
AUG_EXPORT int aug_house_blockers(int32_t *out);
/**
 * Which building is dragging each house down: out[type]++ for the worst neighbour within eight
 * tiles of every occupied house whose next level is out of reach for want of desirability (the
 * houses aug_house_blockers counts under code 62). Returns how many houses had one. Read-only.
 */
AUG_EXPORT int aug_house_bad_neighbours(int32_t *out);
/** out[level]++ for every occupied house; returns the number of houses counted. */
AUG_EXPORT int aug_house_levels(int32_t *out);
/**
 * Culture coverage percentages: theater, amphitheater, arena, colosseum, hippodrome, tavern, average
 * entertainment, school, library, academy, hospital, then religion for the five gods. Returns 16.
 */
AUG_EXPORT int aug_coverage(int32_t *out);

#define AUG_FINANCE_COUNT 20
/**
 * The city's money: treasury, tax percentage, estimated tax income, estimated wages, the percentage
 * of people who pay tax, then this year so far (tax income, exports, total income; wages,
 * construction, interest, salary, sundries, tribute, levies and total expenses; net) and last
 * year's total income, total expenses and construction. Returns AUG_FINANCE_COUNT.
 */
AUG_EXPORT int aug_finance(int32_t *out);

/** Number of values aug_building_supply writes. */
#define AUG_SUPPLY_COUNT 11
/**
 * What one production or storage building holds and whether its output can move. Written for a
 * governor that has to tell "this workshop is idle" from "this workshop has nowhere to send it":
 * out[0] output resource id, out[1] production progress, out[2] houses covered, out[3] workers,
 * out[4] loads of its raw material in stock, out[5] loads of its output in stock, out[6] road
 * network id, out[7] distance from the map entry (0 = the distribution system cannot see it),
 * out[8] loads of weapons held (a warehouse counts its eight spaces, anything else its own store),
 * out[9] whether it accepts weapons, out[10] whether a warehouse would take a load of them now.
 * Returns 0 and writes nothing for an id that is not a building in use. Read-only.
 */
AUG_EXPORT int aug_building_supply(int building_id, int32_t *out);
/**
 * Occupied houses no tax collector reaches: count, the centroid (x, y) of those houses (-1 when
 * there are none), the people living in them, and the number of houses that are covered. Writes 5
 * values and returns the count of uncovered houses.
 */
AUG_EXPORT int aug_untaxed_houses(int32_t *out);

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
/**
 * Places the building at exact coordinates and charges its cost. Returns the building id, 1 for the
 * kinds that leave no building behind (gardens, plazas), or 0 if the engine refused.
 */
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
/**
 * Throws a low bridge across the water starting at (x,y), which must be the shore water tile on the
 * near bank: water, no road or building on it, and exactly three of its four neighbours water. The
 * engine measures the crossing itself, so it is the authority on where a bridge can go; with
 * measure_only nothing is built or charged. Returns the bridge length in tiles, 0 if there is no
 * crossing here. Bridge tiles carry a road, so a bridge joins the road networks on the two banks.
 */
AUG_EXPORT int aug_bridge(int x, int y, int measure_only);

/** City policy. */
AUG_EXPORT void aug_set_tax(int percentage);
AUG_EXPORT void aug_change_wages(int delta);
AUG_EXPORT void aug_set_labor_priority(int category, int priority);
/** Schedules a festival for god (0..4) of size 1 small, 2 large, 3 grand. Returns 1 on success. */
AUG_EXPORT int aug_festival(int god, int size);
/**
 * Sets the tactical layout of the legion garrisoned at fort_building_id. The one that matters for
 * defence is FORMATION_MOP_UP (6): the legion hunts and attacks any enemy within ~20 tiles of the
 * fort, refilling from its barracks, instead of holding station and letting enemies path past it.
 * Returns 1 if the fort has a live legion and the layout was set, 0 otherwise.
 */
AUG_EXPORT int aug_legion_layout(int fort_building_id, int layout);

/**
 * Whether a 3x3 reservoir placed with its top-left at (x,y) would actually hold water: either
 * natural water lies in the ring around it, or an aqueduct there already leads back to a filled
 * reservoir. A reservoir may be placed anywhere, but a dry one supplies nothing.
 */
AUG_EXPORT int aug_reservoir_would_fill(int x, int y);

/**
 * Runs an aqueduct along the engine's own route from (x0,y0) to (x1,y1) and returns its length in
 * tiles, 0 if there is no route. With measure_only nothing is built or charged. The endpoints for
 * a reservoir are its mid-side connector tiles -- (1,-1), (3,1), (1,3), (-1,1) from its top-left --
 * not the reservoir itself. An aqueduct may cross a road but never run along one.
 */
AUG_EXPORT int aug_aqueduct(int x0, int y0, int x1, int y1, int measure_only);

/**
 * Sets which walkers may pass a roadblock or gate, as a bitmask of 1 << roadblock_permission
 * (MAINTENANCE 1, PRIEST 2, MARKET 3, ENTERTAINER 4, EDUCATION 5, MEDICINE 6, TAX_COLLECTOR 7,
 * LABOR_SEEKER 8, MISSIONARY 9, WATCHMAN 10); 0xffff lets everyone through. A newly built roadblock
 * blocks every roaming walker until this is called. Destination walkers -- market buyers, cart
 * pushers, immigrants -- pass regardless. Returns 1 if the mask was set.
 */
AUG_EXPORT int aug_roadblock_permissions(int building_id, int mask);

#endif // AUG_GOVERN_H
