#include "api/aug_govern.h"

#include "building/building.h"
#include "building/construction.h"
#include "building/construction_building.h"
#include "building/construction_clear.h"
#include "building/construction_routed.h"
#include "building/house_evolution.h"
#include "building/properties.h"
#include "city/culture.h"
#include "city/festival.h"
#include "city/finance.h"
#include "city/labor.h"
#include "figure/formation.h"
#include "game/undo.h"
#include "map/building.h"
#include "map/data.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/grid.h"
#include "map/property.h"
#include "map/road_network.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "pantheon/rules.h"
#include "scenario/map.h"

#include <string.h>

int aug_grid_size(void)
{
    return GRID_SIZE;
}

int aug_grid_start_offset(void)
{
    return map_data.start_offset;
}

const uint32_t *aug_grid_terrain(void)
{
    return map_terrain_grid_ptr();
}

const uint32_t *aug_grid_buildings(void)
{
    return map_building_grid_ptr();
}

const int8_t *aug_grid_desirability(void)
{
    return map_desirability_grid_ptr();
}

const uint8_t *aug_grid_road_network(void)
{
    return map_road_network_grid_ptr();
}

const uint8_t *aug_grid_elevation(void)
{
    return map_elevation_grid_ptr();
}

int aug_map_points(int32_t *out)
{
    map_point entry = scenario_map_entry();
    map_point exit = scenario_map_exit();
    out[0] = entry.x;
    out[1] = entry.y;
    out[2] = exit.x;
    out[3] = exit.y;
    return 4;
}

static int valid_type(int type)
{
    return type > BUILDING_NONE && type < BUILDING_TYPE_MAX;
}

static int occupied_house(const building *b)
{
    return b->state == BUILDING_STATE_IN_USE && b->house_size && building_is_house(b->type) && b->house_population > 0;
}

int aug_house_blockers(int32_t *out)
{
    memset(out, 0, AUG_HOUSE_BLOCKER_SLOTS * sizeof(int32_t));
    int houses = 0;
    for (int i = 1; i < building_count(); i++) {
        building *b = building_get(i);
        if (!occupied_house(b)) {
            continue;
        }
        // Only the building-info window computes this field, so it is stale in headless play; compute it
        // here and put the saved value back, because the field is part of the hashed state.
        int saved = b->data.house.evolve_text_id;
        building_house_determine_evolve_text(b, building_house_determine_worst_desirability_building_type(b));
        int code = b->data.house.evolve_text_id;
        b->data.house.evolve_text_id = saved;
        out[code >= 0 && code < AUG_HOUSE_BLOCKER_SLOTS - 1 ? code : AUG_HOUSE_BLOCKER_SLOTS - 1]++;
        houses++;
    }
    return houses;
}

int aug_house_levels(int32_t *out)
{
    memset(out, 0, AUG_HOUSE_LEVELS * sizeof(int32_t));
    int houses = 0;
    for (int i = 1; i < building_count(); i++) {
        building *b = building_get(i);
        if (!occupied_house(b)) {
            continue;
        }
        int level = b->subtype.house_level;
        out[level >= 0 && level < AUG_HOUSE_LEVELS ? level : AUG_HOUSE_LEVELS - 1]++;
        houses++;
    }
    return houses;
}

int aug_coverage(int32_t *out)
{
    out[0] = city_culture_coverage_theater();
    out[1] = city_culture_coverage_amphitheater();
    out[2] = city_culture_coverage_arena();
    out[3] = city_culture_coverage_colosseum();
    out[4] = city_culture_coverage_hippodrome();
    out[5] = city_culture_coverage_tavern();
    out[6] = city_culture_coverage_average_entertainment();
    out[7] = city_culture_coverage_school();
    out[8] = city_culture_coverage_library();
    out[9] = city_culture_coverage_academy();
    out[10] = city_culture_coverage_hospital();
    for (int god = 0; god < 5; god++) {
        out[11 + god] = city_culture_coverage_religion(god);
    }
    return AUG_COVERAGE_COUNT;
}

int aug_build_cost(int type)
{
    if (!valid_type(type)) {
        return 0;
    }
    return model_get_building(type)->cost;
}

int aug_build_size(int type)
{
    if (!valid_type(type)) {
        return 0;
    }
    if (type == BUILDING_WAREHOUSE) {
        return 3;
    }
    return building_properties_for_type(type)->size;
}

/**
 * building_construction_set_type() owns the "required terrain" state that
 * building_construction_can_place_on_terrain() reads, and the viewer's observe lock ignores it
 * unless the call is flagged as coming from the API.
 */
static void begin_construction(int type)
{
    pantheon_set_api_construction(1);
    building_construction_set_type(type, 0);
}

static void end_construction(void)
{
    building_construction_clear_type();
    pantheon_set_api_construction(0);
}

