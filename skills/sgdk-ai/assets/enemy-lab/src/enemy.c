#include "world.h"

void enemy_change(Enemy *enemy,State state,u16 duration)
{
    enemy->state=state; enemy->timer=duration; enemy->moving=false;
    enemy->route_len=enemy->route_at=0; enemy->pending=false; enemy->repath=0;
    ++enemy->serial; /* Invalidate an in-flight planner result after a state change. */
}
static void follow(Game *game,Enemy *enemy)
{
    if (enemy->route_at>=enemy->route_len) { enemy->route_len=enemy->route_at=0; return; }
    Point next=world_point(enemy->route[enemy->route_at]);
    s16 dx=(next.x>enemy->pos.x)-(next.x<enemy->pos.x);
    s16 dy=dx ? 0 : (next.y>enemy->pos.y)-(next.y<enemy->pos.y);
    enemy->moving=world_move(game->level,&enemy->pos,dx,dy);
    if (dx) { enemy->facing=dx>0 ? 1 : 3; enemy->flip=dx<0; }
    else if (dy) { enemy->facing=dy>0 ? 0 : 2; enemy->flip=false; }
    if (enemy->pos.x==next.x && enemy->pos.y==next.y) {
        ++enemy->route_at;
        if (enemy->state==CHASE) enemy->route_len=enemy->route_at=0;
    }
}
void enemy_tick(Game *game,u16 index)
{
    Enemy *enemy=&game->enemies[index]; enemy->moving=false;
    if (enemy->alert) --enemy->alert;
    if (enemy->memory) --enemy->memory;
    if (enemy->repath) --enemy->repath;
    if (enemy->state==STUNNED || enemy->state==RECOVER) {
        if (enemy->timer) --enemy->timer;
        if (!enemy->timer) enemy_change(enemy,RETURN,0);
        return;
    }
    if (enemy->state==WINDUP) {
        if (enemy->timer) --enemy->timer;
        if (!enemy->timer) {
            if (enemy->kind==SENTRY) {
                if (shot_fire(game,enemy->pos,enemy->aim)) game->events|=EVENT_FIRE;
            }
            else if (enemy->target<2) {
                Player *player=&game->players[enemy->target];
                s16 dx=player->pos.x-enemy->pos.x,dy=player->pos.y-enemy->pos.y;
                if (player->active && (long)dx*dx+(long)dy*dy<=24*24 && world_line(game->level,enemy->pos,player->pos))
                    player_hit(game,enemy->target);
            }
            enemy_change(enemy,RECOVER,45);
        }
        return;
    }
    if (enemy->state==CHASE && !enemy->memory) enemy_change(enemy,RETURN,0);
    if (enemy->state==IDLE) return;
    if (enemy->state==RETURN && enemy->pos.x==enemy->home.x && enemy->pos.y==enemy->home.y) {
        enemy_change(enemy,enemy->kind==SENTRY ? IDLE : PATROL,0); return;
    }
    if (enemy->state==PATROL) {
        Point goal=enemy->outbound ? enemy->patrol : enemy->home;
        if (enemy->pos.x==goal.x && enemy->pos.y==goal.y) enemy->outbound=!enemy->outbound;
    }
    follow(game,enemy);
    if (!enemy->route_len) nav_request(enemy);
    enemy->animation=(enemy->animation+1)%(enemy->moving ? 32 : 96);
}
