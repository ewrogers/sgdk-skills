#ifndef STREAM_CACHE_H
#define STREAM_CACHE_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

enum { SC_SLOTS=18, SC_PAGES=24, SC_PAGE_TILES=16, SC_CHUNK_TILES=8,
       SC_TILE_BYTES=32, SC_CACHE_BASE=128, SC_EMPTY=0, SC_LOADING, SC_READY };
enum { SC_AVAILABLE, SC_WAIT, SC_FULL, SC_INVALID };

typedef struct { u16 page; u8 state,pinned; u32 touched; } ScSlot;
typedef struct {
    ScSlot slots[SC_SLOTS];
    u16 wanted[SC_SLOTS],wanted_count;
    u16 pending,completed,flight;
    u32 clock,hits,misses,evictions,read_errors,queue_errors;
    /* Owned, aligned RAM; never overwritten while flight is nonzero. */
    u16 staging[SC_CHUNK_TILES*SC_TILE_BYTES/2];
    u8 slot_for[SC_PAGES];
} StreamCache;

typedef bool (*ScRead)(void *context,u16 page,u16 offset,void *dest,u16 bytes);
typedef bool (*ScQueue)(void *context,const void *source,u16 tile,u16 tiles);

void sc_init(StreamCache *cache);
/* Validate the whole wanted set before changing pins. Missing pages are pinned
 * as they are admitted. Include old visible pages until their last VBlank. */
bool sc_pin(StreamCache *cache,const u16 *pages,u16 count);
u16 sc_require(StreamCache *cache,u16 page);
u16 sc_step(StreamCache *cache,u16 tile_budget,ScRead read,ScQueue queue,void *context);
/* Only acknowledge transfers known to have drained, never merely queued. */
void sc_complete(StreamCache *cache,bool drained);
bool sc_tile(const StreamCache *cache,u16 page,u16 within,u16 *tile);
/* Refuse cleanup while DMA still borrows staging. */
bool sc_reset(StreamCache *cache);
#endif