int aug_can_build(int type, int x, int y)
{
    int size = aug_build_size(type);
    if (size <= 0 || !map_grid_is_inside(x, y, size)) {
        return 0;
    }
    if (city_finance_out_of_money()) {
        return 0;
    }
    int check_figure = type == BUILDING_ROADBLOCK && size == 1 ? 0 : 1;
    if (!map_tiles_are_clear(x, y, size, TERRAIN_ALL, check_figure)) {
        return 0;
    }
    begin_construction(type);
    int warning = 0;
    int ok = building_construction_can_place_on_terrain(x, y, &warning);
    end_construction();
    return ok ? 1 : 0;
}

int aug_build(int type, int x, int y)
{
    if (!aug_can_build(type, x, y)) {
        return 0;
    }
    begin_construction(type);
    int placed = building_construction_place_building(type, x, y, 1);
    end_construction();
    if (!placed) {
        return 0;
    }
    formation_move_herds_away(x, y);
    city_finance_process_construction(aug_build_cost(type));
    // No undo for API placements: the governor never takes anything back.
    game_undo_disable();
    return (int) map_building_at(map_grid_offset(x, y));
}

/**
 * The routed and clearing helpers start with game_undo_restore_map(), which copies the undo
 * backups over the live grids. Taking those backups first makes that a no-op; the backups also
 * let a dry run put the map back exactly as it was.
 */
static int take_undo_backups(int type)
{
    return game_undo_start_build(type);
}

int aug_build_road(int x0, int y0, int x1, int y1, int measure_only)
{
    if (!map_grid_is_inside(x0, y0, 1) || !map_grid_is_inside(x1, y1, 1)) {
        return 0;
    }
    if (!measure_only && city_finance_out_of_money()) {
        return 0;
    }
    if (!take_undo_backups(BUILDING_ROAD)) {
        return 0;
    }
    int tiles = building_construction_place_road(measure_only, x0, y0, x1, y1);
    if (measure_only || tiles <= 0) {
        game_undo_restore_map(0);
    } else {
        city_finance_process_construction(tiles * aug_build_cost(BUILDING_ROAD));
        map_road_network_update();
    }
    game_undo_disable();
    return tiles > 0 ? tiles : 0;
}

int aug_clear(int x0, int y0, int x1, int y1, int measure_only)
{
    if (!map_grid_is_inside(x0, y0, 1) || !map_grid_is_inside(x1, y1, 1)) {
        return 0;
    }
    if (measure_only) {
        // clear_select marks the tiles "deleting" in the property grid (the GUI's red overlay);
        // drop those bits again so a measurement leaves the state hash untouched.
        unsigned int tiles = building_construction_clear_select(x0, y0, x1, y1);
        map_property_clear_constructing_and_deleted();
        return (int) tiles;
    }
    if (city_finance_out_of_money()) {
        return 0;
    }
    if (!take_undo_backups(BUILDING_CLEAR_LAND)) {
        return 0;
    }
    unsigned int tiles = building_construction_clear_land(x0, y0, x1, y1);
    if (tiles == BUILDING_CONSTRUCTION_CLEAR_LAND_INTERRUPTED) {
        // A confirmation dialog would be needed (fort, bridge, monument): refuse, leave the map as it was.
        game_undo_restore_map(0);
        game_undo_disable();
        return 0;
    }
    city_finance_process_construction((int) tiles * aug_build_cost(BUILDING_CLEAR_LAND));
    map_road_network_update();
    game_undo_disable();
    return (int) tiles;
}

int aug_demolish(int building_id)
{
    if (building_id <= 0 || building_id >= building_count()) {
        return 0;
    }
    building *b = building_main(building_get(building_id));
    // A building placed this tick is still BUILDING_STATE_CREATED; it turns IN_USE on the next update.
    if (!b || (b->state != BUILDING_STATE_IN_USE && b->state != BUILDING_STATE_CREATED)) {
        return 0;
    }
    int size = building_properties_for_type(b->type)->size;
    if (size <= 0) {
        size = 1;
    }
    return aug_clear(b->x, b->y, b->x + size - 1, b->y + size - 1, 0) > 0 ? 1 : 0;
}

void aug_set_tax(int percentage)
{
    if (percentage < 0) {
        percentage = 0;
    } else if (percentage > 25) {
        percentage = 25;
    }
    city_finance_set_tax_percentage(percentage);
}

void aug_change_wages(int delta)
{
    city_labor_change_wages(delta);
}

void aug_set_labor_priority(int category, int priority)
{
    if (category < 0 || category >= 10) { // city_data.labor.categories[10]
        return;
    }
    if (priority < 0) {
        priority = 0;
    }
    int max = city_labor_max_selectable_priority(category);
    if (priority > max) {
        priority = max;
    }
    city_labor_set_priority(category, priority);
}

int aug_festival(int god, int size)
{
    if (god < 0 || god > 4 || size < FESTIVAL_SMALL || size > FESTIVAL_GRAND) {
        return 0;
    }
    if (city_festival_is_planned()) {
        return 0;
    }
    city_festival_select_god(god);
    if (!city_festival_select_size(size)) {
        return 0;
    }
    city_festival_schedule();
    return 1;
}
