#ifndef SAVE_LAB_STORAGE_H
#define SAVE_LAB_STORAGE_H

#include "save.h"

SaveResult storage_load(SaveState *state, u8 *slot);
SaveResult storage_store(const SaveState *state);
SaveResult storage_erase(void);
/* Lab-only fault injection: corrupt the newest slot's payload, not both slots. */
SaveResult storage_damage_newest(void);

#endif
