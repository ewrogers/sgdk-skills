#include "demo.h"
#define P(n) ((n)*CC_PIXEL)
const CcBox cd_walls[8]={
    {P(16),P(64),P(8),P(128)},{P(296),P(64),P(8),P(128)},
    {P(24),P(64),P(144),P(8)},{P(168),P(64),P(128),P(8)},
    {P(24),P(184),P(144),P(8)},{P(168),P(184),P(128),P(8)},
    {P(128),P(88),P(16),P(64)},{P(192),P(136),P(56),P(16)}
};
void cd_fire(CcDemo *d)
{
    for(u16 i=0;i<2;i++){d->shot[i]=(CcVec){P(40),P(92)+P(56)*i};d->alive[i]=true;d->flash[i]=0;}
    d->shots++;d->tick=0;
}
void cd_init(CcDemo *d,u16 page)
{
    *d=(CcDemo){0};d->page=page%3;d->player=(CcBox){P(48),P(112),P(12),P(12)};
    d->aim=(CcVec){P(280),P(100)};d->fast=true;d->target_x=P(184);d->target_dx=P(1);
    cd_fire(d);
}
void cd_tick(CcDemo *d,u16 held)
{
    d->tick++;
    if(!d->page) {
        s16 dx=d->moving ? d->target_dx : 0;
        if(d->target_x+dx>P(240) || d->target_x+dx<P(128)){d->target_dx=-d->target_dx;dx=d->moving ? d->target_dx : 0;}
        for(u16 i=0;i<2;i++) {
            if(d->flash[i])d->flash[i]--;
            if(!d->alive[i])continue;
            CcBox shot={d->shot[i].x,d->shot[i].y,P(4),P(4)};
            CcBox wall={d->target_x,P(80)+P(56)*i,P(4),P(32)};
            CcVec velocity={P(d->fast ? 40 : 4),0};
            CcHit h=cc_sweep(shot,velocity,wall,(CcVec){dx,0});
            bool hit=i ? h.kind==CC_HIT || h.kind==CC_OVERLAP :
                cc_overlap((CcBox){shot.x+velocity.x,shot.y,shot.w,shot.h},(CcBox){wall.x+dx,wall.y,wall.w,wall.h});
            if(hit) {
                if(i && h.kind==CC_HIT)d->shot[i].x+=(s16)((s32)velocity.x*h.num/h.den);
                else d->shot[i].x+=velocity.x;
                d->alive[i]=false;d->flash[i]=30;if(i)d->hits++;
            } else {d->shot[i].x+=velocity.x;if(d->shot[i].x>P(304)){d->alive[i]=false;if(!i)d->misses++;}}
        }
        d->target_x+=dx;
        if(d->tick>=80)cd_fire(d);
    } else if(d->page==1) {
        d->aim.x+=P(2)*((held&CD_RIGHT)!=0)-P(2)*((held&CD_LEFT)!=0);
        d->aim.y+=P(2)*((held&CD_DOWN)!=0)-P(2)*((held&CD_UP)!=0);
        if(d->aim.x<P(40))d->aim.x=P(40);
        if(d->aim.x>P(288))d->aim.x=P(288);
        if(d->aim.y<P(76))d->aim.y=P(76);
        if(d->aim.y>P(176))d->aim.y=P(176);
        CcVec start={P(48),P(112)},delta={d->aim.x-start.x,d->aim.y-start.y};
        d->ray=(CcHit){0};
        for(u16 i=0;i<8;i++) {
            CcHit h=cc_ray(start,delta,cd_walls[i]);
            if(h.kind==CC_HIT && (d->ray.kind!=CC_HIT || h.num*d->ray.den<d->ray.num*h.den)){d->ray=h;d->ray.index=i;}
        }
    } else {
        CcVec delta={P(3)*((held&CD_RIGHT)!=0)-P(3)*((held&CD_LEFT)!=0),
            P(3)*((held&CD_DOWN)!=0)-P(3)*((held&CD_UP)!=0)};
        d->move=cc_slide(d->player,delta,cd_walls,8);d->player=d->move.box;
    }
}
u16 cd_frame(CcDemo *d,u16 held,u16 hz,bool paused)
{
    if(paused || (hz!=50 && hz!=60))return 0;
    d->phase+=60;u16 steps=0;
    while(d->phase>=hz){d->phase-=hz;cd_tick(d,held);steps++;}
    return steps;
}
