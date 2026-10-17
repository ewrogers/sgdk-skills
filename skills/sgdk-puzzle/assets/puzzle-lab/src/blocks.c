#include "blocks.h"
#include <string.h>
/* Four-by-four footprints, low bit at the upper left. These are Lab rules,
 * not an implementation of a commercial game's rotation specification. */
u16 bl_shape(u16 piece,u16 rotation)
{
    const u16 bases[7]={0x00f0,0x0066,0x0072,0x0036,0x0063,0x0071,0x0074};
    if(piece>=7 || rotation>=4)return 0;
    u16 mask=bases[piece];if(piece==1)return mask;
    for(u16 r=0;r<rotation;r++) {
        u16 turned=0;
        for(u16 y=0;y<4;y++)for(u16 x=0;x<4;x++)if(mask&(1u<<(y*4+x)))turned|=(u16)(1u<<(x*4+3-y));
        mask=turned;
    }
    return mask;
}
static u8 draw(BlGame *g)
{
    if(g->bag_pos==7) {
        for(u16 i=0;i<7;i++)g->bag[i]=(u8)i;
        for(u16 i=6;i>0;i--) {
            g->rng=g->rng*1664525u+1013904223u;u16 j=(u16)((g->rng>>16)%(i+1));
            u8 v=g->bag[i];g->bag[i]=g->bag[j];g->bag[j]=v;
        }
        g->bag_pos=0;
    }
    return g->bag[g->bag_pos++];
}
bool bl_fits(const BlGame *g,s16 px,s16 py,u16 rotation)
{
    u16 mask=bl_shape(g->piece,rotation);if(!mask)return false;
    for(s16 y=0;y<4;y++)for(s16 x=0;x<4;x++)if(mask&(1u<<(y*4+x))) {
        s16 xx=px+x,yy=py+y;
        if(xx<0 || xx>=BL_W || yy<0 || yy>=BL_H || g->cells[yy*BL_W+xx])return false;
    }
    return true;
}
static void spawn(BlGame *g)
{g->piece=g->next;g->next=draw(g);g->x=3;g->y=0;g->rotation=0;g->gravity=0;g->lock=0;g->over=!bl_fits(g,g->x,g->y,0);}
void bl_init(BlGame *g,u32 seed)
{memset(g,0,sizeof(*g));g->rng=seed;g->bag_pos=7;g->next=draw(g);spawn(g);}
bool bl_shift(BlGame *g,s16 dx,s16 dy)
{
    if(g->over || dx<-1 || dx>1 || dy<0 || dy>1 || (!dx && !dy))return false;
    if(!bl_fits(g,g->x+dx,g->y+dy,g->rotation))return false;
    g->x+=dx;g->y+=dy;return true;
}
bool bl_rotate(BlGame *g)
{
    if(g->over)return false;
    const s16 offsets[][2]={{0,0},{-1,0},{1,0},{-2,0},{2,0},{0,-1}};
    u16 rotation=(g->rotation+1)&3;
    for(u16 i=0;i<6;i++)if(bl_fits(g,g->x+offsets[i][0],g->y+offsets[i][1],rotation)) {
        g->x+=offsets[i][0];g->y+=offsets[i][1];g->rotation=(u8)rotation;return true;
    }
    return false;
}
u16 bl_clear(BlGame *g)
{
    s16 dest=BL_H-1;u16 cleared=0;
    for(s16 src=BL_H-1;src>=0;src--) {
        bool full=true;for(u16 x=0;x<BL_W;x++)if(!g->cells[src*BL_W+x])full=false;
        if(full){cleared++;continue;}
        for(u16 x=0;x<BL_W;x++)g->cells[dest*BL_W+x]=g->cells[src*BL_W+x];
        dest--;
    }
    while(dest>=0){for(u16 x=0;x<BL_W;x++)g->cells[dest*BL_W+x]=0;dest--;}
    return cleared;
}
static u16 lock_piece(BlGame *g)
{
    u16 mask=bl_shape(g->piece,g->rotation);
    for(u16 y=0;y<4;y++)for(u16 x=0;x<4;x++)if(mask&(1u<<(y*4+x)))g->cells[(g->y+y)*BL_W+g->x+x]=g->piece+1;
    u16 rows=bl_clear(g);const u16 awards[5]={0,100,300,500,800};
    g->lines+=rows;if(g->lines>9999)g->lines=9999;
    g->score+=awards[rows>4 ? 4 : rows];if(g->score>999999)g->score=999999;
    if(rows)g->flash=45;
    spawn(g);return 1|(rows ? 2 : 0)|(g->over ? 4 : 0);
}
u16 bl_drop(BlGame *g)
{
    if(g->over)return 0;
    while(bl_shift(g,0,1)){}return lock_piece(g);
}
u16 bl_tick(BlGame *g,u16 held)
{
    u16 previous=g->previous,pressed=held&~previous;g->previous=held;
    if(g->over)return 0;
    if(g->flash)g->flash--;
    if(pressed&BL_ROTATE)bl_rotate(g);
    if(pressed&BL_DROP)return bl_drop(g);
    u16 horizontal=held&(BL_LEFT|BL_RIGHT),prior=previous&(BL_LEFT|BL_RIGHT);
    if(horizontal!=prior || !horizontal)g->repeat=0;
    if(horizontal && horizontal!=(BL_LEFT|BL_RIGHT)) {
        if(!g->repeat){bl_shift(g,horizontal==BL_LEFT ? -1 : 1,0);g->repeat=horizontal!=prior ? 15 : 5;}
        else g->repeat--;
    }
    g->gravity++;
    u16 interval=(held&BL_SOFT) ? 3 : 30;
    if(g->gravity>=interval){g->gravity=0;bl_shift(g,0,1);}
    if(!bl_fits(g,g->x,g->y+1,g->rotation)) {
        if(++g->lock>=30)return lock_piece(g);
    } else g->lock=0;
    return 0;
}
u16 bl_clock(u16 *phase,u16 hz)
{
    if(hz!=50 && hz!=60)return 0;
    *phase+=60;u16 count=*phase/hz;*phase%=hz;return count;
}
bool bl_valid(const BlGame *g)
{
    if(g->piece>=7 || g->next>=7 || g->rotation>=4 || g->bag_pos>7 || g->score>999999 || g->lines>9999)return false;
    u16 mask=0;
    for(u16 i=0;i<7;i++){if(g->bag[i]>=7)return false;mask|=1u<<g->bag[i];}
    if(mask!=127)return false;
    for(u16 i=0;i<BL_W*BL_H;i++)if(g->cells[i]>7)return false;
    return g->over || bl_fits(g,g->x,g->y,g->rotation);
}
u32 bl_digest(const BlGame *g)
{
    u32 h=2166136261u;
#define MIX(v) h=(h^(u32)(v))*16777619u
    for(u16 i=0;i<BL_W*BL_H;i++){MIX(g->cells[i]);}
    for(u16 i=0;i<7;i++){MIX(g->bag[i]);}
    MIX(g->bag_pos);MIX(g->piece);MIX(g->next);MIX(g->rotation);MIX(g->x);MIX(g->y);
    MIX(g->gravity);MIX(g->lock);MIX(g->repeat);MIX(g->previous);MIX(g->rng);MIX(g->score);MIX(g->lines);MIX(g->over);
#undef MIX
    return h;
}
