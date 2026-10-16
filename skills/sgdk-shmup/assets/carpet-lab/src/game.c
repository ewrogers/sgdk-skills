#include "game.h"

void game_init(Game *game)
{
    *game=(Game){0}; game->state=TITLE; game->stage=1; game->lives=3;
    game->x=64*UNIT; game->y=128*UNIT;
}
void game_stage_begin(Game *game,u8 stage)
{
    for (u16 i=0;i<SHOT_COUNT;i++) game->shots[i].active=false;
    for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
    for (u16 i=0;i<ENEMY_COUNT;i++) game->enemies[i].active=false;
    for (u16 i=0;i<GEM_COUNT;i++) game->gems[i].active=false;
    for (u16 i=0;i<EFFECT_COUNT;i++) game->effects[i].age=0;
    game->ribbon_active=false; game->ribbon_wait=0;
    game->boss=(Boss){0}; game->stage=stage; game->state=STAGECARD;
    game->age=0; game->stage_time=0; game->spawn_index=0; game->enemy_kills=0;
    game->x=64*UNIT; game->y=128*UNIT; game->invulnerable=120;
    for (u16 i=0;i<32;i++) game->history[i]=(Point){game->x,game->y};
}
void game_begin(Game *game,Loadout loadout)
{
    u16 held=game->held; game_init(game); game->held=held;
    game->loadout=loadout; game_stage_begin(game,1); game->events=EV_START;
}
void game_effect(Game *game,s16 x,s16 y,u8 kind)
{
    for (u16 i=0;i<EFFECT_COUNT;i++) if (!game->effects[i].age)
    { game->effects[i]=(Effect){x,y,1,kind}; break; }
}
void game_gem(Game *game,s16 x,s16 y)
{
    for (u16 i=0;i<GEM_COUNT;i++) if (!game->gems[i].active)
    { game->gems[i]=(Gem){.x=x,.y=y,.active=true}; break; }
}
bool game_upgrade_available(const Game *game,u16 slot)
{
    switch (slot)
    {
        case 1: return game->speed<3;
        case 2: return !(game->weapon&1);
        case 3: return !(game->weapon&2);
        case 4: return game->familiars<2;
        case 5: return game->barrier<2;
        default: return false;
    }
}
void game_upgrade(Game *game)
{
    if (!game_upgrade_available(game,game->meter)) { game->events|=EV_DENIED; return; }
    bool accepted=false;
    switch (game->meter)
    {
        case 1: if (game->speed<3) { game->speed++; accepted=true; } break;
        case 2: game->weapon|=1; accepted=true; break;
        case 3: game->weapon|=2; accepted=true; break;
        case 4: if (game->familiars<2) { game->familiars++; accepted=true; } break;
        case 5: game->barrier=2; accepted=true; break;
        default: break;
    }
    if (accepted) { game->meter=0; game->events|=EV_UPGRADE; }
    else game->events|=EV_DENIED;
}
void game_step(Game *game,u16 held,u16 pressed)
{
    if (game->paused) return;
    game->ticks++; game->age++;
    if (game->state==STAGECARD)
    { if (game->age>=150 || (game->age>=45 && (pressed&(A|START)))) { game->state=PLAY; game->age=0; } return; }
    if (game->state==CLEAR)
    {
        if (game->age<150) game_player(game,held,false);
        if (game->age>=330 || (game->age>=150 && (pressed&(A|START))))
        {
            if (game->stage<3) game_stage_begin(game,game->stage+1);
            else { game->state=CREDITS; game->age=0; }
        }
    }
    if (game->state!=PLAY && game->state!=BOSS) return;
    if (game->invulnerable) game->invulnerable--;
    game_player(game,held,true);
    if (pressed&B) game_upgrade(game);
    game_encounters(game);
    if (game->state==OVER) return;
    game_combat(game);
    if (game->state==CLEAR || game->state==OVER) return;
    for (u16 i=0;i<GEM_COUNT;i++)
    {
        Gem *gem=&game->gems[i]; if (!gem->active) continue;
        gem->age++; gem->x-=UNIT;
        if (gem->x<-12*UNIT) { gem->active=false; continue; }
        if (gem->x-game->x>-17*UNIT && gem->x-game->x<17*UNIT &&
            gem->y-game->y>-15*UNIT && gem->y-game->y<15*UNIT)
        {
            gem->active=false;
            game->meter=game->meter%5+1;
            game->events|=EV_GEM; game->score+=50;
        }
    }
    for (u16 i=0;i<EFFECT_COUNT;i++) if (game->effects[i].age && ++game->effects[i].age>24) game->effects[i].age=0;
}
void game_frame(Game *game,u16 held,u16 hz)
{
    u16 pressed=held&~game->held; game->held=held; game->events=0;
    if (game->state==TITLE && (pressed&(A|START))) { game->state=HOWTO; game->age=0; game->events=EV_SELECT; return; }
    if (game->state==HOWTO && (pressed&(A|START))) { game->state=LOADOUT; game->age=0; game->events=EV_SELECT; return; }
    if (game->state==LOADOUT)
    {
        if (pressed&(UP|LEFT)) { game->loadout=(game->loadout+2)%3; game->events=EV_SELECT; }
        if (pressed&(DOWN|RIGHT)) { game->loadout=(game->loadout+1)%3; game->events=EV_SELECT; }
        if (pressed&(A|START)) { Loadout selected=game->loadout; game_begin(game,selected); return; }
    }
    if ((game->state==CREDITS || game->state==OVER) && (pressed&(A|START)))
    {
        if (game->state==OVER) { game->lives=3; game_stage_begin(game,game->stage); }
        else game_init(game);
        game->held=held; game->events=EV_START; return;
    }
    if ((game->state==PLAY || game->state==BOSS) && (pressed&START)) game->paused=!game->paused;
    if (game->paused || (hz!=50 && hz!=60)) return;
    game->clock+=60;
    while (game->clock>=hz) { game->clock-=hz; game_step(game,held,pressed); pressed=0; }
}
