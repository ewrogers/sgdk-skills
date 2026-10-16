#ifndef COLLISION_LAB_H
#define COLLISION_LAB_H
#ifdef CC_HOST
#include <stdint.h>
#include <stdbool.h>
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
#else
#include <genesis.h>
#endif

/* Local Q4 coordinates, NOT SGDK fix16/fix32. 16 units = one pixel. */
#define CC_PIXEL 16
#define CC_TIME 4096
#define CC_LIMIT 16384
#define CC_EXTENT 4096
#define CC_MOTION 4096
#define CC_MAX_BOXES 32
#define CC_SLIDES 3
enum {CC_MISS,CC_HIT,CC_OVERLAP,CC_INVALID,CC_EXHAUSTED};
typedef struct {s16 x,y;} CcVec;
typedef struct {s16 x,y,w,h;} CcBox;
typedef struct {u16 kind,time,index;s16 nx,ny;s32 num,den;} CcHit;
typedef struct {u16 visited,narrow;} CcStats;
typedef struct {CcBox box;u16 kind,contacts;CcVec remaining;CcStats stats;} CcMove;

bool cc_overlap(CcBox a,CcBox b);
CcHit cc_sweep(CcBox a,CcVec da,CcBox b,CcVec db);
CcHit cc_ray(CcVec start,CcVec delta,CcBox target);
CcHit cc_cast(CcBox a,CcVec delta,const CcBox *walls,u16 count,CcStats *stats);
CcMove cc_slide(CcBox a,CcVec delta,const CcBox *walls,u16 count);
#endif
