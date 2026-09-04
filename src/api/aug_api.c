#include "api/aug_api.h"

#include "city/finance.h"
#include "city/population.h"
#include "core/image.h"
#include "core/log.h"
#include "core/time.h"
#include "game/file.h"
#include "game/file_io.h"
#include "game/game.h"
#include "game/system.h"
#include "game/tick.h"
#include "map/road_network.h"
#include "game/time.h"
#include "platform/file_manager.h"
#include "platform/log.h"
#include "platform/screen.h"

#ifdef PANTHEON_HEADLESS
#include "platform/headless/headless.h"
#endif

#include <stdlib.h>
#include <string.h>

#define MILLIS_PER_TICK 16

#define TRAILER_MAGIC "AUG1"
#define TRAILER_SIZE 24

static struct {
    int initialised;
    uint64_t virtual_millis;
    int ticks_total;
} state;

int aug_api_version(void)
{
    return AUG_API_VERSION;
}

static void set_clock(uint64_t millis)
{
    state.virtual_millis = millis;
#ifdef PANTHEON_HEADLESS
    headless_set_ticks(millis);
#endif
    time_set_millis((time_millis) millis);
}

int aug_init(const char *c3_dir, const char *pref_dir, int index_only)
{
    if (state.initialised) {
        return 1;
    }
#ifdef PANTHEON_HEADLESS
    if (pref_dir) {
        headless_set_pref_path(pref_dir);
    }
#endif
    platform_log_setup();
    log_info("Pantheon engine, Augustus", system_version(), 0);
    if (!platform_file_manager_set_base_path(c3_dir)) {
        log_error("Cannot use Caesar III directory", c3_dir, 0);
        return 0;
    }
    if (!game_pre_init()) {
        log_error("game_pre_init failed: not a Caesar III directory?", c3_dir, 0);
        return 0;
    }
    if (!platform_screen_create("Pantheon", 100, 0)) {
        log_error("platform_screen_create failed", 0, 0);
        return 0;
    }
    set_clock(0);
    image_set_index_only(index_only);
    if (!game_init()) {
        log_error("game_init failed", 0, 0);
        return 0;
    }
    game_file_set_disk_saves_enabled(0);
    state.initialised = 1;
    return 1;
}

int aug_load_scenario(const char *name)
{
    int ok = game_file_start_scenario_by_name((const uint8_t *) name);
    if (ok) {
        state.ticks_total = 0;
        map_road_network_update(); // stock only recomputes this on the daily tick; sensors want it at once
    }
    return ok;
}

int aug_load_save(const char *path)
{
    int ok = game_file_load_saved_game(path) == 1;
    if (ok) {
        state.ticks_total = 0;
    }
    return ok;
}

void aug_tick(int n)
{
    for (int i = 0; i < n; i++) {
        set_clock(state.virtual_millis + MILLIS_PER_TICK);
        game_tick_run();
        state.ticks_total++;
    }
}

int aug_ticks_total(void)
{
    return state.ticks_total;
}

int aug_virtual_millis(void)
{
    return (int) state.virtual_millis;
}

int aug_time_year(void) { return game_time_year(); }
int aug_time_month(void) { return game_time_month(); }
int aug_time_day(void) { return game_time_day(); }
int aug_time_tick(void) { return game_time_tick(); }

int aug_population(void) { return city_population(); }
int aug_treasury(void) { return city_finance_treasury(); }

static void write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = value & 0xff;
    dst[1] = (value >> 8) & 0xff;
    dst[2] = (value >> 16) & 0xff;
    dst[3] = (value >> 24) & 0xff;
}

static uint32_t read_u32(const uint8_t *src)
{
    return src[0] | (src[1] << 8) | (src[2] << 16) | ((uint32_t) src[3] << 24);
}

uint8_t *aug_state_write(int *out_size, int compress)
{
    uint8_t *data = 0;
    int size = 0;
    if (!game_file_io_write_saved_game_to_memory(&data, &size, compress)) {
        return 0;
    }
    uint8_t *with_trailer = realloc(data, size + TRAILER_SIZE);
    if (!with_trailer) {
        free(data);
        return 0;
    }
    uint8_t *trailer = with_trailer + size;
    memcpy(trailer, TRAILER_MAGIC, 4);
    write_u32(trailer + 4, AUG_API_VERSION);
    write_u32(trailer + 8, (uint32_t) state.virtual_millis);
    write_u32(trailer + 12, (uint32_t) (state.virtual_millis >> 32));
    write_u32(trailer + 16, (uint32_t) state.ticks_total);
    write_u32(trailer + 20, 0);
    *out_size = size + TRAILER_SIZE;
    return with_trailer;
}

int aug_state_read(uint8_t *data, int size)
{
    int has_trailer = size >= TRAILER_SIZE && memcmp(data + size - TRAILER_SIZE, TRAILER_MAGIC, 4) == 0;
    int payload = has_trailer ? size - TRAILER_SIZE : size;
    if (game_file_load_saved_game_from_memory(data, payload) != 1) {
        return 0;
    }
    if (has_trailer) {
        const uint8_t *trailer = data + payload;
        uint64_t millis = read_u32(trailer + 8) | ((uint64_t) read_u32(trailer + 12) << 32);
        set_clock(millis);
        state.ticks_total = (int) read_u32(trailer + 16);
    } else {
        state.ticks_total = 0;
    }
    return 1;
}

uint32_t aug_state_hash(void)
{
    return game_file_io_state_hash();
}

int aug_state_pieces(uint32_t *out)
{
    return game_file_io_state_hash_pieces(out, AUG_STATE_PIECES);
}

const char *aug_piece_name(int index)
{
    return game_file_io_piece_name(index);
}

void aug_free(void *pointer)
{
    free(pointer);
}
