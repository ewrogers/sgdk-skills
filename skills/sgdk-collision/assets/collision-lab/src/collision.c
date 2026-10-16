#include "collision.h"

typedef struct {s32 n,d;} Ratio;
static CcHit result(u16 kind)
{
    CcHit h={kind,0,65535,0,0,0,1};return h;
}
static bool less(Ratio a,Ratio b){return a.n*b.d < b.n*a.d;}
static bool equal(Ratio a,Ratio b){return a.n*b.d == b.n*a.d;}
static bool position(s32 p){return p>=-CC_LIMIT && p<=CC_LIMIT;}
static bool valid(CcBox a,bool point)
{
    return position(a.x) && position(a.y) && a.w>=(point ? 0 : 1) &&
        a.h>=(point ? 0 : 1) && a.w<=CC_EXTENT && a.h<=CC_EXTENT;
}
static bool motion(CcBox a,CcVec d)
{
    return d.x>=-CC_MOTION && d.x<=CC_MOTION && d.y>=-CC_MOTION && d.y<=CC_MOTION &&
        position((s32)a.x+d.x) && position((s32)a.y+d.y);
}
bool cc_overlap(CcBox a,CcBox b)
{
    return valid(a,false) && valid(b,false) && a.x<(s32)b.x+b.w &&
        (s32)a.x+a.w>b.x && a.y<(s32)b.y+b.h && (s32)a.y+a.h>b.y;
}
static bool inside(CcBox a,CcBox b)
{
    return a.x<(s32)b.x+b.w && (s32)a.x+a.w>b.x &&
        a.y<(s32)b.y+b.h && (s32)a.y+a.h>b.y;
}
/* Test relative swept bounds before divisions. Includes end-face contact. */
static bool broad(CcBox a,s32 dx,s32 dy,CcBox b)
{
    s32 x0=a.x+(dx<0 ? dx : 0),y0=a.y+(dy<0 ? dy : 0);
    s32 x1=(s32)a.x+a.w+(dx>0 ? dx : 0),y1=(s32)a.y+a.h+(dy>0 ? dy : 0);
    return x0<=(s32)b.x+b.w && x1>=b.x && y0<=(s32)b.y+b.h && y1>=b.y;
}
static CcHit query(CcBox a,s32 dx,s32 dy,CcBox b)
{
    if(inside(a,b))return result(CC_OVERLAP);
    Ratio enter={-1,1},exit={2,1};s16 nx=0,ny=0;
    for(u16 axis=0;axis<2;axis++) {
        s32 p=axis ? a.y : a.x,size=axis ? a.h : a.w;
        s32 lo=axis ? b.y : b.x,hi=lo+(axis ? b.h : b.w),d=axis ? dy : dx;
        if(!d){if(p>=hi || p+size<=lo)return result(CC_MISS);continue;}
        Ratio en,ex;s16 normal;
        if(d>0){en=(Ratio){lo-p-size,d};ex=(Ratio){hi-p,d};normal=-1;}
        else{en=(Ratio){p-hi,-d};ex=(Ratio){p+size-lo,-d};normal=1;}
        if(less(enter,en)){enter=en;nx=axis ? 0 : normal;ny=axis ? normal : 0;}
        else if(equal(enter,en)){if(axis)ny=normal;else nx=normal;}
        if(less(ex,exit))exit=ex;
    }
    /* Grazing one corner for an instant does not enter the open interior. */
    if(enter.n<0 || enter.n>enter.d || !less(enter,exit))return result(CC_MISS);
    CcHit h=result(CC_HIT);h.num=enter.n;h.den=enter.d;
    h.time=(u16)(enter.n*CC_TIME/enter.d);h.nx=nx;h.ny=ny;return h;
}
CcHit cc_sweep(CcBox a,CcVec da,CcBox b,CcVec db)
{
    if(!valid(a,false) || !valid(b,false) || !motion(a,da) || !motion(b,db))return result(CC_INVALID);
    s32 dx=(s32)da.x-db.x,dy=(s32)da.y-db.y;
    if(!broad(a,dx,dy,b))return result(CC_MISS);
    return query(a,dx,dy,b);
}
CcHit cc_ray(CcVec start,CcVec delta,CcBox target)
{
    CcBox point={start.x,start.y,0,0};
    if(!valid(point,true) || !valid(target,false) || !motion(point,delta))return result(CC_INVALID);
    if(!broad(point,delta.x,delta.y,target))return result(CC_MISS);
    return query(point,delta.x,delta.y,target);
}
CcHit cc_cast(CcBox a,CcVec delta,const CcBox *walls,u16 count,CcStats *stats)
{
    CcStats local={0,0};if(!stats)stats=&local;*stats=local;
    if(!valid(a,false) || !motion(a,delta) || count>CC_MAX_BOXES || (count && !walls))return result(CC_INVALID);
    /* Validate the entire supplied list, even walls rejected by broad phase. */
    for(u16 i=0;i<count;i++)if(!valid(walls[i],false))return result(CC_INVALID);
    CcHit best=result(CC_MISS);
    for(u16 i=0;i<count;i++) {
        stats->visited++;
        if(!broad(a,delta.x,delta.y,walls[i]))continue;
        stats->narrow++;CcHit h=query(a,delta.x,delta.y,walls[i]);h.index=i;
        if(h.kind==CC_OVERLAP)return h;
        if(h.kind!=CC_HIT)continue;
        if(best.kind!=CC_HIT || less((Ratio){h.num,h.den},(Ratio){best.num,best.den}))best=h;
        else if(equal((Ratio){h.num,h.den},(Ratio){best.num,best.den})) {
            /* Simultaneous walls: combine axes; lowest list index wins metadata. */
            if(h.nx)best.nx=h.nx;
            if(h.ny)best.ny=h.ny;
        }
    }
    return best;
}
CcMove cc_slide(CcBox a,CcVec delta,const CcBox *walls,u16 count)
{
    CcMove m={a,CC_MISS,0,delta,{0,0}};
    /* Run even a zero move so an invalid list or initial overlap is reported. */
    for(u16 step=0;step<CC_SLIDES;step++) {
        CcStats stats;CcHit h=cc_cast(m.box,m.remaining,walls,count,&stats);
        m.stats.visited+=stats.visited;m.stats.narrow+=stats.narrow;
        if(h.kind==CC_INVALID || h.kind==CC_OVERLAP){m.kind=h.kind;return m;}
        if(h.kind==CC_MISS) {
            m.box.x+=m.remaining.x;m.box.y+=m.remaining.y;m.remaining=(CcVec){0,0};return m;
        }
        m.kind=CC_HIT;m.contacts++;
        /* Use the exact fraction, not the rounded display time. Products fit s32. */
        s16 used_x=(s16)((s32)m.remaining.x*h.num/h.den);
        s16 used_y=(s16)((s32)m.remaining.y*h.num/h.den);
        m.box.x+=used_x;m.box.y+=used_y;
        m.remaining.x-=used_x;m.remaining.y-=used_y;
        /* A blocked coordinate is exactly on an integer raw-unit face. */
        if(h.nx)m.remaining.x=0;
        if(h.ny)m.remaining.y=0;
        if(!m.remaining.x && !m.remaining.y)return m;
    }
    m.kind=CC_EXHAUSTED;return m;
}
