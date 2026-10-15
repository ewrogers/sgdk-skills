#include "game.h"

static bool inside(s16 x,s16 y,s16 cx,s16 cy,s16 half_x,s16 half_y)
{
    return x>=cx-half_x && x<cx+half_x && y>=cy-half_y && y<cy+half_y;
}

void game_add_score(Game *game,u32 amount)
{
    if (amount>99999999UL-game->score) game->score=99999999UL;
    else game->score+=amount;
}

void game_effect(Game *game,s16 x,s16 y)
{
    for (u16 i=0;i<EFFECT_COUNT;i++)
        if (!game->effects[i].life)
        {
            game->effects[i]=(Effect){.x=x,.y=y,.life=24,
                .vx=(s16)((i%3)-1)*UNIT,.vy=-UNIT};
            return;
        }
}

void game_enemy_kill(Game *game,u16 index)
{
    Enemy *enemy=&game->enemies[index];
    if (!enemy->active) return;
    enemy->active=false;
    if (game->combo<10) game->combo++;
    game_add_score(game,100+game->combo*10);
    game->kills++; game->events|=EVENT_KILL;
    game_effect(game,enemy->x,enemy->y);
    if (game->kills%3) return;
    u8 kind=(game->kills/3-1)%(game->stage==2?4:3);
    if (!(kind==2 && game->shield) && !(kind==3 && game->twins==2))
        for (u16 i=0;i<PICKUP_COUNT;i++)
            if (!game->pickups[i].active)
            {
                game->pickups[i]=(Pickup){.x=enemy->x,.y=enemy->y,
                    .kind=kind,.active=true};
                break;
            }
}

void game_hurt(Game *game)
{
    if (game->invulnerable || game->state==CLEAR || game->state==OVER) return;
    if (game->shield)
    {
        game->shield=false; game->shield_time=0; game->invulnerable=60;
        game->events|=EVENT_SHIELD_BREAK; game_effect(game,game->x,game->y); return;
    }
    game->events|=EVENT_HURT; game->combo=0;
    game->power=1; game->twins=0; game->shield=false; game->shield_time=0;
    for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
    game_effect(game,game->x,game->y);
    if (--game->lives==0) { game->state=OVER; game->phase_time=0; return; }
    game->invulnerable=120; game->x=120*UNIT; game->y=192*UNIT;
}

void game_boss_damage(Game *game,u16 damage)
{
    Boss *boss=&game->boss;
    if (!boss->active || boss->age<BOSS_READY_TICKS || game_boss_guarded(game) || !damage) return;
    boss->flash=4;
    if (damage<boss->hp) { boss->hp-=damage; return; }
    game->payload.active=false;
    boss->hp=0; boss->active=false; game->state=CLEAR; game->phase_time=0;
    game->events|=EVENT_CLEAR; game_add_score(game,5000);
    for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
}

static bool hit_shot(Game *game,s16 x,s16 y,u16 candidates)
{
    if (candidates) for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if ((candidates&(1U<<i)) && enemy->active && inside(x,y,enemy->x,enemy->y,10*UNIT,10*UNIT))
        {
            enemy->flash=6;
            if (--enemy->hp==0) game_enemy_kill(game,i);
            return true;
        }
    }
    Boss *boss=&game->boss;
    if (boss->active && boss->age>=BOSS_READY_TICKS && inside(x,y,boss->x,boss->y,15*UNIT,13*UNIT))
    {
        game_boss_damage(game,1);
        return true;
    }
    return false;
}

/* Conservative swept bounds only reject impossible targets. The narrow phase
 * below retains the original substep positions and enemy index priority. */
static bool near_path(const Bullet *bullet,s16 x,s16 y,s16 hx,s16 hy)
{
    s16 end_x=bullet->x+bullet->vx,end_y=bullet->y+bullet->vy;
    s16 left=bullet->x<end_x?bullet->x:end_x;
    s16 right=bullet->x>end_x?bullet->x:end_x;
    s16 top=bullet->y<end_y?bullet->y:end_y;
    s16 bottom=bullet->y>end_y?bullet->y:end_y;
    return right>=x-hx && left<x+hx && bottom>=y-hy && top<y+hy;
}

static void move(Game *game,Bullet *bullet,bool player)
{
    u16 candidates=0;
    bool collision=false;
    if (player)
    {
        for (u16 i=0;i<ENEMY_COUNT;i++)
            if (game->enemies[i].active && near_path(bullet,game->enemies[i].x,game->enemies[i].y,10*UNIT,10*UNIT))
                candidates|=1U<<i;
        collision=candidates || (game->boss.active && game->boss.age>=BOSS_READY_TICKS &&
            near_path(bullet,game->boss.x,game->boss.y,15*UNIT,13*UNIT));
    }
    else collision=!game->invulnerable && near_path(bullet,game->x,game->y,4*UNIT,4*UNIT);
    if (!collision)
    { bullet->x+=bullet->vx; bullet->y+=bullet->vy; }
    else
    {
        /* Power-of-two substeps keep samples at most one pixel apart without
         * variable division in the inner loop. Positions remain in Q4 units. */
        s16 ax=bullet->vx<0?-bullet->vx:bullet->vx;
        s16 ay=bullet->vy<0?-bullet->vy:bullet->vy;
        u16 count=1,shift=0,progress_x=0,progress_y=0;
        while (ax>UNIT*count || ay>UNIT*count) { count*=2; shift++; }
        s16 x=bullet->x,y=bullet->y;
        for (u16 step=1;step<=count;step++)
        {
            progress_x+=ax; progress_y+=ay;
            s16 offset_x=progress_x>>shift,offset_y=progress_y>>shift;
            bullet->x=x+(bullet->vx<0?-offset_x:offset_x);
            bullet->y=y+(bullet->vy<0?-offset_y:offset_y);
            if (player?hit_shot(game,bullet->x,bullet->y,candidates):
                (!game->invulnerable && inside(bullet->x,bullet->y,game->x,game->y,4*UNIT,4*UNIT)))
            {
                bullet->active=false;
                if (!player) game_hurt(game);
                return;
            }
        }
    }
    if (bullet->x<-8*UNIT || bullet->x>(FIELD_W+8)*UNIT ||
        bullet->y<-8*UNIT || bullet->y>(FIELD_H+8)*UNIT) bullet->active=false;
}

void game_projectiles(Game *game)
{
    for (u16 i=0;i<SHOT_COUNT;i++)
        if (game->shots[i].active)
        { move(game,&game->shots[i],true); if (game->state==CLEAR) return; }
    for (u16 i=0;i<BULLET_COUNT;i++)
        if (game->bullets[i].active)
        { move(game,&game->bullets[i],false); if (game->state==OVER) return; }
}
