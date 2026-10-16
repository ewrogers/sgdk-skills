#include "game.h"

typedef struct {int16_t left,top,right,bottom;} Rect;
static bool inside(int16_t x,int16_t y,Rect r){return x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom;}

static bool point_walkable(uint8_t zone,int16_t x,int16_t y)
{
    if(zone>3||x<8||x>312||y<8||y>184)return false;
    if(zone==0)
        return (inside(x,y,(Rect){12,72,308,126})||inside(x,y,(Rect){115,8,153,184}))
            &&!inside(x,y,(Rect){192,80,224,110});
    if(zone==1)
        return inside(x,y,(Rect){28,65,292,144})&&!inside(x,y,(Rect){221,73,257,112});
    if(zone==2)
    {
        /* A broad capsule follows the painted trail; no generated-color test
         * silently turns a decorative flower or shadow into collision. */
        const int16_t line=45+(y*11)/10;
        return y>=14&&y<=180&&x>=line-26&&x<=line+28;
    }
    return (inside(x,y,(Rect){24,57,205,148})||inside(x,y,(Rect){202,96,251,132}))
        &&!inside(x,y,(Rect){192,82,210,98});
}

bool field_walkable(uint8_t zone,int16_t x,int16_t y)
{
    return point_walkable(zone,x-4,y-3)&&point_walkable(zone,x+4,y-3)
        &&point_walkable(zone,x-4,y)&&point_walkable(zone,x+4,y);
}

void field_enter(Game *g)
{
    g->hero=(Actor){g->zone==2?150:144,96,0,0,false};
    if(g->zone==0){g->npc[0]=(Actor){64,100,1,0,false};g->npc[1]=(Actor){272,116,3,0,false};}
    else if(g->zone==1){g->npc[0]=(Actor){96,92,1,0,false};g->npc[1]=(Actor){180,124,3,0,false};}
    else if(g->zone==2){g->npc[0]=(Actor){116,65,0,0,false};g->npc[1]=(Actor){208,144,2,0,false};}
    else {g->npc[0]=(Actor){80,100,1,0,false};g->npc[1]=(Actor){228,116,3,0,false};}
}

static void move_actor(Actor *a,uint8_t zone,int16_t dx,int16_t dy)
{
    const int16_t old_x=a->x,old_y=a->y;
    if(dx)a->direction=dx>0?1:3;
    else if(dy)a->direction=dy>0?0:2;
    if(field_walkable(zone,a->x+dx,a->y))a->x+=dx;
    if(field_walkable(zone,a->x,a->y+dy))a->y+=dy;
    a->moving=a->x!=old_x||a->y!=old_y;
}

void field_step(Game *g,uint16_t held)
{
    int16_t dx=((held&RIGHT)?1:0)-((held&LEFT)?1:0);
    int16_t dy=((held&DOWN)?1:0)-((held&UP)?1:0);
    move_actor(&g->hero,g->zone,dx,dy);
    g->hero.frame=g->hero.moving?(g->ticks/9)%4:0;
    for(uint16_t n=0;n<2;n++)
    {
        Actor *a=&g->npc[n];
        /* Quiet, bounded wandering. Each NPC pauses before changing direction. */
        uint16_t phase=(g->ticks+n*93)%360;
        uint16_t direction=(g->ticks/360+n*2)%4;
        int16_t nx=0,ny=0;
        if(phase<44&&(g->ticks&1)==0)
        {if(direction==0)ny=1;else if(direction==1)nx=1;else if(direction==2)ny=-1;else nx=-1;}
        if((g->ticks&1)==0)move_actor(a,g->zone,nx,ny);
        if(phase>=44)a->moving=false;
        a->frame=a->moving?(g->ticks/12)%4:0;
    }
}

const char *zone_name(uint8_t zone)
{
    static const char *names[]={"Willowmere","Old Lantern Crypt","Mossbell Wood","Tideglass Quay"};
    return names[zone<4?zone:0];
}
const char *zone_subtitle(uint8_t zone)
{
    static const char *names[]={"A home beneath the red roofs","Where the old lantern sleeps","Follow the golden trail","Every voyage begins here"};
    return names[zone<4?zone:0];
}
