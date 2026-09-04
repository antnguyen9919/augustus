#ifndef PANTHEON_RULES_H
#define PANTHEON_RULES_H

/**
 * Runtime switches the world layer sets on an engine instance. They are not part of the
 * savegame: the world layer re-applies them after every load or snapshot restore.
 */

/** 1 (default) if the engine's own mood logic may bless/curse for this god. */
int pantheon_god_autonomous(int god_id);
void pantheon_set_god_autonomous(int god_id, int autonomous);

/** 0 turns every popup message into a plain list entry (observe mode). Default 1. */
int pantheon_popups_enabled(void);
void pantheon_set_popups_enabled(int enabled);

/** 1 while a governor API call is constructing; lets the observe lock stay on for the UI. */
int pantheon_api_construction(void);
void pantheon_set_api_construction(int active);

#endif // PANTHEON_RULES_H
