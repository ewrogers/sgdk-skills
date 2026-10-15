#include "game.h"

/* Unit vectors scaled by 256, clockwise from up, sampled at 22.5 degrees.
 * These bounded constants avoid per-bullet trig, division or normalization. */
const s16 direction_x[16]={0,98,181,237,256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98};
const s16 direction_y[16]={-256,-237,-181,-98,0,98,181,237,256,237,181,98,0,-98,-181,-237};

u16 game_aim(s16 dx,s16 dy)
{
    if (!dx && !dy) return 8;
    s32 best=-2147483647L;
    u16 result=0;
    for (u16 i=0;i<16;i++)
    {
        s32 dot=(s32)dx*direction_x[i]+(s32)dy*direction_y[i];
        if (dot>best) { best=dot; result=i; }
    }
    return result;
}

bool game_bullet(Game *game,s16 x,s16 y,u16 angle,s16 speed)
{
    if (speed<0 || speed>4*UNIT || angle>=16) return false;
    for (u16 i=0;i<BULLET_COUNT;i++)
        if (!game->bullets[i].active)
        {
            game->bullets[i]=(Bullet){.x=x,.y=y,
                .vx=(s16)((s32)direction_x[angle]*speed/256),
                .vy=(s16)((s32)direction_y[angle]*speed/256),.active=true};
            return true;
        }
    game->denied++;
    return false;
}

bool game_shot(Game *game,s16 x,s16 y,s16 vx,s16 vy)
{
    if (vx<-6*UNIT || vx>6*UNIT || vy<-6*UNIT || vy>6*UNIT) return false;
    for (u16 i=0;i<SHOT_COUNT;i++)
        if (!game->shots[i].active)
        { game->shots[i]=(Bullet){.x=x,.y=y,.vx=vx,.vy=vy,.active=true}; return true; }
    return false;
}

u16 game_pattern(Game *game,Pattern pattern,s16 x,s16 y,s16 target_x,s16 target_y,u16 phase)
{
    u16 angle=(pattern==AIMED || pattern==FAN)?game_aim(target_x-x,target_y-y):0,count=0;
    if (pattern==AIMED) return game_bullet(game,x,y,angle,2*UNIT);
    u16 needed=pattern==RING?8:pattern==FAN?3:1;
    u16 free=0;
    for (u16 i=0;i<BULLET_COUNT;i++) if (!game->bullets[i].active) free++;
    /* Accept an entire geometric volley or skip it. A full pool must not
     * silently turn a symmetric ring into an arbitrary one-sided attack. */
    if (free<needed) { game->denied++; return 0; }
    for (u16 i=0;i<needed;i++)
    {
        u16 direction=pattern==RING?(phase+i*2)%16:pattern==FAN?(angle+15+i)%16:phase%16;
        count+=game_bullet(game,x,y,direction,pattern==RING?3*UNIT/2:2*UNIT);
    }
    return count;
}
