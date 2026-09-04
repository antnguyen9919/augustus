#ifndef GAME_FILE_IO_H
#define GAME_FILE_IO_H

#include "core/buffer.h"
#include "scenario/data.h"

typedef enum {
    SAVEGAME_FROM_CUSTOM_SCENARIO = 0,
    SAVEGAME_FROM_ORIGINAL_CAMPAIGN = 1,
    SAVEGAME_FROM_CUSTOM_CAMPAIGN = 2
} saved_game_origin;

typedef struct {
    struct {
        int mission;
        char scenario_name[MAX_SCENARIO_NAME];
        char campaign_name[FILE_NAME_MAX];
        saved_game_origin type;
    } origin;
    int treasury;
    int population;
    int month;
    int year;
    uint8_t description[MAX_BRIEF_DESCRIPTION];
    int image_id;
    int start_year;
    int climate;
    int map_size;
    int total_invasions;
    int player_rank;
    int is_open_play;
    int open_play_id;
    scenario_win_criteria win_criteria;
} saved_game_info;

int game_file_io_read_scenario(const char *filename);

int game_file_io_read_scenario_from_buffer(buffer *buf);

int game_file_io_read_scenario_info(const char *filename, saved_game_info *info);

int game_file_io_read_scenario_info_from_buffer(buffer *buf, saved_game_info *info);

int game_file_io_write_scenario(const char *filename);

int game_file_io_read_saved_game(const char *filename, int offset);

int game_file_io_read_save_game_from_buffer(buffer *buf);

int game_file_io_read_saved_game_info(const char *filename, int offset, saved_game_info *info);

int game_file_io_read_saved_game_info_from_buffer(buffer *buf, saved_game_info *info);

int game_file_io_write_saved_game(const char *filename);

int game_file_io_delete_saved_game(const char *filename);

#ifdef PANTHEON
/**
 * Pantheon: serialize the current game into memory using the .svx file format.
 * @param out_data Receives a malloc'ed buffer, to be released with free()
 * @param out_size Receives the size of the buffer
 * @param compress Whether to deflate the compressible pieces (0 gives a byte-stable dump)
 * @return 1 on success
 */
int game_file_io_write_saved_game_to_memory(uint8_t **out_data, int *out_size, int compress);

/**
 * Pantheon: FNV-1a hash of the simulation state (every savegame piece except camera,
 * sounds, sprites, names and other presentation-only pieces).
 */
uint32_t game_file_io_state_hash(void);

/**
 * Pantheon: one FNV-1a hash per savegame piece, in the order game_file_io_state_hash walks them;
 * presentation-only pieces and empty ones get 0. Returns how many were written.
 */
int game_file_io_state_hash_pieces(uint32_t *out, int capacity);

/** Pantheon: the name of one savegame piece by index, for reporting which one diverged. */
const char *game_file_io_piece_name(int index);

/** Pantheon: print one line per savegame piece (index, size, hash) to stdout, for debugging. */
void game_file_io_state_hash_dump(void);

/** Pantheon: write every savegame piece as a raw file into an existing directory, for debugging. */
void game_file_io_dump_pieces(const char *directory);
#endif

#endif // GAME_FILE_IO_H
