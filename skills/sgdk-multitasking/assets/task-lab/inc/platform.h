#ifndef MT_PLATFORM_H
#define MT_PLATFORM_H
#include <genesis.h>
#include "work.h"
#define MT_STACK_WORDS 1024
#define MT_GUARD_WORDS 32
extern MTWork mt_work;
extern u16 mt_stack[MT_STACK_WORDS];
extern const u16 mt_palette[16];
void mt_set_user_stack(void *top); /* Lab supervisor-only assembly helper, not SGDK API. */
void mt_start_worker(void);
int mt_decode(u16 variant,u8 *out,u16 capacity,void *ctx);
#endif
