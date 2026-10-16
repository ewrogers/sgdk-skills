#ifndef BP_PAIR_H
#define BP_PAIR_H
#include <genesis.h>
/* Foreground only. The sprite engine must already be initialized. Definitions are
 * borrowed and must outlive the pair. No sprite pointer escapes this module. */
bool bp_pair_open(const SpriteDefinition *left, const SpriteDefinition *right);
void bp_pair_close(void);
bool bp_pair_is_open(void);
#endif
