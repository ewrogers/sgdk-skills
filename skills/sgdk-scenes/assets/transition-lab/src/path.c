#include "level.h"

#define NAV_WIDTH 32
#define NAV_HEIGHT 15
#define NAV_CELLS (NAV_WIDTH*NAV_HEIGHT)
#define UNVISITED 0xFFFF

/* One main-loop search at a time. No heap or recursion; 2,400 bytes of workspace, including cached walkable cells. */
static u16 parent[NAV_CELLS],queue[NAV_CELLS],cached_room=UNVISITED;
static u8 walkable[NAV_CELLS];

bool level_next_step(u16 room,s16 x,s16 y,s16 target_x,s16 target_y,s16 *next_x,s16 *next_y)
{
    s16 sx=(x-16+4)/8,sy=(y-64+4)/8;
    if (room>=ROOM_COUNT || sx<0 || sx>=NAV_WIDTH || sy<0 || sy>=NAV_HEIGHT ||
        !next_x || !next_y) return false;
    if (cached_room!=room) {
        for (u16 i=0;i<NAV_CELLS;++i)
            walkable[i]=!level_solid(room,16+(i%NAV_WIDTH)*8,64+(i/NAV_WIDTH)*8,32,32);
        cached_room=room;
    }
    if (!walkable[sy*NAV_WIDTH+sx]) return false;
    s16 gx=(target_x-16+4)/8,gy=(target_y-64+4)/8;
    if (gx<0) gx=0;
    if (gx>=NAV_WIDTH) gx=NAV_WIDTH-1;
    if (gy<0) gy=0;
    if (gy>=NAV_HEIGHT) gy=NAV_HEIGHT-1;
    target_x=16+gx*8; target_y=64+gy*8;
    for (u16 i=0;i<NAV_CELLS;++i) parent[i]=UNVISITED;
    u16 start=sy*NAV_WIDTH+sx,head=0,tail=1,best=start,best_distance=0xFFFF;
    queue[0]=start; parent[start]=start;
    while (head<tail) {
        u16 cell=queue[head++];
        s16 cx=cell%NAV_WIDTH,cy=cell/NAV_WIDTH;
        s16 dx=16+cx*8-target_x,dy=64+cy*8-target_y;
        u16 distance=(dx<0 ? -dx : dx)+(dy<0 ? -dy : dy);
        if (distance<best_distance) { best=cell; best_distance=distance; }
        if (!distance) break;
        static const s16 step_x[]={-1,1,0,0},step_y[]={0,0,-1,1};
        for (u16 d=0;d<4;++d) {
            s16 nx=cx+step_x[d],ny=cy+step_y[d];
            if (nx<0 || nx>=NAV_WIDTH || ny<0 || ny>=NAV_HEIGHT) continue;
            u16 next=ny*NAV_WIDTH+nx;
            if (parent[next]!=UNVISITED || !walkable[next]) continue;
            parent[next]=cell; queue[tail++]=next;
        }
    }
    if (best==start) return false; /* Already closest, or the target is inaccessible. */
    while (parent[best]!=start) best=parent[best];
    *next_x=16+(best%NAV_WIDTH)*8; *next_y=64+(best/NAV_WIDTH)*8;
    return true;
}
