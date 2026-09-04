#include "api/aug_govern.h"

#include "building/building.h"
#include "building/construction.h"
#include "building/construction_building.h"
#include "building/construction_clear.h"
#include "building/construction_routed.h"
#include "building/house_evolution.h"
#include "building/industry.h"
#include "building/warehouse.h"
#include "building/properties.h"
#include "building/roadblock.h"
#include "building/storage.h"
#include "city/culture.h"
#include "city/festival.h"
#include "city/finance.h"
#include "city/labor.h"
#include "core/config.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "game/undo.h"
#include "map/bridge.h"
#include "map/building.h"
#include "map/data.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/grid.h"
#include "map/property.h"
#include "map/road_network.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "map/water_supply.h"
#include "pantheon/rules.h"
#include "translation/translation.h"
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

/** Augustus's own evolve reasons are translation keys far outside 0..65; give each one a slot. */
static int blocker_slot(int code)
{
    switch (code) {
        case TR_BUILDING_LATRINES_MISSING_DEVOLVE: return AUG_BLOCKER_LATRINES;
        case TR_BUILDING_LATRINES_MISSING_EVOLVE: return AUG_BLOCKER_LATRINES_EVOLVE;
        case TR_BUILDING_FOURTH_FOODTYPE_MISSING_DEVOLVE: return AUG_BLOCKER_FOURTH_FOOD;
        case TR_BUILDING_FOURTH_FOODTYPE_MISSING_EVOLVE: return AUG_BLOCKER_FOURTH_FOOD_EVOLVE;
        case TR_BUILDING_FIFTH_FOODTYPE_MISSING_DEVOLVE: return AUG_BLOCKER_FIFTH_FOOD;
        case TR_BUILDING_FIFTH_FOODTYPE_MISSING_EVOLVE: return AUG_BLOCKER_FIFTH_FOOD_EVOLVE;
        case TR_BUILDING_FOURTH_GOOD_MISSING_DEVOLVE: return AUG_BLOCKER_FOURTH_GOOD;
        case TR_BUILDING_FOURTH_GOOD_MISSING_EVOLVE: return AUG_BLOCKER_FOURTH_GOOD_EVOLVE;
        case TR_BUILDING_FIFTH_GOOD_MISSING_DEVOLVE: return AUG_BLOCKER_FIFTH_GOOD;
        case TR_BUILDING_FIFTH_GOOD_MISSING_EVOLVE: return AUG_BLOCKER_FIFTH_GOOD_EVOLVE;
        default: return code >= 0 && code < AUG_HOUSE_BLOCKER_SLOTS - 1 ? code : AUG_HOUSE_BLOCKER_SLOTS - 1;
    }
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
        out[blocker_slot(code)]++;
        houses++;
    }
    return houses;
}

int aug_house_bad_neighbours(int32_t *out)
{
    memset(out, 0, AUG_BUILDING_TYPES * sizeof(int32_t));
    int blamed = 0;
    for (int i = 1; i < building_count(); i++) {
        building *b = building_get(i);
        if (!occupied_house(b)) {
            continue;
        }
        // the same reading aug_house_blockers takes, kept on the side so the saved field is untouched
        int saved = b->data.house.evolve_text_id;
        building_type worst = building_house_determine_worst_desirability_building_type(b);
        building_house_determine_evolve_text(b, worst);
        int code = b->data.house.evolve_text_id;
        b->data.house.evolve_text_id = saved;
        if (code != 62 || worst <= BUILDING_NONE || worst >= AUG_BUILDING_TYPES) {
            continue;
        }
        out[worst]++;
        blamed++;
    }
    return blamed;
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

int aug_building_supply(int building_id, int32_t *out)
{
    building *b = building_get(building_id);
    if (!b || b->state != BUILDING_STATE_IN_USE) {
        return 0;
    }
    memset(out, 0, AUG_SUPPLY_COUNT * sizeof(int32_t));
    resource_supply_chain chain[RESOURCE_SUPPLY_CHAIN_MAX_SIZE];
    int raw = building_get_raw_materials_for_workshop(chain, b->type);
    out[0] = b->output_resource_id;
    out[1] = b->data.industry.progress;
    out[2] = b->houses_covered;
    out[3] = b->num_workers;
    out[4] = raw > 0 ? b->resources[chain[0].raw_material] : 0;
    out[5] = b->output_resource_id ? b->resources[b->output_resource_id] : 0;
    out[6] = b->road_network_id;
    out[7] = b->distance_from_entry;
    if (b->type == BUILDING_WAREHOUSE) {
        out[8] = building_warehouse_amount_can_get_from(b, RESOURCE_WEAPONS);
        out[10] = building_warehouse_accepts_storage(b, RESOURCE_WEAPONS, 0)
            && building_warehouse_maximum_receptible_amount(b, RESOURCE_WEAPONS) > 0;
    } else {
        out[8] = b->resources[RESOURCE_WEAPONS];
    }
    out[9] = b->accepted_goods[RESOURCE_WEAPONS] != 0;
    if (b->type == BUILDING_WAREHOUSE) {
        // What is actually in there, so "the warehouse is full of something else" is a reading and
        // not a guess: total loads across every resource, and the one holding the most of them.
        int stored = 0;
        for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            stored += building_warehouse_get_amount(b, r);
        }
        out[11] = stored;
        out[12] = building_storage_get_highest_quantity_resource(b);
    }
    return AUG_SUPPLY_COUNT;
}

