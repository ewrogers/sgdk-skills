#include "blocks.h"
static int block_contracts(void)
{
    BlGame g,before;bl_init(&g,147);CHECK(bl_valid(&g));
    for(u16 p=0;p<7;p++)for(u16 r=0;r<4;r++) {
        u16 mask=bl_shape(p,r),count=0;
        for(u16 i=0;i<16;i++)if(mask&(1u<<i))count++;
        CHECK(count==4);
    }
    CHECK(!bl_shape(7,0));CHECK(!bl_shape(0,4));
    u16 bag=0;
    for(u16 i=0;i<7;i++) {
        CHECK(!(bag&(1u<<g.piece)));bag|=1u<<g.piece;
        memset(g.cells,0,sizeof(g.cells));CHECK(bl_drop(&g)&1);CHECK(bl_valid(&g));
    }
    CHECK(bag==127);
    bl_init(&g,147);g.piece=1;g.x=0;g.y=0;before=g;
    CHECK(!bl_shift(&g,-2,0));CHECK(!bytecmp(&g,&before,sizeof(g)));
    while(bl_shift(&g,-1,0)){}before=g;CHECK(!bl_shift(&g,-1,0));CHECK(!bytecmp(&g,&before,sizeof(g)));
    g.piece=2;g.rotation=0;g.x=3;g.y=3;
    for(u16 i=0;i<BL_W*BL_H;i++)g.cells[i]=1;
    before=g;CHECK(!bl_rotate(&g));CHECK(!bytecmp(&g,&before,sizeof(g)));
    bl_init(&g,8);memset(g.cells,0,sizeof(g.cells));
    for(u16 y=14;y<18;y++)for(u16 x=0;x<BL_W;x++)g.cells[y*BL_W+x]=3;
    g.cells[13*BL_W+2]=7;CHECK(bl_clear(&g)==4);CHECK(g.cells[17*BL_W+2]==7);
    for(u16 i=0;i<17*BL_W;i++)CHECK(!g.cells[i]);
    CHECK(!bl_clear(&g));
    bl_init(&g,22);g.piece=0;g.rotation=1;g.x=3;g.y=0;
    /* The rotated I occupies column 2 of its four-cell footprint. */
    u16 column=0;for(u16 x=0;x<4;x++)if(bl_shape(0,1)&(1u<<x))column=x;
    for(u16 y=14;y<18;y++)for(u16 x=0;x<BL_W;x++)if(x!=g.x+column)g.cells[y*BL_W+x]=2;
    CHECK(bl_drop(&g)&2);CHECK(g.lines==4 && g.score==800);CHECK(bl_valid(&g));
    bl_init(&g,147);g.piece=1;g.x=3;g.y=15;CHECK(bl_shift(&g,0,1));CHECK(!bl_shift(&g,0,1));
    for(u16 i=0;i<29;i++)CHECK(!bl_tick(&g,0));
    CHECK(g.lock==29);CHECK(bl_tick(&g,0)&1);
    bl_init(&g,147);g.piece=1;g.x=3;g.y=15;CHECK(bl_shift(&g,0,1));
    for(u16 i=0;i<29;i++)CHECK(!bl_tick(&g,i&1 ? BL_LEFT : BL_RIGHT));
    CHECK(bl_tick(&g,BL_LEFT)&1); /* Floor shuffling cannot postpone locking forever. */
    bl_init(&g,147);memset(g.cells,1,2*BL_W);g.cells[0]=0;g.cells[BL_W]=0;g.piece=1;g.x=3;g.y=15;CHECK(bl_drop(&g)&4);CHECK(g.over);
    before=g;CHECK(!bl_drop(&g));CHECK(!bl_shift(&g,1,0));CHECK(!bl_rotate(&g));CHECK(!bytecmp(&g,&before,sizeof(g)));
    for(u16 hz=50;hz<=60;hz+=10) {
        u16 phase=0,count=0;for(u16 i=0;i<hz*10;i++)count+=bl_clock(&phase,hz);
        CHECK(count==600 && phase==0);CHECK(!bl_clock(&phase,0));CHECK(phase==0);
    }
    return 0;
}
static int block_soak(u16 ticks,u32 *digest)
{
    BlGame g;bl_init(&g,2026);u32 h=2166136261u,rng=418;
    for(u16 i=0;i<ticks;i++) {
        rng=rng*1664525u+1013904223u;
        if(g.over)bl_init(&g,rng);
        bl_tick(&g,(u16)((rng>>24)&31));CHECK(bl_valid(&g));h=(h^bl_digest(&g))*16777619u;
    }
    *digest=h;return 0;
}
