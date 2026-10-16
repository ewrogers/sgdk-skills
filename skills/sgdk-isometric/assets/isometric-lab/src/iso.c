#include "iso.h"
static s32 floor_div(s32 n,s32 d){s32 q=n/d;return q-(n%d<0);}
IlPoint il_project(s16 x,s16 y,s16 z)
{
    return (IlPoint){(s16)floor_div((s32)x-y,16),(s16)floor_div((s32)x+y-2*(s32)z,32)};
}
bool il_unproject(IlPoint p,s16 z,IlPoint *out)
{
    if(!out || z<0 || z>2048 || p.x < -1024 || p.x>1024 || p.y < -1024 || p.y>1024)return false;
    s32 x=8*(s32)p.x+16*(s32)p.y+z,y=-8*(s32)p.x+16*(s32)p.y+z;
    if(x < -IL_LIMIT || x>IL_LIMIT || y < -IL_LIMIT || y>IL_LIMIT)return false;
    *out=(IlPoint){(s16)x,(s16)y};return true;
}
static bool valid(IlBox b)
{
    return b.x0>=-IL_LIMIT && b.y0>=-IL_LIMIT && b.z0>=0 && b.x1<=IL_LIMIT && b.y1<=IL_LIMIT && b.z1<=2048 &&
        b.x0<b.x1 && b.y0<b.y1 && b.z0<b.z1;
}
static bool interval(s32 a0,s32 a1,s32 b0,s32 b1){return a0<b1 && b0<a1;}
u16 il_relation(IlBox a,IlBox b)
{
    if(!valid(a) || !valid(b))return IL_INVALID;
    /* Separating axes of the projected 3-D box hexagons: x-y, x-z, y-z. */
    if(!interval((s32)a.x0-a.y1,(s32)a.x1-a.y0,(s32)b.x0-b.y1,(s32)b.x1-b.y0) ||
       !interval((s32)a.x0-a.z1,(s32)a.x1-a.z0,(s32)b.x0-b.z1,(s32)b.x1-b.z0) ||
       !interval((s32)a.y0-a.z1,(s32)a.y1-a.z0,(s32)b.y0-b.z1,(s32)b.y1-b.z0))return IL_NONE;
    bool before=a.x1<=b.x0 || a.y1<=b.y0 || a.z1<=b.z0;
    bool after=b.x1<=a.x0 || b.y1<=a.y0 || b.z1<=a.z0;
    if(before && !after)return IL_BEFORE;
    if(after && !before)return IL_AFTER;
    return IL_AMBIGUOUS;
}
static const IlItem *item_at(const IlItem *fixed,u16 n,const IlItem *dynamic,u16 i)
{
    return i<n ? &fixed[i] : &dynamic[i-n];
}
static u16 sort(const IlItem *fixed,u16 fixed_count,const u32 *fixed_edges,const IlItem *dynamic,u16 dynamic_count,
                const IlLink *links,u16 link_count,u16 *order)
{
    u16 count=fixed_count+dynamic_count;
    if(!count || count>IL_MAX_ITEMS || link_count>IL_MAX_LINKS || (dynamic_count && !dynamic) || !order || (link_count && !links))return IL_SORT_INVALID;
    for(u16 i=fixed_count;i<count;i++) {
        const IlItem *item=item_at(fixed,fixed_count,dynamic,i);
        if(!valid(item->box))return IL_SORT_INVALID;
        for(u16 j=0;j<i;j++)if(item->id==item_at(fixed,fixed_count,dynamic,j)->id)return IL_SORT_INVALID;
    }
    u32 edges[IL_MAX_ITEMS]={0};u16 indegree[IL_MAX_ITEMS]={0};u32 used=0;u16 status=IL_SORT_OK;
    for(u16 i=0;i<fixed_count;i++)edges[i]=fixed_edges[i];
    for(u16 i=0;i<link_count;i++) {
        u16 a=count,b=count;
        for(u16 j=0;j<count;j++) {
            u16 id=item_at(fixed,fixed_count,dynamic,j)->id;
            if(id==links[i].before)a=j;
            if(id==links[i].after)b=j;
        }
        if(a==count || b==count || a==b)return IL_SORT_INVALID;
        /* Explicit links replace inference for that pair, including cached edges. */
        edges[b]&=~((u32)1<<a);edges[a]|=(u32)1<<b;
        for(u16 j=0;j<i;j++)if(links[j].before==links[i].after && links[j].after==links[i].before)return IL_SORT_INVALID;
    }
    for(u16 i=0;i<count;i++)for(u16 j=i+1;j<count;j++) {
        if(i<fixed_count && j<fixed_count)continue;
        if((edges[i]&((u32)1<<j)) || (edges[j]&((u32)1<<i)))continue;
        u16 relation=il_relation(item_at(fixed,fixed_count,dynamic,i)->box,item_at(fixed,fixed_count,dynamic,j)->box);
        if(relation==IL_BEFORE)edges[i]|=(u32)1<<j;
        else if(relation==IL_AFTER)edges[j]|=(u32)1<<i;
        else if(relation==IL_AMBIGUOUS)status=IL_SORT_AMBIGUOUS;
    }
    for(u16 i=0;i<count;i++)for(u16 j=0;j<count;j++)if(edges[i]&((u32)1<<j))indegree[j]++;
    for(u16 step=0;step<count;step++) {
        u16 pick=count;
        for(u16 i=0;i<count;i++)if(!(used&((u32)1<<i)) && !indegree[i] &&
            (pick==count || item_at(fixed,fixed_count,dynamic,i)->id<item_at(fixed,fixed_count,dynamic,pick)->id))pick=i;
        if(pick==count) {
            status=IL_SORT_CYCLE;
            for(u16 i=0;i<count;i++)if(!(used&((u32)1<<i)) &&
                (pick==count || item_at(fixed,fixed_count,dynamic,i)->id<item_at(fixed,fixed_count,dynamic,pick)->id))pick=i;
        }
        order[step]=pick;used|=(u32)1<<pick;
        for(u16 j=0;j<count;j++)if((edges[pick]&((u32)1<<j)) && indegree[j])indegree[j]--;
    }
    return status;
}
u16 il_sort(const IlItem *items,u16 count,const IlLink *links,u16 link_count,u16 *order)
{
    return sort((const IlItem *)0,0,(const u32 *)0,items,count,links,link_count,order);
}
u16 il_cache_init(IlCache *cache,const IlItem *fixed,u16 count)
{
    if(!cache)return IL_SORT_INVALID;
    cache->valid=false;
    if(!fixed || !count || count>IL_MAX_ITEMS)return IL_SORT_INVALID;
    u16 order[IL_MAX_ITEMS];u16 status=il_sort(fixed,count,(const IlLink *)0,0,order);
    if(status!=IL_SORT_OK)return status;
    cache->count=count;
    for(u16 i=0;i<count;i++) {
        cache->fixed[i]=fixed[i];cache->edges[i]=0;
        for(u16 j=0;j<count;j++)if(i!=j && il_relation(fixed[i].box,fixed[j].box)==IL_BEFORE)cache->edges[i]|=(u32)1<<j;
    }
    cache->valid=true;return IL_SORT_OK;
}
u16 il_sort_cached(const IlCache *cache,const IlItem *dynamic,u16 count,const IlLink *links,u16 link_count,u16 *order)
{
    if(!cache || !cache->valid || cache->count>IL_MAX_ITEMS || count>IL_MAX_ITEMS-cache->count)return IL_SORT_INVALID;
    return sort(cache->fixed,cache->count,cache->edges,dynamic,count,links,link_count,order);
}
