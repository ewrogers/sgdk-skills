#include "game.h"
static bool inside(s16 x,s16 y,s16 cx,s16 cy,s16 hx,s16 hy)
{ return x>=cx-hx && x<cx+hx && y>=cy-hy && y<cy+hy; }
static bool hit(Game *game,Shot *shot)
{
    bool piercing=shot->kind==1 || shot->kind==4;
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if (!enemy->active || (shot->hit_mask&(1U<<i)) || !inside(shot->x,shot->y,enemy->x,enemy->y,14*UNIT,12*UNIT)) continue;
        shot->hit_mask|=1U<<i;
        if (shot->damage>=enemy->hp) game_enemy_kill(game,i); else enemy->hp-=shot->damage;
        if (shot->kind==5) game_chain(game,shot);
        if (!piercing) return true;
    }
    if (game->boss.active && !(shot->hit_mask&256) && inside(shot->x,shot->y,game->boss.x,game->boss.y,28*UNIT,26*UNIT))
    { shot->hit_mask|=256; game_boss_damage(game,shot->damage); if (shot->kind==5 && game->state!=CLEAR) game_chain(game,shot); if (!piercing) return true; }
    return false;
}
void reference_game_combat(Game *game)
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
            /* At most one-pixel samples. Bounding pools keep the worst case
             * finite; broad-phase rejection can be added after an oracle test. */
            s16 ax=shot->vx<0?-shot->vx:shot->vx,ay=shot->vy<0?-shot->vy:shot->vy;
            u16 steps=1,shift=0,px=0,py=0;
            while (ax>UNIT*steps || ay>UNIT*steps) { steps*=2; shift++; }
            s16 x=shot->x,y=shot->y;
            for (u16 step=0;step<steps;step++)
            {
                px+=ax; py+=ay;
                shot->x=x+(shot->vx<0?-(s16)(px>>shift):(s16)(px>>shift));
                shot->y=y+(shot->vy<0?-(s16)(py>>shift):(s16)(py>>shift));
                bool collided=group?(!game->invulnerable && inside(shot->x,shot->y,game->x,game->y,5*UNIT,5*UNIT)):hit(game,shot);
                if (collided) { shot->active=false; if (group) game_hurt(game); break; }
                if (game->state==CLEAR || game->state==OVER) return;
            }
            if (shot->x<-32*UNIT || shot->x>352*UNIT || shot->y<(HUD_HEIGHT-16)*UNIT || shot->y>240*UNIT || shot->age>180) shot->active=false;
        }
    }
}