int aug_finance(int32_t *out)
{
    finance_overview now;
    int estimated_tax_income = 0, estimated_wages = 0;
    city_finance_pantheon_estimates(&now, &estimated_tax_income, &estimated_wages);
    const finance_overview *last = city_finance_overview_last_year();
    memset(out, 0, AUG_FINANCE_COUNT * sizeof(int32_t));
    out[0] = city_finance_treasury();
    out[1] = city_finance_tax_percentage();
    out[2] = estimated_tax_income;
    out[3] = estimated_wages;
    out[4] = city_finance_percentage_taxed_people();
    out[5] = now.income.taxes;
    out[6] = now.income.exports;
    out[7] = now.income.total;
    out[8] = now.expenses.wages;
    out[9] = now.expenses.construction;
    out[10] = now.expenses.interest;
    out[11] = now.expenses.salary;
    out[12] = now.expenses.sundries;
    out[13] = now.expenses.tribute;
    out[14] = now.expenses.levies;
    out[15] = now.expenses.total;
    out[16] = now.net_in_out;
    out[17] = last->income.total;
    out[18] = last->expenses.total;
    out[19] = last->expenses.construction;
    return AUG_FINANCE_COUNT;
}

int aug_untaxed_houses(int32_t *out)
{
    int untaxed = 0, taxed = 0, sx = 0, sy = 0, people = 0;
    for (int i = 1; i < building_count(); i++) {
        building *b = building_get(i);
        if (!occupied_house(b)) {
            continue;
        }
        if (b->house_tax_coverage) {
            taxed++;
        } else {
            untaxed++;
            sx += b->x;
            sy += b->y;
            people += b->house_population;
        }
    }
    memset(out, 0, 5 * sizeof(int32_t));
    out[0] = untaxed;
    out[1] = untaxed ? sx / untaxed : -1;
    out[2] = untaxed ? sy / untaxed : -1;
    out[3] = people;
    out[4] = taxed;
    return untaxed;
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

/**
 * The per-type terrain mask place_building() itself uses (building/construction_building.c). This
 * check used to hardcode TERRAIN_ALL, which is stricter than the engine for every type that is
 * meant to sit on top of something: a roadblock belongs ON a road, a gatehouse and a tower on a
 * wall, and a reservoir may overlap an aqueduct. TERRAIN_ALL meant no tile could satisfy both this
 * function and place_building at once, so those types were silently unbuildable through the API.
 */
static int placement_mask(int type)
{
    if ((building_type_is_roadblock(type) && !(type == BUILDING_GRANARY || type == BUILDING_WAREHOUSE)) ||
        (config_get(CONFIG_GP_CH_WAREHOUSES_GRANARIES_OVER_ROAD_PLACEMENT) &&
        (type == BUILDING_GRANARY || type == BUILDING_WAREHOUSE))) {
        return type == BUILDING_GATEHOUSE
            ? ~TERRAIN_WALL & ~TERRAIN_ROAD & ~TERRAIN_HIGHWAY & ~TERRAIN_BUILDING
            : ~TERRAIN_ROAD & ~TERRAIN_HIGHWAY;
    }
    if (type == BUILDING_TOWER) {
        return ~TERRAIN_WALL & ~TERRAIN_BUILDING;
    }
    if (type == BUILDING_RESERVOIR || type == BUILDING_DRAGGABLE_RESERVOIR) {
        return ~TERRAIN_AQUEDUCT;
    }
    return TERRAIN_ALL;
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
    if (!map_tiles_are_clear(x, y, size, placement_mask(type), check_figure)) {
        return 0;
    }
    // A roadblock replaces a road tile, so alone among the types it needs one to already be there.
    if (type == BUILDING_ROADBLOCK && map_tiles_are_clear(x, y, size, TERRAIN_ROAD, check_figure)) {
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
    // gardens and plazas are terrain, not buildings, so they have no id to give back
    int id = (int) map_building_at(map_grid_offset(x, y));
    return id > 0 ? id : 1;
}

/**
 * A bridge is not placed through building_construction: the engine's own path runs the measurement
 * from the ghost that the player drags, and only then adds the bridge. Measure and add here in one
 * breath, because map_bridge_add consumes the static state the measurement leaves behind.
 *
 * The slice of blocked tiles is a GRID_SIZE * GRID_SIZE array of ints, far too large for the wasm
 * stack, so it lives in the data segment. Nothing hashed is touched by the measurement.
 */
static grid_slice bridge_blocked;

int aug_bridge(int x, int y, int measure_only)
{
    if (!map_grid_is_inside(x, y, 1)) {
        return 0;
    }
    if (!measure_only && city_finance_out_of_money()) {
        return 0;
    }
    int length = 0;
    int direction = 0;
    bridge_blocked.size = 0;
    int end_offset = map_bridge_calculate_length_direction(x, y, &length, &direction, &bridge_blocked);
    if (!end_offset || bridge_blocked.size > 0 || length < 2) {
        map_bridge_reset_building_length();
        return 0;
    }
    if (measure_only) {
        map_bridge_reset_building_length();
        return length;
    }
    int built = map_bridge_add(x, y, 0);
    if (built <= 1) {
        return 0;
    }
    city_finance_process_construction(built * aug_build_cost(BUILDING_LOW_BRIDGE));
    // map_bridge_add refreshes routing, but road networks are numbered separately.
    map_road_network_update();
    game_undo_disable();
    return built;
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

int aug_storage_order(int building_id, int resource, int state)
{
    if (state < 0 || state >= BUILDING_STORAGE_STATE_MAX) {
        return 0;
    }
    if (resource < -1 || resource >= RESOURCE_MAX) {
        return 0;
    }
    if (building_id <= 0 || building_id >= building_count()) {
        return 0;
    }
    building *b = building_get(building_id);
    if (b->state != BUILDING_STATE_IN_USE && b->state != BUILDING_STATE_CREATED) {
        return 0;
    }
    if (!b->storage_id) {
        return 0;
    }
    const building_storage *current = building_storage_get(b->storage_id);
    if (!current) {
        return 0;
    }
    // Read-modify-write the whole order table: there is no per-resource setter that takes a state,
    // only a cycler. Permissions and empty_all are left exactly as they were.
    building_storage new_data = *current;
    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (resource != -1 && r != resource) {
            continue;
        }
        new_data.resource_state[r].state = state;
        if (state != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
            new_data.resource_state[r].quantity = BUILDING_STORAGE_QUANTITY_MAX;
        }
    }
    building_storage_set_data(b->storage_id, new_data);
    return 1;
}

int aug_legion_layout(int fort_building_id, int layout)
{
    if (layout < 0 || layout >= FORMATION_MAX) {
        return 0;
    }
    if (fort_building_id <= 0 || fort_building_id >= building_count()) {
        return 0;
    }
    building *fort = building_get(fort_building_id);
    // A fort placed this same month is still BUILDING_STATE_CREATED; its legion is created at
    // placement, so accept CREATED as well and let the formation guard below reject anything else.
    if (fort->state != BUILDING_STATE_IN_USE && fort->state != BUILDING_STATE_CREATED) {
        return 0;
    }
    formation *m = formation_get(fort->formation_id);
    if (!m->in_use || !m->is_legion) {
        return 0;
    }
    formation_legion_change_layout(m, layout);
    return 1;
}

int aug_reservoir_would_fill(int x, int y)
{
    if (!map_grid_is_inside(x, y, 3)) {
        return 0;
    }
    // The two ways map_water_supply_update_reservoir_fountain() fills a reservoir: natural water in
    // the ring around its 3x3, or an aqueduct running back to one that is already filled.
    if (map_terrain_exists_tile_in_area_with_type(x - 1, y - 1, 5, TERRAIN_WATER)) {
        return 1;
    }
    return map_water_supply_has_aqueduct_access(map_grid_offset(x, y)) ? 1 : 0;
}

int aug_aqueduct(int x0, int y0, int x1, int y1, int measure_only)
{
    if (!map_grid_is_inside(x0, y0, 1) || !map_grid_is_inside(x1, y1, 1)) {
        return 0;
    }
    if (!measure_only && city_finance_out_of_money()) {
        return 0;
    }
    if (!take_undo_backups(BUILDING_AQUEDUCT)) {
        return 0;
    }
    // Three ways this differs from the road helper: it reports success as 1/0 and hands the tile
    // count back as a cost; it writes terrain even when only measuring, so the undo backups above
    // are what make a measurement free; and it leaves the aqueduct tiles and land routing for the
    // caller to refresh, which the engine's own drag does at its call site.
    int cost = 0;
    int placed = building_construction_place_aqueduct(measure_only, x0, y0, x1, y1, &cost);
    int per_tile = aug_build_cost(BUILDING_AQUEDUCT);
    int tiles = placed && per_tile > 0 ? cost / per_tile : 0;
    if (measure_only || tiles <= 0) {
        game_undo_restore_map(0);
    } else {
        city_finance_process_construction(cost);
        map_tiles_update_all_aqueducts(0);
        map_routing_update_land();
    }
    game_undo_disable();
    return tiles;
}

int aug_roadblock_permissions(int building_id, int mask)
{
    if (building_id <= 0 || building_id >= building_count()) {
        return 0;
    }
    building *b = building_get(building_id);
    if (b->state != BUILDING_STATE_IN_USE && b->state != BUILDING_STATE_CREATED) {
        return 0;
    }
    // 1 is a plain roadblock or gate; granaries, warehouses and bridges also carry permissions but
    // are not something the governor should be reconfiguring by accident.
    if (building_type_is_roadblock(b->type) != 1) {
        return 0;
    }
    // Assign rather than toggle: building_roadblock_set_permission XORs a single bit at a time.
    b->data.roadblock.exceptions = mask & ROADBLOCK_PERMISSION_ALL;
    return 1;
}
