#include "world.h"

bool bomb_throw(Game *game,u16 owner,Point target)
{
    if (owner>=BOMB_COUNT || game->bombs[owner].phase!=BOMB_FREE) return false;
    Enemy *enemy=&game->enemies[owner];
    u16 cell=world_cell(target);
    if (enemy->kind!=BOMBARDIER || cell==NO_CELL || world_wall(game->level,cell%GRID_W,cell/GRID_W)) return false;
    target=world_point(cell);
    s16 dx=target.x-enemy->pos.x,dy=target.y-enemy->pos.y;
    if ((long)dx*dx+(long)dy*dy>GRENADE_RANGE*GRENADE_RANGE || !world_lob_line(game->level,enemy->pos,target)) return false;
    game->bombs[owner]=(Bomb){enemy->pos,target,BOMB_FLIGHT,BOMB_FLYING};
    return true;
}
u16 bomb_cells(u16 level,const Bomb *bomb,Point cells[1+4*BOMB_REACH])
{
    if (bomb->phase!=BOMB_EXPLODING) return 0;
    u16 cell=world_cell(bomb->target),count=0;
    if (cell==NO_CELL || world_wall(level,cell%GRID_W,cell/GRID_W)) return 0;
    cells[count++]=world_point(cell);
    static const s16 dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
    for (u16 d=0;d<4;++d) for (u16 step=1;step<=BOMB_REACH;++step) {
        s16 x=cell%GRID_W+dx[d]*step,y=cell/GRID_W+dy[d]*step;
        if (world_wall(level,x,y)) break;
        cells[count++]=world_point(y*GRID_W+x);
    }
    return count;
}
void bombs_tick(Game *game)
{
    for (u16 i=0;i<BOMB_COUNT;++i) {
        Bomb *bomb=&game->bombs[i]; if (bomb->phase==BOMB_FREE) continue;
        if (bomb->timer) --bomb->timer;
        if (!bomb->timer) {
            if (bomb->phase==BOMB_FLYING) { bomb->phase=BOMB_FUSED; bomb->timer=BOMB_FUSE; }
            else if (bomb->phase==BOMB_FUSED) {
                bomb->phase=BOMB_EXPLODING; bomb->timer=BOMB_BLAST; game->events|=EVENT_FIRE;
            } else { bomb->phase=BOMB_FREE; continue; }
        }
        Point cells[1+4*BOMB_REACH]; u16 count=bomb_cells(game->level,bomb,cells);
        for (u16 p=0;p<2;++p) {
            const Player *player=&game->players[p];
            for (u16 c=0;c<count;++c)
                if (player->pos.x+ACTOR_INSET<cells[c].x+16 && cells[c].x<player->pos.x+ACTOR_INSET+ACTOR_SIZE &&
                    player->pos.y+ACTOR_INSET<cells[c].y+16 && cells[c].y<player->pos.y+ACTOR_INSET+ACTOR_SIZE)
                    player_hit(game,p);
        }
    }
}
