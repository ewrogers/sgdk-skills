#ifndef BP_CHECKED_H
#define BP_CHECKED_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

/* An in-memory record, deliberately not a wire-format struct. */
typedef struct {
    u32 score;
    u16 health;
    u8 flags;
} BpRecord;
u32 bp_square(u16 value);
u16 bp_add_clamped(u16 value, u16 amount);
u16 bp_elapsed(u16 now, u16 then);
bool bp_layer(u16 index, u16 *mask);
/* Exactly eight bytes: version, flags, health BE16, score BE32.
 * Invalid input leaves the destination unchanged. */
bool bp_decode(const u8 *bytes, u16 length, BpRecord *out);
bool bp_encode(const BpRecord *record, u8 *bytes, u16 capacity);
int bp_example_checks(void);
#endif
