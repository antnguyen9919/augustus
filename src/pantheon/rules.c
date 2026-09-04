#include "pantheon/rules.h"

#define RULES_MAX_GODS 8

static int god_manual[RULES_MAX_GODS]; // 0 = autonomous (stock behaviour)
static int popups_disabled;

int pantheon_god_autonomous(int god_id)
{
    if (god_id < 0 || god_id >= RULES_MAX_GODS) {
        return 1;
    }
    return !god_manual[god_id];
}

void pantheon_set_god_autonomous(int god_id, int autonomous)
{
    if (god_id >= 0 && god_id < RULES_MAX_GODS) {
        god_manual[god_id] = !autonomous;
    }
}

int pantheon_popups_enabled(void)
{
    return !popups_disabled;
}

void pantheon_set_popups_enabled(int enabled)
{
    popups_disabled = !enabled;
}

static int api_construction;

int pantheon_api_construction(void)
{
    return api_construction;
}

void pantheon_set_api_construction(int active)
{
    api_construction = active ? 1 : 0;
}
