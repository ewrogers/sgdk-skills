#include "game.h"

void game_bomb_launch(Game *game)
{
    if (!game->bombs) { game->events|=EVENT_BOMB_EMPTY; return; }
    if (game->payload.active) return;
    game->bombs--; game->events|=EVENT_BOMB;
    game->payload=(Payload){.x=game->x,.y=game->y,.active=true};
}

static bool reached(const Payload *payload,s16 x,s16 y)
{
    s32 dx=(x-payload->x)/UNIT,dy=(y-payload->y)/UNIT;
    return dx*dx+dy*dy<=(s32)payload->radius*payload->radius;
}

void game_bomb_step(Game *game)
{
    Payload *payload=&game->payload;
    if (!payload->active) return;
    payload->age++;
    if (payload->age<BOMB_ARM_TICKS) return;
    if (payload->age==BOMB_ARM_TICKS)
    { game->events|=EVENT_BLAST; game->bomb_flash=40; }
    payload->radius=(payload->age-BOMB_ARM_TICKS+1)*10;
    for (u16 i=0;i<BULLET_COUNT;i++)
        if (game->bullets[i].active && reached(payload,game->bullets[i].x,game->bullets[i].y))
            game->bullets[i].active=false;
    for (u16 i=0;i<ENEMY_COUNT;i++)
        if (game->enemies[i].active && reached(payload,game->enemies[i].x,game->enemies[i].y)) game_enemy_kill(game,i);
    if (!payload->boss_hit && !game_boss_guarded(game) && game->boss.active && game->boss.age>=BOSS_READY_TICKS &&
        reached(payload,game->boss.x,game->boss.y))
    {
        /* A bomb helps, but normal fire must finish the boss. */
        u16 damage=game->boss.hp>20?20:game->boss.hp>1?game->boss.hp-1:0;
        game_boss_damage(game,damage); payload->boss_hit=true;
    }
    if (payload->age>=BOMB_ARM_TICKS+BOMB_BLAST_TICKS) payload->active=false;
}

void game_blast_point(const Game *game,u16 index,s16 *x,s16 *y)
{
    u16 angle=(index*2)%16;
    *x=game->payload.x+(s32)direction_x[angle]*game->payload.radius*UNIT/256;
    *y=game->payload.y+(s32)direction_y[angle]*game->payload.radius*UNIT/256;
}

void game_twin_point(const Game *game,u16 index,s16 *x,s16 *y)
{
    /* Interpolate the shared 16-direction table to 64 orbit positions. Two
     * companions stay opposite; offsets are always relative to the ship. */
    u16 phase=(game->ticks/2+index*32)%64;
    u16 at=phase/4,fraction=phase%4,next=(at+1)%16;
    s32 dx=direction_x[at]*(4-fraction)+direction_x[next]*fraction;
    s32 dy=direction_y[at]*(4-fraction)+direction_y[next]*fraction;
    *x=game->x+dx*20*UNIT/1024;
    *y=game->y+dy*20*UNIT/1024;
}
