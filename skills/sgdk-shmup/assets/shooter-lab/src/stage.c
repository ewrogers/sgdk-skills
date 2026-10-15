#include "game.h"

const Spawn stage_spawns[24]={
    {60,48,SCOUT},{100,108,SCOUT},{140,168,SCOUT},{180,208,SCOUT},
    {300,64,SCOUT},{330,184,SCOUT},{390,32,SCOUT},{420,208,SCOUT},
    {540,40,SWEEPER},{590,200,SWEEPER},{640,80,SCOUT},{690,160,SCOUT},
    {810,72,TURRET},{900,168,TURRET},{990,40,SWEEPER},{1050,200,SWEEPER},
    {1170,40,SCOUT},{1200,200,SCOUT},{1260,80,SWEEPER},{1320,160,SWEEPER},
    {1440,72,TURRET},{1530,168,TURRET},{1620,48,SCOUT},{1660,192,SCOUT}
};

void game_stage(Game *game)
{
    if (game->state!=PLAY) return;
    if (game->spawn_index<24 && game->ticks>=stage_spawns[game->spawn_index].at)
        for (u16 i=0;i<ENEMY_COUNT;i++)
            if (!game->enemies[i].active)
            {
                const Spawn *spawn=&stage_spawns[game->spawn_index];
                s16 x=game->stage==2?FIELD_W-spawn->x:spawn->x;
                EnemyKind kind=game->stage==2?(EnemyKind)((spawn->kind+1)%3):spawn->kind;
                game->enemies[i]=(Enemy){.x=x*UNIT,.home=x*UNIT,.y=-12*UNIT,
                    .hp=kind==TURRET?5:kind==SWEEPER?3:2,.kind=kind,.active=true};
                game->wave=1+game->spawn_index/4; game->spawn_index++;
                break;
            }
    if (game->spawn_index==24 && game->ticks>=1980)
    {
        game->state=BOSS; game->events|=EVENT_BOSS; game->phase_time=0;
        for (u16 i=0;i<ENEMY_COUNT;i++) game->enemies[i].active=false;
        for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
        game->boss=(Boss){.x=120*UNIT,.y=-24*UNIT,.hp=BOSS_HEALTH,.active=true};
    }
}

void game_enemies(Game *game)
{
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if (!enemy->active) continue;
        enemy->age++;
        if (enemy->flash) enemy->flash--;
        if (enemy->kind==SCOUT)
        {
            enemy->y+=UNIT;
            u16 phase=(enemy->age/2)%64;
            enemy->x=enemy->home+((s16)(phase<32?phase:63-phase)-16)*UNIT;
        }
        else if (enemy->kind==SWEEPER)
        {
            enemy->y+=UNIT/2;
            u16 phase=enemy->age%160;
            s16 offset=(phase<80?phase:159-phase)-40;
            enemy->x=enemy->home+offset*UNIT;
        }
        else if (enemy->age<64) enemy->y+=UNIT;
        else if (enemy->age>260) enemy->y+=UNIT;
        if ((enemy->age+i*11)%90==60)
            game_pattern(game,enemy->kind==SCOUT?AIMED:enemy->kind==SWEEPER?FAN:RING,
                enemy->x,enemy->y+8*UNIT,game->x,game->y,enemy->age/90);
        if (enemy->y>(FIELD_H+16)*UNIT)
        { enemy->active=false; game->combo=0; }
        if (enemy->x-game->x<10*UNIT && enemy->x-game->x>-10*UNIT &&
            enemy->y-game->y<10*UNIT && enemy->y-game->y>-10*UNIT) game_hurt(game);
    }
    Boss *boss=&game->boss;
    if (!boss->active) return;
    if (boss->age==BOSS_READY_TICKS+720) boss->age=BOSS_READY_TICKS;
    boss->age++; if (boss->flash) boss->flash--;
    /* Warning, fly-in, then health-bar reveal. No hostile contact or shots
     * until the showcase finishes; the player can keep moving throughout. */
    if (boss->age<BOSS_WARNING_TICKS) return;
    if (boss->age<BOSS_WARNING_TICKS+BOSS_ENTRY_TICKS)
    { boss->y+=3*UNIT/4; return; }
    if (boss->age<BOSS_READY_TICKS) return;
    u16 time=(boss->age-BOSS_READY_TICKS)%720;
    u16 phase=time%240;
    u16 motion=(time+60)%240;
    s16 travel=motion<120?motion:239-motion;
    if (boss->hp<=BOSS_HEALTH/2)
    {
        boss->x=(30+travel*3/2)*UNIT;
        s16 dive=phase>=90 && phase<150?(phase<120?phase-90:150-phase):0;
        boss->y=(44+dive*2)*UNIT;
    }
    else { boss->x=(60+travel)*UNIT; boss->y=44*UNIT; }
    if (game->stage==2)
    {
        u16 cross=(time*2+60)%240;
        s16 sweep=cross<120?cross:239-cross;
        u16 vertical=time%180;
        boss->x=(24+sweep*8/5)*UNIT;
        boss->y=(40+(s16)(vertical<90?vertical:179-vertical)/2)*UNIT;
    }
    /* Three readable four-second phrases. Each has a quiet wind-up and
     * recovery; the second half tightens the firing cadence, not the pool. */
    u16 cadence=boss->hp>BOSS_HEALTH/2?40:30;
    if (!game_boss_guarded(game) && phase>=45 && phase<180)
    {
        if (time<240 && phase%cadence==0)
            game_pattern(game,FAN,boss->x,boss->y+16*UNIT,game->x,game->y,0);
        else if (time>=240 && time<480 && phase%60==0)
            game_pattern(game,RING,boss->x,boss->y,game->x,game->y,phase/60);
        else if (time>=480 && phase%12==0)
            game_pattern(game,SPIRAL,boss->x,boss->y,game->x,game->y,phase/12);
    }
    if (boss->x-game->x<18*UNIT && boss->x-game->x>-18*UNIT &&
        boss->y-game->y<18*UNIT && boss->y-game->y>-18*UNIT) game_hurt(game);
}

bool game_boss_guarded(const Game *game)
{
    if (game->stage!=2 || !game->boss.active || game->boss.age<BOSS_READY_TICKS) return false;
    u16 phase=(game->boss.age-BOSS_READY_TICKS)%180;
    return phase<(game->boss.hp<=BOSS_HEALTH/2?75:45);
}
