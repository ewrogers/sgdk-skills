#include "game.h"

void game_init(Game *game)
{
    *game=(Game){0}; game->state=TITLE;
    game->x=120*UNIT; game->y=192*UNIT; game->lives=3; game->power=1; game->bombs=2; game->stage=1;
}
void game_start(Game *game)
{
    game_init(game); game->state=PLAY; game->invulnerable=90; game->events=EVENT_START;
}

static void next_stage(Game *game)
{
    u32 score=game->score;
    u8 lives=game->lives,power=game->power,bombs=game->bombs,twins=game->twins;
    bool shield=game->shield; u16 shield_time=game->shield_time,held=game->held;
    game_start(game); game->stage=2; game->score=score;
    game->lives=lives; game->power=power; game->bombs=bombs; game->twins=twins;
    game->shield=shield; game->shield_time=shield_time; game->held=held;
}

static void player(Game *game,u16 held,u16 pressed)
{
    s16 dx=((held&RIGHT)!=0)-((held&LEFT)!=0);
    s16 dy=((held&DOWN)!=0)-((held&UP)!=0);
    s16 speed=(held&C)?3*UNIT/2:3*UNIT;
    if (dx && dy) speed=speed*181/256;
    game->x+=dx*speed; game->y+=dy*speed;
    if (game->x<8*UNIT) game->x=8*UNIT;
    if (game->x>(FIELD_W-8)*UNIT) game->x=(FIELD_W-8)*UNIT;
    if (game->y<8*UNIT) game->y=8*UNIT;
    if (game->y>(FIELD_H-8)*UNIT) game->y=(FIELD_H-8)*UNIT;
    if (game->fire_wait) game->fire_wait--;
    if ((held&A) && !game->fire_wait)
    {
        bool fired=false;
        if (game->power==1) fired=game_shot(game,game->x,game->y-8*UNIT,0,-6*UNIT);
        else
        {
            fired=game_shot(game,game->x-4*UNIT,game->y-6*UNIT,-UNIT,-6*UNIT);
            fired=game_shot(game,game->x+4*UNIT,game->y-6*UNIT,UNIT,-6*UNIT)||fired;
            if (game->power==3) fired=game_shot(game,game->x,game->y-8*UNIT,0,-6*UNIT)||fired;
        }
        if ((game->ticks/10)%2==0)
            for (u16 i=0;i<game->twins;i++)
            {
                s16 x,y; game_twin_point(game,i,&x,&y);
                fired=game_shot(game,x,y-6*UNIT,0,-6*UNIT)||fired;
            }
        if (fired) game->events|=EVENT_FIRE;
        game->fire_wait=10;
    }
    if (pressed&B) game_bomb_launch(game);
}

static void pickups(Game *game)
{
    for (u16 i=0;i<PICKUP_COUNT;i++)
    {
        Pickup *item=&game->pickups[i];
        if (!item->active) continue;
        item->y+=UNIT/2;
        if (item->y>FIELD_H*UNIT) { item->active=false; continue; }
        s16 dx=item->x-game->x,dy=item->y-game->y;
        if (dx>-12*UNIT && dx<12*UNIT && dy>-12*UNIT && dy<12*UNIT)
        {
            item->active=false; game->events|=EVENT_PICKUP;
            game->pickup_kind=item->kind;
            if (item->kind==3) { if (game->twins<2) game->twins++; else game_add_score(game,250); }
            else if (item->kind==2) { game->shield=true; game->shield_time=SHIELD_TICKS; }
            else if (!item->kind) { if (game->power<3) game->power++; else game_add_score(game,250); }
            else { if (game->bombs<3) game->bombs++; else game_add_score(game,250); }
        }
    }
}

void game_step(Game *game,u16 held,u16 pressed)
{
    if (game->paused || game->state==TITLE) return;
    if (game->state==CLEAR || game->state==OVER)
    {
        if (game->phase_time<65535) game->phase_time++;
        if (game->state==CLEAR && game->stage==1 && game->phase_time>=360) next_stage(game);
        return;
    }
    game->ticks++; game->phase_time++;
    if (game->shield_time && --game->shield_time==0) game->shield=false;
    if (game->invulnerable) game->invulnerable--;
    if (game->bomb_flash) game->bomb_flash--;
    player(game,held,pressed);
    game_stage(game); game_enemies(game);
    if (game->state==OVER) return;
    game_bomb_step(game);
    game_projectiles(game);
    if (game->state==CLEAR || game->state==OVER) return;
    pickups(game);
    for (u16 i=0;i<EFFECT_COUNT;i++)
    {
        Effect *effect=&game->effects[i];
        if (effect->life) { effect->life--; effect->x+=effect->vx; effect->y+=effect->vy; }
    }
    if (game->stage==2 && (game->ticks==240 || game->ticks==600))
        for (u16 i=0;i<PICKUP_COUNT;i++) if (!game->pickups[i].active)
        { game->pickups[i]=(Pickup){.x=120*UNIT,.y=16*UNIT,.kind=3,.active=true}; break; }
    /* Guaranteed upgrades keep the sample approachable even after misses. */
    if (game->ticks==360 || game->ticks==720 || game->ticks==1080 || game->ticks==1440)
        for (u16 i=0;i<PICKUP_COUNT;i++) if (!game->pickups[i].active)
        { game->pickups[i]=(Pickup){.x=120*UNIT,.y=16*UNIT,.kind=game->ticks%720==0?2:0,.active=true}; break; }
}

void game_frame(Game *game,u16 held,u16 hz)
{
    u16 pressed=held&~game->held;
    game->held=held; game->events=0;
    if ((game->state==TITLE || game->state==CLEAR || game->state==OVER) && (pressed&(A|START)))
    {
        if (game->state==CLEAR && game->phase_time<BOSS_DEATH_TICKS) return;
        if (game->state==CLEAR && game->stage==1) next_stage(game);
        else game_start(game);
        game->held=held; return;
    }
    if (game->state==PLAY || game->state==BOSS)
        if (pressed&START) game->paused=!game->paused;
    if (game->paused || (hz!=50 && hz!=60)) return;
    game->clock+=60;
    while (game->clock>=hz)
    { game->clock-=hz; game_step(game,held,pressed); pressed=0; }
}
