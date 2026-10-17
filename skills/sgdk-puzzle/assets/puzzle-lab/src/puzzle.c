#include "puzzle.h"
#include <string.h>
const char *const pz_rooms[3]={
    "############"
    "#..........#"
    "#...#......#"
    "#.@.$...g..#"
    "#..........#"
    "#...$...g..#"
    "#........E.#"
    "#..........#"
    "############",
    "############"
    "#......#...#"
    "#..o...#...#"
    "#..$...D.E.#"
    "#.@....#...#"
    "#..$g..#...#"
    "#......#...#"
    "#......#...#"
    "############",
    "############"
    "#.....#....#"
    "#..$..#....#"
    "#.@...~..E.#"
    "#..$..#....#"
    "#....g#....#"
    "#.....#....#"
    "#.....#....#"
    "############"
};
static s16 crate_at(const PzState *s,u16 pos)
{for(u16 i=0;i<PZ_CRATES;i++)if(s->crate[i]==pos)return (s16)i;return -1;}
static bool filled(const PzBoard *b,const PzState *s,u16 cell)
{return b->pit[cell]!=PZ_NONE && (s->filled&(1u<<b->pit[cell]));}
bool pz_gate_open(const PzBoard *b,const PzState *s)
{
    for(u16 i=0;i<PZ_CELLS;i++)if((b->terrain[i]==PZ_PLATE || b->terrain[i]==PZ_GATE) &&
        (s->player==i || crate_at(s,i)>=0))return true;
    return false;
}
bool pz_solved(const PzBoard *b,const PzState *s)
{
    for(u16 i=0;i<PZ_CELLS;i++)if(b->terrain[i]==PZ_GOAL && crate_at(s,i)<0)return false;
    return true;
}
bool pz_complete(const PzBoard *b)
{return b->terrain[b->state.player]==PZ_EXIT && pz_solved(b,&b->state);}
static bool open_cell(const PzBoard *b,const PzState *s,s16 cell,bool pushing)
{
    if(cell<0 || cell>=PZ_CELLS)return false;
    u8 t=b->terrain[cell];
    if(t==PZ_WALL || (t==PZ_GATE && !pz_gate_open(b,s)))return false;
    if(t==PZ_PIT && !filled(b,s,(u16)cell) && !pushing)return false;
    return true;
}
bool pz_valid(const PzBoard *b)
{
    const PzState *s=&b->state;
    if(s->player>=PZ_CELLS || !open_cell(b,s,s->player,false) || crate_at(s,s->player)>=0 || b->count>PZ_HISTORY || b->head>=PZ_HISTORY)return false;
    u16 pits=0;for(u16 i=0;i<PZ_CELLS;i++)if(b->terrain[i]==PZ_PIT)pits++;
    if(pits>8 || (s->filled>>pits))return false;
    for(u16 i=0;i<PZ_CRATES;i++)if(s->crate[i]!=PZ_NONE) {
        u16 cell=s->crate[i];if(cell>=PZ_CELLS || !open_cell(b,s,(s16)cell,false))return false;
        for(u16 j=0;j<i;j++)if(s->crate[j]==cell)return false;
    }
    return true;
}
bool pz_parse(PzBoard *b,const char *cells)
{
    if(!cells || strlen(cells)!=PZ_CELLS)return false;
    PzBoard next;memset(&next,0,sizeof(next));memset(next.pit,PZ_NONE,sizeof(next.pit));
    memset(next.state.crate,PZ_NONE,sizeof(next.state.crate));u16 actors=0,crates=0,pits=0,exits=0;
    for(u16 i=0;i<PZ_CELLS;i++) {
        char c=cells[i];u16 t=PZ_FLOOR;
        switch(c) {
            case '#':t=PZ_WALL;break;case 'g':t=PZ_GOAL;break;case 'o':t=PZ_PLATE;break;
            case 'D':t=PZ_GATE;break;case '~':t=PZ_PIT;if(pits==8)return false;next.pit[i]=(u8)pits++;break;
            case 'E':t=PZ_EXIT;exits++;break;
            case '@':next.state.player=(u8)i;actors++;break;
            case '$':if(crates==PZ_CRATES)return false;next.state.crate[crates++]=(u8)i;break;
            case '.':break;default:return false;
        }
        next.terrain[i]=(u8)t;
    }
    if(actors!=1 || exits!=1)return false;
    next.initial=next.state;if(!pz_valid(&next))return false;
    *b=next;return true;
}
bool pz_load(PzBoard *b,u16 room)
{if(room>=3)return false;if(!pz_parse(b,pz_rooms[room]))return false;b->room=(u8)room;return true;}
u16 pz_move(PzBoard *b,u16 direction)
{
    if(direction>3 || pz_complete(b))return PZ_BLOCKED;
    s16 dx=direction==PZ_LEFT ? -1 : direction==PZ_RIGHT ? 1 : 0;
    s16 dy=direction==PZ_UP ? -1 : direction==PZ_DOWN ? 1 : 0;
    s16 x=b->state.player%PZ_W+dx,y=b->state.player/PZ_W+dy;
    if(x<0 || x>=PZ_W || y<0 || y>=PZ_H)return PZ_BLOCKED;
    s16 target=y*PZ_W+x,index=crate_at(&b->state,(u16)target);PzState next=b->state;u16 result=PZ_WALK;
    if(!open_cell(b,&next,target,false))return PZ_BLOCKED;
    if(index>=0) {
        x+=dx;y+=dy;if(x<0 || x>=PZ_W || y<0 || y>=PZ_H)return PZ_BLOCKED;
        s16 beyond=y*PZ_W+x;
        if(!open_cell(b,&next,beyond,true) || crate_at(&next,(u16)beyond)>=0)return PZ_BLOCKED;
        if(b->terrain[beyond]==PZ_PIT && !filled(b,&next,(u16)beyond)) {
            next.filled|=(u8)(1u<<b->pit[beyond]);next.crate[index]=PZ_NONE;result=PZ_BRIDGE;
        } else {next.crate[index]=(u8)beyond;result=PZ_PUSH;}
    }
    next.player=(u8)target;if(next.moves<65535)next.moves++;
    b->history[b->head]=b->state;b->head=(b->head+1)%PZ_HISTORY;if(b->count<PZ_HISTORY)b->count++;
    b->state=next;return pz_complete(b) ? PZ_FINISH : result;
}
bool pz_undo(PzBoard *b)
{if(!b->count)return false;b->head=(b->head+PZ_HISTORY-1)%PZ_HISTORY;b->state=b->history[b->head];b->count--;return true;}
void pz_restart(PzBoard *b)
{b->state=b->initial;b->head=0;b->count=0;}
u32 pz_digest(const PzBoard *b)
{
    u32 h=2166136261u;
    h=(h^b->room)*16777619u;h=(h^b->state.player)*16777619u;
    for(u16 i=0;i<PZ_CRATES;i++)h=(h^b->state.crate[i])*16777619u;
    h=(h^b->state.filled)*16777619u;h=(h^b->state.moves)*16777619u;return h;
}
