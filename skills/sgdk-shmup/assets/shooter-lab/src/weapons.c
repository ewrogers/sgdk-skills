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
        u16 damage=game->boss.hp>20*DAMAGE_UNIT?20*DAMAGE_UNIT:game->boss.hp>DAMAGE_UNIT?game->boss.hp-DAMAGE_UNIT:0;
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

/* Native Q4 offsets avoid per-frame wide multiplication/division. The
 * 64 samples interpolate the shared 16-direction orbit at a 20-pixel radius. */
static const s16 orbit_x[64]={
    0,30,61,91,122,148,174,200,226,243,261,278,296,302,308,314,
    320,314,308,302,296,278,261,243,226,200,174,148,122,91,61,30,
    0,-30,-61,-91,-122,-148,-174,-200,-226,-243,-261,-278,-296,-302,-308,-314,
    -320,-314,-308,-302,-296,-278,-261,-243,-226,-200,-174,-148,-122,-91,-61,-30,
};
static const s16 orbit_y[64]={
    -320,-314,-308,-302,-296,-278,-261,-243,-226,-200,-174,-148,-122,-91,-61,-30,
    0,30,61,91,122,148,174,200,226,243,261,278,296,302,308,314,
    320,314,308,302,296,278,261,243,226,200,174,148,122,91,61,30,
    0,-30,-61,-91,-122,-148,-174,-200,-226,-243,-261,-278,-296,-302,-308,-314,
};
void game_twin_point(const Game *game,u16 index,s16 *x,s16 *y)
{
    u16 phase=((u16)(game->ticks>>1)+index*32)&63;
    *x=game->x+orbit_x[phase]; *y=game->y+orbit_y[phase];
}
