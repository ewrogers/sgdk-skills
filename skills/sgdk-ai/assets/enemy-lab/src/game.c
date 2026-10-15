#include "world.h"

void game_load_level(Game *game,u16 level)
{
    if (level>=LEVEL_COUNT) return;
    u16 tries=game->tries,hits[2]={game->players[0].hits,game->players[1].hits};
    bool second=game->players[1].active; u8 overlay=game->overlay;
    *game=(Game){0}; game->tries=tries; game->overlay=overlay; game->level=level;
    game->players[0].active=true; game->players[1].active=second;
    static const Point spawns[LEVEL_COUNT][2]={{{32,192},{48,192}},{{288,192},{272,192}},{{32,192},{48,192}}};
    static const Point homes[LEVEL_COUNT][ENEMY_COUNT]={
        {{48,80},{256,176},{144,80},{256,96},{128,176},{240,144}},
        {{48,96},{256,176},{144,80},{208,176},{112,112},{240,112}},
        {{64,80},{256,176},{128,80},{208,96},{112,176},{272,144}}
    };
    static const Point patrols[LEVEL_COUNT][ENEMY_COUNT]={
        {{80,80},{224,176},{176,80},{256,64},{128,176},{240,144}},
        {{48,144},{224,176},{176,80},{256,176},{112,112},{240,112}},
        {{64,128},{224,176},{160,80},{208,128},{112,176},{272,144}}
    };
    for (u16 p=0;p<2;++p) {
        game->players[p].pos=spawns[level][p]; game->players[p].hits=hits[p];
        game->players[p].invulnerable=120;
    }
    for (u16 i=0;i<ENEMY_COUNT;++i) {
        Enemy *enemy=&game->enemies[i]; enemy->kind=i/2;
        enemy->pos=enemy->home=homes[level][i]; enemy->patrol=patrols[level][i];
        enemy->state=enemy->kind==SENTRY ? IDLE : PATROL;
        enemy->facing=i&1 ? 3 : 1; enemy->outbound=true; enemy->serial=1;
    }
}
void game_init(Game *game)
{
    *game=(Game){0}; game->tries=3; game_load_level(game,0);
}
u16 game_clock(u16 *fraction,u16 hz)
{
    if (!fraction || (hz!=50 && hz!=60)) return 0;
    *fraction+=60; u16 ticks=*fraction/hz; *fraction%=hz; return ticks;
}
void game_input(Input *input,u16 previous[2],const u16 held[2])
{
    for (u16 p=0;p<2;++p) { input->held[p]=held[p]; input->pressed[p]=held[p]&~previous[p]; previous[p]=held[p]; }
}
static void move_player(Game *game,Player *player,u16 keys)
{
    s16 dx=2*(((keys&RIGHT)!=0)-((keys&LEFT)!=0)),dy=2*(((keys&DOWN)!=0)-((keys&UP)!=0));
    if (dx) dy=0; /* Four-way movement, with no diagonal speed boost. */
    bool moving=world_move(game->level,&player->pos,dx,dy);
    u8 facing=dx ? (dx>0 ? 1 : 3) : dy ? (dy>0 ? 0 : 2) : player->facing;
    if (moving!=player->moving || facing!=player->facing) player->animation=0;
    else player->animation=(player->animation+1)%(moving ? 32 : 96);
    player->facing=facing; player->flip=facing==3; player->moving=moving;
}
static void pulse(Game *game,u16 p)
{
    Player *player=&game->players[p];
    if (!player->active || player->cooldown) return;
    player->pulse=16; player->cooldown=30;
    for (u16 i=0;i<ENEMY_COUNT;++i) {
        Enemy *enemy=&game->enemies[i]; s16 dx=enemy->pos.x-player->pos.x,dy=enemy->pos.y-player->pos.y;
        if ((long)dx*dx+(long)dy*dy<=32*32 && world_line(game->level,player->pos,enemy->pos)) {
            enemy_change(enemy,STUNNED,90); game->events|=EVENT_STUN;
        }
    }
}
void game_frame(Game *game,const Input *input,u16 ticks)
{
    game->events=game->sensors=0; game->nav.expanded=0;
    if (!input || ticks>2) return;
    if (input->pressed[0]&B) { game_init(game); return; }
    if (game->phase==GAME_OVER || game->phase==COMPLETE) {
        if (input->pressed[0]&START) game_init(game);
        return;
    }
    if (game->phase!=PLAYING) {
        game->phase_ticks=ticks>=game->phase_ticks ? 0 : game->phase_ticks-ticks;
        if (!game->phase_ticks) {
            if (game->phase==CAUGHT) {
                if (!game->tries) game->phase=GAME_OVER;
                else game_load_level(game,game->level);
            } else if (game->level+1<LEVEL_COUNT) game_load_level(game,game->level+1);
            else game->phase=COMPLETE;
        }
        return;
    }
    if (input->pressed[0]&C) game->overlay=(game->overlay+1)%3;
    if (input->pressed[1]&START && !game->players[1].active) {
        game->players[1].active=true; game->events|=EVENT_JOIN;
    }
    if (input->pressed[0]&START) game->paused=!game->paused;
    if (game->paused) return;
    for (u16 p=0;p<2;++p) if (input->pressed[p]&A) pulse(game,p);
    while (ticks--) {
        for (u16 p=0;p<2;++p) {
            Player *player=&game->players[p]; if (!player->active) continue;
            if (player->invulnerable) --player->invulnerable;
            if (player->pulse) --player->pulse;
            if (player->cooldown) --player->cooldown;
            move_player(game,player,input->held[p]);
        }
        for (u16 i=0;i<ENEMY_COUNT;++i) {
            if (game->tick%ENEMY_COUNT==i) enemy_sense(game,i);
            enemy_tick(game,i);
            if (game->phase!=PLAYING) break;
        }
        if (game->phase==PLAYING) shots_tick(game);
        ++game->tick;
        if (game->phase!=PLAYING) return;
        Point exit=world_exit(game->level);
        for (u16 p=0;p<2;++p) if (game->players[p].active) {
            s16 dx=game->players[p].pos.x-exit.x,dy=game->players[p].pos.y-exit.y;
            if (dx>-12 && dx<12 && dy>-12 && dy<12) {
                game->phase=AREA_CLEAR; game->phase_ticks=60; return;
            }
        }
    }
    nav_frame(game,NAV_BUDGET);
}
