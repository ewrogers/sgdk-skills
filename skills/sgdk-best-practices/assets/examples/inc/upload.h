#ifndef BP_UPLOAD_H
#define BP_UPLOAD_H
#include <genesis.h>
/* Foreground only, after normal SDK startup. Caller owns this palette slot.
 * colors points to 16 readable, aligned words; it may be reused on return.
 * False means no transfer was accepted. Leave a redraw pending and retry. */
bool bp_queue_palette(u16 slot, const u16 colors[16]);
#endif
