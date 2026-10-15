#include "world.h"

void player_hit(Game *game,u16 index)
{
    Player *player=&game->players[index];
    if (game->phase!=PLAYING || !player->active || player->invulnerable) return;
    player->invulnerable=90; if (player->hits<999) ++player->hits;
    if (game->tries) --game->tries;
    game->phase=CAUGHT; game->phase_ticks=60;
    game->events|=EVENT_HIT;
}
bool shot_fire(Game *game,Point from,u8 direction)
{
    if (direction>=8) return false;
    for (u16 i=0;i<SHOT_COUNT;++i) if (!game->shots[i].active) {
        game->shots[i]=(Shot){(from.x+4)*8,(from.y+4)*8,90,direction,true}; return true;
    }
    return false; /* Optional attack is skipped when all slots are occupied. */
}
void shots_tick(Game *game)
{
    static const s16 dx[]={24,17,0,-17,-24,-17,0,17},dy[]={0,17,24,17,0,-17,-24,-17};
    for (u16 i=0;i<SHOT_COUNT;++i) {
        Shot *shot=&game->shots[i]; if (!shot->active) continue;
        shot->x+=dx[shot->direction]; shot->y+=dy[shot->direction];
        s16 x=shot->x/8,y=shot->y/8;
        if (!--shot->life || world_solid(game->level,x,y,8,8)) { shot->active=false; continue; }
        for (u16 p=0;p<2;++p) {
            Player *player=&game->players[p];
            if (player->active && x<player->pos.x+ACTOR_INSET+ACTOR_SIZE && player->pos.x+ACTOR_INSET<x+8 &&
                y<player->pos.y+ACTOR_INSET+ACTOR_SIZE && player->pos.y+ACTOR_INSET<y+8) {
                player_hit(game,p); shot->active=false; break;
            }
        }
    }
}
