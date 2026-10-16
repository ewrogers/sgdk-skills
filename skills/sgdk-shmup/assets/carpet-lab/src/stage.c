#include "game.h"
static void aimed(Game *game,s16 x,s16 y,u16 spread,u8 kind)
{
    u16 center=game_aim(game->x-x,game->y-y);
    u16 needed=spread*2+1,free=0;
    for (u16 i=0;i<BULLET_COUNT;i++) free+=!game->bullets[i].active;
    if (free<needed) return;
    for (u16 i=0;i<needed;i++)
    {
        u16 angle=(center+16-spread+i)&15;
        game_emit(game,false,x,y,(s32)direction_x[angle]*2*UNIT/256,
            (s32)direction_y[angle]*2*UNIT/256,kind,1);
    }
}
bool game_boss_guarded(const Game *game)
{
    if (!game->boss.active || game->boss.age<180) return true;
    return game->stage==1 && (game->boss.age-180)%240<60;
}
void game_encounters(Game *game)
{
    if (game->state==PLAY)
    {
        game->stage_time++;
        /* The pattern table is authored arithmetic, not an SGDK wave API. */
        if (game->stage_time>=90+game->spawn_index*50 && game->spawn_index<48)
            for (u16 i=0;i<ENEMY_COUNT;i++) if (!game->enemies[i].active)
            {
                u16 n=game->spawn_index++;
                u8 kind=(n/4+game->stage-1)%3;
                s16 y=(64+((n*37+game->stage*23)%112))*UNIT;
                game->enemies[i]=(Enemy){.x=344*UNIT,.y=y,.home=y,.hp=2+kind,.kind=kind,.active=true};
                break;
            }
        if (game->stage_time%480==360) game_gem(game,330*UNIT,(80+(game->stage_time/480*43)%100)*UNIT);
        if (game->stage_time>=STAGE_TICKS)
        {
            game->state=BOSS; game->age=0; game->events|=EV_BOSS;
            for (u16 i=0;i<ENEMY_COUNT;i++) game->enemies[i].active=false;
            for (u16 i=0;i<BULLET_COUNT;i++) game->bullets[i].active=false;
            game->boss=(Boss){.x=360*UNIT,.y=128*UNIT,.hp=BOSS_HP,.active=true};
        }
    }
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i]; if (!enemy->active) continue;
        enemy->age++; enemy->x-=enemy->kind==2?2*UNIT:UNIT;
        u16 phase=enemy->age%120;
        s16 wave=phase<60?(s16)phase:(s16)(119-phase);
        if (enemy->kind==1) enemy->y=enemy->home+(wave-30)*UNIT/2;
        if (enemy->kind==2 && enemy->age%24==0)
        { if (enemy->y<game->y) enemy->y+=4*UNIT; else enemy->y-=4*UNIT; }
        if (enemy->age%100==60) aimed(game,enemy->x-8*UNIT,enemy->y,enemy->kind==1?1:0,enemy->kind);
        if (enemy->x<-24*UNIT) enemy->active=false;
        if (enemy->x-game->x<16*UNIT && enemy->x-game->x>-16*UNIT &&
            enemy->y-game->y<14*UNIT && enemy->y-game->y>-14*UNIT) game_hurt(game);
    }
    Boss *boss=&game->boss; if (!boss->active) return;
    boss->age++;
    if (boss->age<90) return;
    if (boss->age<180) { boss->x-=UNIT; return; }
    u16 time=(boss->age-180)%720,phrase=time/240,phase=time%240;
    if (phase==0 && boss->hp<=BOSS_HP/2) boss->phase=1;
    u16 motion=time%180;
    s16 triangle=motion<90?(s16)motion:(s16)(179-motion);
    boss->y=(80+triangle)*UNIT;
    s16 target_x=(game->stage==3 && phase>=100 && phase<160)?170*UNIT:270*UNIT;
    if (boss->x<target_x) boss->x+=UNIT; else if (boss->x>target_x) boss->x-=UNIT;
    /* Each guardian has three telegraphed, player-relative attack phrases. */
    if (!game_boss_guarded(game) && phase>=60 && phase<180)
    {
        u16 cadence=boss->phase?24:36;
        if (phase%cadence==0) aimed(game,boss->x-24*UNIT,boss->y,phrase==0?1:phrase==1?2:0,(game->stage+phrase)%3);
        if (game->stage==2 && phrase==2 && phase%48==0)
            for (u16 i=0;i<8;i++)
                game_emit(game,false,boss->x,boss->y,(s32)direction_x[i*2]*UNIT/256,
                    (s32)direction_y[i*2]*UNIT/256,1,1);
    }
    if (boss->x-game->x<30*UNIT && boss->x-game->x>-30*UNIT &&
        boss->y-game->y<28*UNIT && boss->y-game->y>-28*UNIT) game_hurt(game);
}
