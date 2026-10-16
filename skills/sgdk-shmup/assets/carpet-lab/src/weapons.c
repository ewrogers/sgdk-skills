#include "game.h"

const s16 direction_x[16]={0,98,181,237,256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98};
const s16 direction_y[16]={-256,-237,-181,-98,0,98,181,237,256,237,181,98,0,-98,-181,-237};
u16 game_aim(s16 dx,s16 dy)
{
    s32 best=-2147483647L; u16 angle=0;
    for (u16 i=0;i<16;i++)
    { s32 dot=(s32)dx*direction_x[i]+(s32)dy*direction_y[i]; if (dot>best) { best=dot; angle=i; } }
    return angle;
}
bool game_emit(Game *game,bool player,s16 x,s16 y,s16 vx,s16 vy,u8 kind,u8 damage)
{
    Shot *pool=player?game->shots:game->bullets; u16 count=player?SHOT_COUNT:BULLET_COUNT;
    if (vx<-8*UNIT || vx>8*UNIT || vy<-8*UNIT || vy>8*UNIT) return false;
    for (u16 i=0;i<count;i++) if (!pool[i].active)
    { pool[i]=(Shot){.x=x,.y=y,.vx=vx,.vy=vy,.kind=kind,.damage=damage,.active=true}; return true; }
    return false;
}
void game_familiar(const Game *game,u16 index,s16 *x,s16 *y)
{
    u16 at=(game->history_head+32-(index+1)*8)&31;
    *x=game->history[at].x-16*UNIT;
    *y=game->history[at].y+(index?8:-8)*UNIT;
}
void game_player(Game *game,u16 held,bool shooting)
{
    s16 dx=((held&RIGHT)!=0)-((held&LEFT)!=0),dy=((held&DOWN)!=0)-((held&UP)!=0);
    s16 speed=(held&C)?3*UNIT/2:2*UNIT+game->speed*UNIT/4;
    if (dx && dy) speed=speed*181/256;
    game->x+=dx*speed; game->y+=dy*speed;
    if (game->x<16*UNIT) game->x=16*UNIT;
    if (game->x>304*UNIT) game->x=304*UNIT;
    if (game->y<(HUD_HEIGHT+16)*UNIT) game->y=(HUD_HEIGHT+16)*UNIT;
    if (game->y>192*UNIT) game->y=192*UNIT;
    game->history_head=(game->history_head+1)&31;
    game->history[game->history_head]=(Point){game->x,game->y};
    if (game->fire_wait) game->fire_wait--;
    if (game->ribbon_wait) game->ribbon_wait--;
    bool was_ribbon=game->ribbon_active;
    game->ribbon_active=shooting && (held&A) && game->loadout==MOONWEAVE && (game->weapon&2);
    if (game->ribbon_active && !was_ribbon) game->events|=EV_RIBBON;
    if (!shooting || !(held&A) || game->fire_wait) return;
    bool fired=false; u8 kind=game->loadout+((game->weapon&2)?3:0);
    s16 vx=game->loadout==MOONWEAVE?8*UNIT:6*UNIT;
    u8 damage=(game->weapon&2)?2:1;
    if (game->ribbon_active)
    {
        if (game->weapon&1)
        {
            fired=game_emit(game,true,game->x+12*UNIT,game->y-10*UNIT,vx,0,1,1);
            fired=game_emit(game,true,game->x+12*UNIT,game->y+10*UNIT,vx,0,1,1)||fired;
        }
    }
    else
    {
        fired=game_emit(game,true,game->x+12*UNIT,game->y,vx,0,kind,damage);
        if (game->weapon&1)
            fired=game_emit(game,true,game->x+10*UNIT,game->y-12*UNIT,vx,game->loadout==MOONWEAVE?0:-UNIT,kind,damage)||fired;
        if ((game->weapon&2) && game->loadout==SUNFIRE)
        {
            fired=game_emit(game,true,game->x+10*UNIT,game->y-5*UNIT,vx,-UNIT,kind,1)||fired;
            fired=game_emit(game,true,game->x+10*UNIT,game->y+5*UNIT,vx,UNIT,kind,1)||fired;
        }
    }
    if (!(game->ticks&16)) for (u16 i=0;i<game->familiars;i++)
    {
        s16 x,y; game_familiar(game,i,&x,&y);
        fired=game_emit(game,true,x+8*UNIT,y,5*UNIT,0,game->loadout,1)||fired;
    }
    if (fired) game->events|=EV_FIRE;
    game->fire_wait=10;
}
