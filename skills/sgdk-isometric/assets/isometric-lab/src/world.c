#include "iso.h"
#define U(n) ((n)*IL_UNIT)
/* Foot anchors are in map coordinates; z is 16 raw units per vertical pixel. */
const IlProp il_props[IL_STATIC]={
    {896,1408,512,IL_DECK},{1152,1408,512,IL_DECK},{1408,1408,512,IL_DECK},{1664,1408,512,IL_DECK},
    {832,1312,0,IL_POST},{1728,1312,0,IL_POST},{832,1504,0,IL_POST},{1728,1504,0,IL_POST},
    {640,1408,0,IL_RAMP_L},{1920,1408,0,IL_RAMP_R},
    {384,384,0,IL_TOWER},{2176,384,0,IL_TOWER},
    {384,2048,0,IL_TREE},{2176,2048,0,IL_TREE},{1664,2176,0,IL_FOUNTAIN}
};
IlBox il_prop_box(u16 i)
{
    if(i>=IL_STATIC)return (IlBox){0};
    IlProp p=il_props[i];s16 r=128,low=p.z,high=low+256;
    if(p.kind==IL_DECK){low=448;high=512;}
    if(p.kind==IL_POST){r=64;high=448;}
    if(p.kind==IL_RAMP_L || p.kind==IL_RAMP_R)high=512;
    if(p.kind==IL_TOWER)high=768;
    if(p.kind==IL_TREE)high=512;
    if(p.kind==IL_FOUNTAIN){r=128;high=320;}
    return (IlBox){p.x-r,p.y-r,low,p.x+r,p.y+r,high};
}
static bool overlaps(s16 x,s16 y,IlBox b)
{
    return x-40<b.x1 && x+40>b.x0 && y-40<b.y1 && y+40>b.y0;
}
static bool try_move(IlGame *g,s16 x,s16 y)
{
    if(x<64 || y<64 || x>U(10)-64 || y>U(10)-64)return false;
    u16 layer=g->layer;bool lane=y>=U(5)+40 && y<=U(6)-40;
    if(layer) {
        if(!lane)return false;
        if(x<U(2) || x>=U(8))layer=0;
    } else {
        bool on_ramp=(x>=U(2)-40 && x<U(3)+40) || (x>=U(7)-40 && x<U(8)+40);
        if(y+40>U(5) && y-40<U(6) && on_ramp) {
            bool left=g->x<U(2) && x<U(3) && ((x<U(2) && x<=g->x) || lane);
            bool right=g->x>=U(8) && x>=U(7) && ((x>=U(8) && x>=g->x) || lane);
            if(left || right) {if(x>=U(2) && x<U(8))layer=1;}
            else return false;
        }
    }
    for(u16 i=0;i<IL_STATIC;i++) {
        u16 k=il_props[i].kind;
        if(k==IL_DECK || k==IL_RAMP_L || k==IL_RAMP_R)continue;
        IlBox b=il_prop_box(i);
        if(layer && b.z1<=512)continue;
        if(overlaps(x,y,b))return false;
    }
    g->x=x;g->y=y;g->layer=layer;g->z=0;
    if(layer) {
        if(x<U(3))g->z=(s16)(((x-U(2))/64+1)*128);
        else if(x>=U(7))g->z=(s16)(((U(8)-1-x)/64+1)*128);
        else g->z=512;
    }
    return true;
}
static void camera(IlGame *g)
{
    IlPoint p=il_project(g->x,g->y,g->z);
    g->camera_x=256+p.x-160;g->camera_y=64+p.y-112;
    if(g->camera_x<64)g->camera_x=64;
    if(g->camera_x>128)g->camera_x=128;
    if(g->camera_y<0)g->camera_y=0;
    if(g->camera_y>64)g->camera_y=64;
}
void il_tour(IlGame *g,u16 stop)
{
    static const s16 points[4][3]={{384,1152,0},{640,1408,1},{1280,1408,1},{1280,1152,0}};
    stop%=4;g->x=points[stop][0];g->y=points[stop][1];g->layer=(u16)points[stop][2];g->z=g->layer ? (stop==1 ? 384 : 512) : 0;
    g->walk=0;g->moving=false;camera(g);
}
void il_init(IlGame *g){*g=(IlGame){0};il_tour(g,0);}
void il_tick(IlGame *g,u16 held)
{
    s16 sx=((held&IL_RIGHT)!=0)-((held&IL_LEFT)!=0),sy=((held&IL_DOWN)!=0)-((held&IL_UP)!=0);
    s16 dx=16*sx+16*sy,dy=-16*sx+16*sy;
    if(sx && sy){dx=dx*3/4;dy=dy*3/4;}
    s16 oldx=g->x,oldy=g->y;
    if(dx)try_move(g,g->x+dx,g->y);
    if(dy)try_move(g,g->x,g->y+dy);
    g->moving=oldx!=g->x || oldy!=g->y;
    if(sx || sy)g->facing=sy<0 ? 0 : sy>0 ? 2 : sx>0 ? 1 : 3;
    if(g->moving)g->walk=(g->walk+1)%32;else g->walk=0;
    g->ticks++;camera(g);
}
u16 il_frame(IlGame *g,u16 held,u16 hz,bool paused)
{
    if(paused || (hz!=50 && hz!=60))return 0;
    g->phase+=60;u16 n=0;while(g->phase>=hz){g->phase-=hz;il_tick(g,held);n++;}return n;
}
u16 il_scene(const IlGame *g,IlItem *items,IlLink *links,u16 *link_count)
{
    for(u16 i=0;i<IL_STATIC;i++)items[i]=(IlItem){il_prop_box(i),i};
    items[IL_STATIC]=(IlItem){{g->x-40,g->y-40,g->z,g->x+40,g->y+40,g->z+384},IL_STATIC};
    *link_count=0;
    /* The surface owner resolves sloped/stepped-volume proxy overlap, including
       the footprint margin at the two stair feet. This is not a generic tie. */
    for(u16 ramp=8;ramp<=9;ramp++)if(overlaps(g->x,g->y,items[ramp].box))
        links[(*link_count)++]=(IlLink){ramp,IL_STATIC};
    return IL_ITEMS;
}
