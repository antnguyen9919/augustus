#ifndef API_AUG_API_H
#define API_AUG_API_H

/**
 * Pantheon engine API: the functions exported from the WebAssembly modules (headless and
 * viewer) and used by the native headless CLI. Plain C types only: ints, pointers, C strings.
 * Bumping AUG_API_VERSION invalidates snapshots produced by older builds.
 */

#include <stdint.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define AUG_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define AUG_EXPORT
#endif

#define AUG_API_VERSION 1

/** A game day is 50 ticks, a month 16 days, a year 12 months. */
#define AUG_TICKS_PER_DAY 50
#define AUG_TICKS_PER_MONTH (16 * AUG_TICKS_PER_DAY)

/** Room for every savegame piece aug_state_pieces can report (the engine has about a hundred). */
#define AUG_STATE_PIECES 160
#define AUG_TICKS_PER_YEAR (12 * AUG_TICKS_PER_MONTH)

// --- lifecycle ---------------------------------------------------------------------------

AUG_EXPORT int aug_api_version(void);

/**
 * Initialise the headless engine: locate the Caesar III files, load language and images.
 * @param c3_dir Directory with the Caesar III data files (the process changes into it)
 * @param pref_dir Writable directory for augustus.ini / c3.inf, with trailing slash
 * @param index_only 1 to skip all pixel data (headless), 0 to load images fully
 * @return 1 on success
 */
AUG_EXPORT int aug_init(const char *c3_dir, const char *pref_dir, int index_only);

/** Load a scenario (.map/.mapx) by file name, looked up like the game does. @return 1 on success */
AUG_EXPORT int aug_load_scenario(const char *name);

/** Load a saved game (.svx/.sav) from a path. @return 1 on success */
AUG_EXPORT int aug_load_save(const char *path);

// --- time --------------------------------------------------------------------------------

/** Advance the simulation by n ticks on the virtual clock (16 ms per tick). */
AUG_EXPORT void aug_tick(int n);

/** Ticks simulated through aug_tick since the engine started or the last aug_state_read. */
AUG_EXPORT int aug_ticks_total(void);

/** Virtual clock in milliseconds. */
AUG_EXPORT int aug_virtual_millis(void);

AUG_EXPORT int aug_time_year(void);
AUG_EXPORT int aug_time_month(void); // 0-11
AUG_EXPORT int aug_time_day(void);   // 0-15
AUG_EXPORT int aug_time_tick(void);  // 0-49

// --- quick stats (a packed stats block comes with the world layer) ------------------------

AUG_EXPORT int aug_population(void);
AUG_EXPORT int aug_treasury(void);

// --- snapshots ---------------------------------------------------------------------------

/**
 * Serialise the simulation. The result is a valid .svx image followed by a small trailer
 * (magic "AUG1", api version, virtual millis, total ticks). Free it with aug_free().
 * @param out_size Receives the size in bytes
 * @param compress 1 to deflate compressible pieces, 0 for a raw (faster, larger) image
 */
AUG_EXPORT uint8_t *aug_state_write(int *out_size, int compress);

/** Restore a snapshot written by aug_state_write (trailer optional). @return 1 on success */
AUG_EXPORT int aug_state_read(uint8_t *data, int size);

/** FNV-1a hash over the simulation pieces (presentation-only pieces excluded). */
AUG_EXPORT uint32_t aug_state_hash(void);
/**
 * The hash broken up per savegame piece, so a viewer and its headless twin that disagree can say
 * which part of the city they disagree about. Writes up to AUG_STATE_PIECES values (0 for the
 * presentation-only pieces the state hash skips) and returns how many. Changes nothing.
 */
AUG_EXPORT int aug_state_pieces(uint32_t *out);
/** The name of the piece at that index, for reporting. */
AUG_EXPORT const char *aug_piece_name(int index);

AUG_EXPORT void aug_free(void *pointer);

#endif // API_AUG_API_H
