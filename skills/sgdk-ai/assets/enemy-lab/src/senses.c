#include "world.h"

u16 enemy_range(Kind kind) { return kind==SENTRY ? 144 : kind==BOMBARDIER ? 112 : 96; }
bool enemy_cone(u8 facing,s16 dx,s16 dy)
{
    s16 forward=facing==0 ? dy : facing==1 ? dx : facing==2 ? -dy : -dx;
    s16 side=(facing&1) ? dy : dx;
    if (side<0) side=-side;
    return forward>=0 && side<=forward;
}
bool enemy_visible(u16 level,const Enemy *enemy,const Player *player,bool tracking)
{
    if (!player->active) return false;
    s16 dx=player->pos.x-enemy->pos.x,dy=player->pos.y-enemy->pos.y;
    u16 range=enemy_range(enemy->kind)+(tracking ? 32 : 0);
    if ((u32)((long)dx*dx+(long)dy*dy)>(u32)range*range) return false;
    if (enemy->kind==BOMBARDIER && !tracking && !enemy_cone(enemy->facing,dx,dy)) return false;
    return enemy->kind==BOMBARDIER ? world_lob_line(level,enemy->pos,player->pos) : world_line(level,enemy->pos,player->pos);
}
void enemy_sense(Game *game,u16 index)
{
    Enemy *enemy=&game->enemies[index]; ++game->sensors;
    if (enemy->state==STUNNED || enemy->state==WINDUP || enemy->state==RECOVER || enemy->state==ZAP) return;
    u16 target=2; u32 best=0xFFFFFFFFUL;
    if (enemy->state==CHASE && enemy->target<2 && enemy_visible(game->level,enemy,&game->players[enemy->target],true))
        target=enemy->target;
    else for (u16 p=0;p<2;++p) if (enemy_visible(game->level,enemy,&game->players[p],false)) {
        s16 dx=game->players[p].pos.x-enemy->pos.x,dy=game->players[p].pos.y-enemy->pos.y;
        u32 distance=(long)dx*dx+(long)dy*dy;
        if (distance<best) { best=distance; target=p; }
    }
    if (target==2) return;
    if (!enemy->memory) { enemy->alert=60; game->events|=EVENT_ALERT; }
    enemy->target=target; enemy->last_seen=game->players[target].pos; enemy->memory=120;
    s16 dx=enemy->last_seen.x-enemy->pos.x,dy=enemy->last_seen.y-enemy->pos.y;
    s16 ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
    enemy->facing=ax>=ay ? (dx<0 ? 3 : 1) : (dy<0 ? 2 : 0);
    enemy->flip=enemy->facing==3;
    u16 reach=enemy->kind==SENTRY ? 128 : enemy->kind==BOMBARDIER ? GRENADE_RANGE : 24;
    if ((u32)((long)dx*dx+(long)dy*dy)<=(u32)reach*reach) {
        enemy->aim=aim_direction(dx,dy);
        enemy_change(enemy,WINDUP,enemy->kind==SENTRY ? 36 : 24);
    } else if (enemy->state!=CHASE) enemy_change(enemy,CHASE,0);
}
u8 aim_direction(s16 dx,s16 dy)
{
    u16 ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
    if (ay*2<ax) return dx<0 ? 4 : 0;
    if (ax*2<ay) return dy<0 ? 6 : 2;
    if (dy<0) return dx<0 ? 5 : 7;
    return dx<0 ? 3 : 1;
}
