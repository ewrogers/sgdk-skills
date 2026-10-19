#ifndef HARDWARE_LAB_READINESS_H
#define HARDWARE_LAB_READINESS_H
#include <genesis.h>
#include "everdrive_link.h"

/* Uses the existing Lab VInt clock; it does not introduce a second scheduler. */
void hl_ready_open(bool hard_reset, u16 entry_data, u16 entry_bss, bool pal, const HlEdLink *link);
void hl_ready_update(u32 vints, u32 logic, u32 missed, const HlEdLink *link);

#endif
