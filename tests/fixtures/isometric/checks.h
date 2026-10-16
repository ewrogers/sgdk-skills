#ifndef ISOMETRIC_CHECKS_H
#define ISOMETRIC_CHECKS_H
#include "iso.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)
static int check_projection(void)
{
    IlPoint p=il_project(256,0,0);CHECK(p.x==16 && p.y==8);
    p=il_project(0,256,0);CHECK(p.x==-16 && p.y==8);
    p=il_project(256,256,512);CHECK(!p.x && p.y==-16);
    p=il_project(-1,0,0);CHECK(p.x==-1 && p.y==-1); /* Floor, not truncation. */
    for(s16 x=-4096;x<=4096;x+=256)for(s16 y=-4096;y<=4096;y+=256) {
        p=il_project(x,y,128);IlPoint back;
        CHECK(il_unproject(p,128,&back) && back.x==x && back.y==y);
    }
    IlPoint out={123,456};CHECK(!il_unproject((IlPoint){1024,1024},0,&out) && out.x==123 && out.y==456);
    CHECK(!il_unproject((IlPoint){0,0},-1,&out));
    CHECK(!il_unproject((IlPoint){0,0},0,(IlPoint *)0));
    return 0;
}
static int check_relations(void)
{
    IlBox low={0,0,0,256,256,384},deck={0,0,448,256,256,512},high={0,0,512,256,256,896};
    CHECK(il_relation(low,deck)==IL_BEFORE && il_relation(deck,low)==IL_AFTER);
    CHECK(il_relation(deck,high)==IL_BEFORE && il_relation(high,deck)==IL_AFTER);
    CHECK(il_relation(low,low)==IL_AMBIGUOUS);
    CHECK(il_relation(low,(IlBox){1000,1000,0,1256,1256,384})==IL_NONE);
    /* Rectangle overlap alone would invent a contradictory X/Y relationship. */
    CHECK(il_relation((IlBox){0,512,0,256,768,256},(IlBox){512,0,0,768,256,256})==IL_NONE);
    CHECK(il_relation(low,(IlBox){0,0,0,0,1,1})==IL_INVALID);
    CHECK(il_relation(low,(IlBox){0,0,0,8193,1,1})==IL_INVALID);
    return 0;
}
static int check_sort(void)
{
    IlItem items[]={{{0,0,512,256,256,896},30},{{0,0,0,256,256,384},10},{{0,0,448,256,256,512},20}};
    u16 order[IL_MAX_ITEMS];CHECK(il_sort(items,3,(IlLink *)0,0,order)==IL_SORT_OK);
    CHECK(order[0]==1 && order[1]==2 && order[2]==0);
    IlItem swapped[]={items[2],items[0],items[1]};u16 other[IL_MAX_ITEMS];
    CHECK(il_sort(swapped,3,(IlLink *)0,0,other)==IL_SORT_OK);
    for(u16 i=0;i<3;i++)CHECK(items[order[i]].id==swapped[other[i]].id);
    IlItem intersect[]={items[0],items[0]};intersect[1].id=40;
    CHECK(il_sort(intersect,2,(IlLink *)0,0,order)==IL_SORT_AMBIGUOUS);
    IlLink link={30,40};CHECK(il_sort(intersect,2,&link,1,order)==IL_SORT_OK && order[0]==0);
    IlItem separate[]={{{0,0,0,16,16,16},3},{{1000,1000,0,1016,1016,16},1},{{2000,2000,0,2016,2016,16},2}};
    IlLink cycle[]={{3,1},{1,2},{2,3}};
    CHECK(il_sort(separate,3,cycle,3,order)==IL_SORT_CYCLE);
    CHECK(order[0]!=order[1] && order[1]!=order[2] && order[0]!=order[2]);
    IlLink bad={30,999};order[0]=999;
    CHECK(il_sort(items,3,&bad,1,order)==IL_SORT_INVALID && order[0]==999);
    CHECK(il_sort(items,IL_MAX_ITEMS+1,(IlLink *)0,0,order)==IL_SORT_INVALID);
    items[1].id=30;CHECK(il_sort(items,3,(IlLink *)0,0,order)==IL_SORT_INVALID);
    IlItem full[IL_MAX_ITEMS];IlLink chain[IL_MAX_ITEMS];
    for(u16 i=0;i<IL_MAX_ITEMS;i++) {
        full[i]=(IlItem){{(s16)(i*256),0,0,(s16)(i*256+16),16,16},(u16)(100-i)};
        if(i)chain[i-1]=(IlLink){full[i-1].id,full[i].id};
    }
    CHECK(il_sort(full,IL_MAX_ITEMS,chain,IL_MAX_ITEMS-1,order)==IL_SORT_OK);
    for(u16 i=0;i<IL_MAX_ITEMS;i++)CHECK(order[i]==i);
    chain[IL_MAX_ITEMS-1]=(IlLink){full[IL_MAX_ITEMS-1].id,full[0].id};
    CHECK(il_sort(full,IL_MAX_ITEMS,chain,IL_MAX_ITEMS,order)==IL_SORT_CYCLE);
    return 0;
}
static int check_world(u16 frames)
{
    IlGame g;il_init(&g);IlItem items[IL_MAX_ITEMS];IlLink links[IL_MAX_LINKS];u16 nlinks,order[IL_MAX_ITEMS];
    il_scene(&g,items,links,&nlinks);IlCache cache;CHECK(il_cache_init(&cache,items,IL_STATIC)==IL_SORT_OK);
    for(u16 stop=0;stop<4;stop++) {
        il_tour(&g,stop);il_scene(&g,items,links,&nlinks);
        CHECK(il_sort(items,IL_ITEMS,links,nlinks,order)==IL_SORT_OK);
        u16 cached[IL_MAX_ITEMS];CHECK(il_sort_cached(&cache,&items[IL_STATIC],1,links,nlinks,cached)==IL_SORT_OK);
        for(u16 i=0;i<IL_ITEMS;i++)CHECK(order[i]==cached[i]);
    }
    /* Walk onto the left stairs, across the deck, and down the right stairs. */
    g.x=480;g.y=1408;g.z=0;g.layer=0;
    u16 heights=0;
    for(u16 i=0;i<170;i++){il_tick(&g,IL_RIGHT|IL_DOWN);if(g.z)heights++;}
    CHECK(heights>0 && g.x>2048 && !g.layer && !g.z && g.y==1408);
    /* Turning back while the footprint touches the first riser must not trap us. */
    g.x=500;g.y=1408;g.z=0;g.layer=0;
    il_tick(&g,IL_LEFT|IL_UP);CHECK(g.x<500 && !g.layer);
    /* The lower avenue passes below the same deck without switching layers. */
    g.x=1280;g.y=1200;g.z=0;g.layer=0;
    for(u16 i=0;i<30;i++)il_tick(&g,IL_DOWN);
    CHECK(!g.layer && !g.z && g.y>1536);
    /* An upper actor cannot leave a walkway edge into thin air. */
    il_tour(&g,2);
    for(u16 i=0;i<30;i++)il_tick(&g,IL_UP);
    CHECK(g.layer && g.y>=1320 && g.z==512);
    /* Ground footprint blocks a tower, independent of sprite pixels. */
    g.x=184;g.y=384;g.z=0;g.layer=0;
    for(u16 i=0;i<60;i++)il_tick(&g,IL_RIGHT|IL_DOWN);
    CHECK(g.x<=216 && g.y==384);
    /* Reuse static edges through a changing route; compare full and cached sort. */
    il_init(&g);
    for(u16 i=0;i<frames;i++) {
        if(!(i%100))il_tour(&g,(i/100)%4);
        il_tick(&g,(i%100)<50 ? IL_RIGHT|IL_DOWN : IL_UP);
        il_scene(&g,items,links,&nlinks);u16 cached[IL_MAX_ITEMS];
        u16 full=il_sort(items,IL_ITEMS,links,nlinks,order);
        CHECK(full==IL_SORT_OK);
        CHECK(il_sort_cached(&cache,&items[IL_STATIC],1,links,nlinks,cached)==full);
        for(u16 j=0;j<IL_ITEMS;j++)CHECK(order[j]==cached[j]);
    }
    /* Camera is derived after simulation and constrained to authored map space. */
    CHECK(g.camera_x>=64 && g.camera_x<=128 && g.camera_y>=0 && g.camera_y<=64);
    return 0;
}
static int check_cache_and_timing(void)
{
    IlGame a,b;il_init(&a);il_init(&b);
    for(u16 i=0;i<600;i++)il_frame(&a,0,60,false);
    for(u16 i=0;i<500;i++)il_frame(&b,0,50,false);
    CHECK(a.ticks==600 && b.ticks==600 && a.phase==b.phase);
    u16 tick=b.ticks,phase=b.phase;
    for(u16 i=0;i<50;i++)CHECK(!il_frame(&b,IL_RIGHT,50,true));
    CHECK(b.ticks==tick && b.phase==phase && !il_frame(&b,0,0,false));
    IlItem items[IL_MAX_ITEMS];IlLink links[IL_MAX_LINKS];u16 count,order[IL_MAX_ITEMS];IlCache cache;
    il_scene(&a,items,links,&count);CHECK(il_cache_init(&cache,items,IL_STATIC)==IL_SORT_OK);
    items[0].id=999; /* The cache owns a copy, not a reference to temporary input. */
    CHECK(cache.fixed[0].id==0);
    IlItem actor={0}; /* Initialized below; duplicate IDs must fail. */
    actor=cache.fixed[0];CHECK(il_sort_cached(&cache,&actor,1,(IlLink *)0,0,order)==IL_SORT_INVALID);
    CHECK(il_cache_init(&cache,(IlItem *)0,1)==IL_SORT_INVALID && !cache.valid);
    CHECK(il_sort_cached(&cache,&actor,1,(IlLink *)0,0,order)==IL_SORT_INVALID);
    return 0;
}
static int check_soak(u16 frames)
{
    IlGame g;il_init(&g);IlItem items[IL_MAX_ITEMS];IlLink links[IL_MAX_LINKS];u16 nlinks,order[IL_MAX_ITEMS];IlCache cache;
    il_scene(&g,items,links,&nlinks);CHECK(il_cache_init(&cache,items,IL_STATIC)==IL_SORT_OK);
    u32 rng=12345;u16 input=0;
    const u16 inputs[]={IL_LEFT,IL_RIGHT,IL_UP,IL_DOWN,IL_LEFT|IL_UP,IL_RIGHT|IL_UP,IL_LEFT|IL_DOWN,IL_RIGHT|IL_DOWN};
    for(u16 i=0;i<frames;i++) {
        if(!(i%23)){rng=rng*1664525UL+1013904223UL;input=inputs[(rng>>24)&7];}
        if(!(i%257))il_tour(&g,(i/257)%4);
        il_tick(&g,input);il_scene(&g,items,links,&nlinks);
        CHECK(il_sort_cached(&cache,&items[IL_STATIC],1,links,nlinks,order)==IL_SORT_OK);
        CHECK(g.x>=64 && g.x<=2496 && g.y>=64 && g.y<=2496 && g.z<=512);
    }
    return 0;
}
static u32 trace_digest(u16 frames)
{
    IlGame g;il_init(&g);IlItem items[IL_MAX_ITEMS];IlLink links[IL_MAX_LINKS];u16 nlinks,order[IL_MAX_ITEMS];IlCache cache;
    il_scene(&g,items,links,&nlinks);il_cache_init(&cache,items,IL_STATIC);
    u32 digest=2166136261UL;
    for(u16 i=0;i<frames;i++) {
        if(!(i%64))il_tour(&g,(i/64)%4);
        il_tick(&g,(i%64)<32 ? IL_RIGHT : IL_DOWN);
        il_scene(&g,items,links,&nlinks);u16 status=il_sort_cached(&cache,&items[IL_STATIC],1,links,nlinks,order);
        digest=(digest^(u16)g.x)*16777619UL;digest=(digest^(u16)g.y)*16777619UL;
        digest=(digest^(u16)g.z)*16777619UL;digest=(digest^status)*16777619UL;
        for(u16 j=0;j<IL_ITEMS;j++)digest=(digest^order[j])*16777619UL;
    }
    return digest;
}
#endif
