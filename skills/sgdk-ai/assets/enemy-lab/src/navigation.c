#include "world.h"

void nav_request(Enemy *enemy)
{
    if (!enemy->pending && !enemy->route_len && !enemy->repath) enemy->pending=true;
}
static bool begin(Game *game)
{
    Planner *nav=&game->nav;
    for (u16 offset=0;offset<ENEMY_COUNT;++offset) {
        u16 index=(nav->cursor+offset)%ENEMY_COUNT;
        Enemy *enemy=&game->enemies[index];
        if (!enemy->pending) continue;
        nav->owner=index; nav->cursor=(index+1)%ENEMY_COUNT;
        nav->level=game->level; nav->origin=enemy->pos; nav->start=world_goal(game->level,enemy->pos);
        Point goal=enemy->state==CHASE ? enemy->last_seen :
                   enemy->state==RETURN ? enemy->home : enemy->outbound ? enemy->patrol : enemy->home;
        nav->goal=world_goal(game->level,goal); nav->serial=enemy->serial;
        if (nav->start==NO_CELL || nav->goal==NO_CELL) {
            enemy->pending=false; enemy->repath=30; ++nav->failed; return false;
        }
        for (u16 cell=0;cell<GRID_CELLS;++cell) nav->parent[cell]=NO_CELL;
        nav->head=0; nav->tail=1; nav->queue[0]=nav->start; nav->parent[nav->start]=nav->start;
        nav->active=true; return true;
    }
    return false;
}
static void finish(Game *game,bool found)
{
    Planner *nav=&game->nav; Enemy *enemy=&game->enemies[nav->owner];
    enemy->pending=false; enemy->route_at=enemy->route_len=0; enemy->repath=12;
    if (found) {
        u16 length=0,cell=nav->goal;
        /* Search is finished; reuse its queue as a reversed reconstruction buffer. */
        while (cell!=nav->start) { nav->queue[length++]=cell; cell=nav->parent[cell]; }
        Point anchor=world_point(nav->start);
        if (anchor.x!=enemy->pos.x || anchor.y!=enemy->pos.y) enemy->route[enemy->route_len++]=nav->start;
        for (u16 i=0;i<length && enemy->route_len<ROUTE_CAPACITY;++i)
            enemy->route[enemy->route_len++]=nav->queue[length-1-i];
        ++nav->completed;
    } else ++nav->failed;
    nav->active=false;
}
void nav_frame(Game *game,u16 budget)
{
    Planner *nav=&game->nav; nav->expanded=0;
    if (budget>NAV_BUDGET) budget=NAV_BUDGET;
    if (!budget) return;
    if (nav->active) {
        Enemy *enemy=&game->enemies[nav->owner];
        if (nav->level!=game->level || !enemy->pending || enemy->serial!=nav->serial ||
            enemy->pos.x!=nav->origin.x || enemy->pos.y!=nav->origin.y) {
            nav->active=false; ++nav->cancelled;
        }
    }
    if (!nav->active && !begin(game)) return;
    while (nav->active && nav->expanded<budget) {
        if (nav->head==nav->tail) { finish(game,false); break; }
        u16 cell=nav->queue[nav->head++]; ++nav->expanded;
        if (cell==nav->goal) { finish(game,true); break; }
        s16 x=cell%GRID_W,y=cell/GRID_W;
        static const s16 dx[]={-1,1,0,0},dy[]={0,0,-1,1};
        for (u16 d=0;d<4;++d) {
            s16 nx=x+dx[d],ny=y+dy[d];
            if (world_wall(game->level,nx,ny)) continue;
            u16 next=ny*GRID_W+nx;
            if (nav->parent[next]!=NO_CELL) continue;
            nav->parent[next]=cell; nav->queue[nav->tail++]=next;
        }
    }
}
