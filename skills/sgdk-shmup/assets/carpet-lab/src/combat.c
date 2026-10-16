#include "game.h"
static bool inside(s16 x,s16 y,s16 cx,s16 cy,s16 hx,s16 hy)
{ return x>=cx-hx && x<cx+hx && y>=cy-hy && y<cy+hy; }
void game_enemy_kill(Game *game,u16 index)
{
    Enemy *enemy=&game->enemies[index]; if (!enemy->active) return;
    enemy->active=false; game->score+=100; game->events|=EV_KILL;
    game_effect(game,enemy->x,enemy->y,0);
    game->enemy_kills++;
    if (game->enemy_kills<=2 || game->enemy_kills%4==2) game_gem(game,enemy->x,enemy->y);
}
void game_hurt(Game *game)
{
    if (game->invulnerable || (game->state!=PLAY && game->state!=BOSS)) return;
    game->ribbon_active=false;
    game->events|=EV_HIT;
    if (game->barrier) { game->barrier--; game->invulnerable=60; return; }
    game_effect(game,game->x,game->y,1);
    game->speed=0; game->weapon=0; game->familiars=0; game->meter=0;
    for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
    if (--game->lives==0) { game->state=OVER; game->age=0; return; }
    game->invulnerable=150; game->x=64*UNIT; game->y=128*UNIT;
    for (u16 i=0;i<32;i++) game->history[i]=(Point){game->x,game->y};
}
void game_boss_damage(Game *game,u16 damage)
{
    Boss *boss=&game->boss;
    if (!boss->active || boss->age<180 || game_boss_guarded(game)) return;
    if (damage<boss->hp) { boss->hp-=damage; return; }
    game->ribbon_active=false;
    boss->hp=0; boss->active=false; game->state=CLEAR; game->age=0;
    game->events|=EV_CLEAR; game->score+=5000;
    for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
    for (u16 i=0;i<SHOT_COUNT;i++) game->shots[i].active=false;
}
static bool hit(Game *game,Shot *shot,u16 candidates)
{
    bool piercing=shot->kind==1 || shot->kind==4;
    if (candidates&255) for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if (!(candidates&(1U<<i)) || !enemy->active || (shot->hit_mask&(1U<<i)) || !inside(shot->x,shot->y,enemy->x,enemy->y,14*UNIT,12*UNIT)) continue;
        shot->hit_mask|=1U<<i;
        if (shot->damage>=enemy->hp) game_enemy_kill(game,i); else enemy->hp-=shot->damage;
        if (shot->kind==5) game_chain(game,shot);
        if (!piercing) return true;
    }
    if ((candidates&256) && game->boss.active && !(shot->hit_mask&256) && inside(shot->x,shot->y,game->boss.x,game->boss.y,28*UNIT,26*UNIT))
    { shot->hit_mask|=256; game_boss_damage(game,shot->damage); if (shot->kind==5 && game->state!=CLEAR) game_chain(game,shot); if (!piercing) return true; }
    return false;
}
static void seeking(Game *game,Shot *shot)
{
    if (shot->age&7) return;
    s16 target_y=shot->y,nearest=32767;
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i]; s16 distance=enemy->x-shot->x;
        if (enemy->active && distance>=0 && distance<nearest) { target_y=enemy->y; nearest=distance; }
    }
    if (game->boss.active && game->boss.x>shot->x) target_y=game->boss.y;
    if (target_y>shot->y+4*UNIT && shot->vy<2*UNIT) shot->vy+=UNIT/4;
    else if (target_y<shot->y-4*UNIT && shot->vy>-2*UNIT) shot->vy-=UNIT/4;
}
void game_shot_motion(Game *game,Shot *shot)
{
    if (shot->kind==1)
    {
        /* Fixed lanes and a short, smooth hook; never chase the player's
         * current position, so every crescent follows the same return arc. */
        static const s16 hook_x[8]={6,4,2,0,-2,-4,-6,-8};
        static const s16 hook_y[8]={-2,-3,-3,-2,2,3,3,2};
        if (shot->age<=20) { shot->vx=8*UNIT; shot->vy=0; }
        else if (shot->age<=28)
        { shot->vx=hook_x[shot->age-21]*UNIT; shot->vy=hook_y[shot->age-21]*UNIT; }
        else { shot->vx=-8*UNIT; shot->vy=0; }
    }
    else if (shot->kind==2 || shot->kind==5) seeking(game,shot);
}
void game_chain(Game *game,Shot *shot)
{
    s16 x=shot->x,y=shot->y;
    for (u16 jump=0;jump<2;jump++)
    {
        u16 target=ENEMY_COUNT; s16 best=32767;
        for (u16 i=0;i<ENEMY_COUNT;i++)
        {
            Enemy *enemy=&game->enemies[i];
            if (!enemy->active || (shot->hit_mask&(1U<<i))) continue;
            s16 dx=enemy->x-x,dy=enemy->y-y;
            if (dx<0) dx=-dx;
            if (dy<0) dy=-dy;
            if (dx>64*UNIT || dy>64*UNIT || dx+dy>=best) continue;
            best=dx+dy; target=i;
        }
        if (target==ENEMY_COUNT) break;
        Enemy *enemy=&game->enemies[target]; shot->hit_mask|=1U<<target;
        game_effect(game,(x+enemy->x)/2,(y+enemy->y)/2,2);
        x=enemy->x; y=enemy->y;
        if (enemy->hp<=1) game_enemy_kill(game,target); else enemy->hp--;
    }
}
static bool near_path(const Shot *shot,s16 x,s16 y,s16 hx,s16 hy)
{
    s16 end_x=shot->x+shot->vx,end_y=shot->y+shot->vy;
    s16 left=shot->x<end_x?shot->x:end_x,right=shot->x>end_x?shot->x:end_x;
    s16 top=shot->y<end_y?shot->y:end_y,bottom=shot->y>end_y?shot->y:end_y;
    return right>=x-hx && left<x+hx && bottom>=y-hy && top<y+hy;
}
void game_ribbon(Game *game)
{
    if (!game->ribbon_active || game->ribbon_wait) return;
    game->ribbon_wait=6;
    s16 start=game->x+12*UNIT,end=start+RIBBON_REACH*UNIT;
    if (end>320*UNIT) end=320*UNIT;
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if (!enemy->active || enemy->x+14*UNIT<start || enemy->x-14*UNIT>=end ||
            enemy->y-game->y>=18*UNIT || game->y-enemy->y>18*UNIT) continue;
        if (enemy->hp<=2) game_enemy_kill(game,i); else enemy->hp-=2;
    }
    if (game->boss.active && game->boss.x+28*UNIT>=start && game->boss.x-28*UNIT<end &&
        game->boss.y-game->y<32*UNIT && game->y-game->boss.y<=32*UNIT)
        game_boss_damage(game,2);
}
void game_combat(Game *game)
{
    game_ribbon(game);
    if (game->state==CLEAR || game->state==OVER) return;
    for (u16 group=0;group<2;group++)
    {
        Shot *pool=group?game->bullets:game->shots; u16 count=group?BULLET_COUNT:SHOT_COUNT;
        for (u16 i=0;i<count;i++)
        {
            Shot *shot=&pool[i]; if (!shot->active) continue;
            shot->age++;
            if (!group) game_shot_motion(game,shot);
            u16 candidates=0;
            if (!group)
            {
                for (u16 j=0;j<ENEMY_COUNT;j++)
                    if (game->enemies[j].active && !(shot->hit_mask&(1U<<j)) &&
                        near_path(shot,game->enemies[j].x,game->enemies[j].y,14*UNIT,12*UNIT)) candidates|=1U<<j;
                if (game->boss.active && !(shot->hit_mask&256) && near_path(shot,game->boss.x,game->boss.y,28*UNIT,26*UNIT)) candidates|=256;
            }
            else if (!game->invulnerable && near_path(shot,game->x,game->y,5*UNIT,5*UNIT)) candidates=1;
            /* Reject only impossible hits; retain the original one-pixel
             * samples, target ordering and piercing consumption rules. */
            if (!candidates) { shot->x+=shot->vx; shot->y+=shot->vy; }
            else {
            s16 ax=shot->vx<0?-shot->vx:shot->vx,ay=shot->vy<0?-shot->vy:shot->vy;
            u16 steps=1,shift=0,px=0,py=0;
            while (ax>UNIT*steps || ay>UNIT*steps) { steps*=2; shift++; }
            s16 x=shot->x,y=shot->y;
            for (u16 step=0;step<steps;step++)
            {
                px+=ax; py+=ay;
                shot->x=x+(shot->vx<0?-(s16)(px>>shift):(s16)(px>>shift));
                shot->y=y+(shot->vy<0?-(s16)(py>>shift):(s16)(py>>shift));
                bool collided=group?(!game->invulnerable && inside(shot->x,shot->y,game->x,game->y,5*UNIT,5*UNIT)):hit(game,shot,candidates);
                if (collided) { shot->active=false; if (group) game_hurt(game); break; }
                if (game->state==CLEAR || game->state==OVER) return;
            }
            }
            if (shot->x<-32*UNIT || shot->x>352*UNIT || shot->y<(HUD_HEIGHT-16)*UNIT || shot->y>240*UNIT || shot->age>180) shot->active=false;
        }
    }
}
