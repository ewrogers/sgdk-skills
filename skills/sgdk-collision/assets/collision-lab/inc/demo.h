#ifndef COLLISION_DEMO_H
#define COLLISION_DEMO_H
#include "collision.h"
enum {CD_LEFT=1,CD_RIGHT=2,CD_UP=4,CD_DOWN=8};
typedef struct {
    CcBox player;
    CcVec aim,shot[2];
    CcHit ray;
    CcMove move;
    u16 page,phase,tick,shots,hits,misses,flash[2];
    s16 target_x,target_dx;
    bool fast,moving,alive[2];
} CcDemo;
extern const CcBox cd_walls[8];
void cd_init(CcDemo *d,u16 page);
void cd_fire(CcDemo *d);
void cd_tick(CcDemo *d,u16 held);
u16 cd_frame(CcDemo *d,u16 held,u16 hz,bool paused);
#endif
