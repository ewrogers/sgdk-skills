#ifndef STREAM_CHECKS_H
#define STREAM_CHECKS_H
#include "stream_cache.h"
#include "stream_world.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)

typedef struct {
    const u8 *borrowed;
    u16 tile,bytes,reads,queues;
    bool fail_read,fail_queue;
    u8 uploaded[SC_SLOTS*SC_PAGE_TILES*SC_TILE_BYTES];
} TestIO;

static u8 expected(u16 page,u16 offset) { return (u8)(page*17+offset*3+(offset/32)); }
static bool read_page(void *context,u16 page,u16 offset,void *dest,u16 bytes)
{
    TestIO *io=context;io->reads++;
    if(io->fail_read)return false;
    for(u16 i=0;i<bytes;i++)((u8 *)dest)[i]=expected(page,offset+i);
    return true;
}
static bool queue_page(void *context,const void *source,u16 tile,u16 tiles)
{
    TestIO *io=context;io->queues++;
    if(io->fail_queue)return false;
    io->borrowed=source;io->tile=tile;io->bytes=tiles*SC_TILE_BYTES;
    return true;
}
static void flush_page(StreamCache *c,TestIO *io)
{
    if(io->borrowed) {
        u16 offset=(io->tile-SC_CACHE_BASE)*SC_TILE_BYTES;
        for(u16 i=0;i<io->bytes;i++)io->uploaded[offset+i]=io->borrowed[i];
        io->borrowed=0;
    }
    sc_complete(c,true);
}
static bool load_page(StreamCache *c,TestIO *io,u16 page,u16 budget)
{
    if(sc_require(c,page)!=SC_WAIT)return false;
    for(u16 n=0;n<SC_PAGE_TILES;n++) {
        if(sc_require(c,page)==SC_AVAILABLE)return true;
        if(!sc_step(c,budget,read_page,queue_page,io))return false;
        flush_page(c,io);
    }
    return sc_require(c,page)==SC_AVAILABLE;
}

static int check_core(void)
{
    StreamCache c;TestIO io={0};u16 tile;
    sc_init(&c);
    CHECK(sc_require(&c,SC_PAGES)==SC_INVALID && !c.misses);
    u16 pinned[]={0};CHECK(sc_pin(&c,pinned,1));
    CHECK(sc_require(&c,0)==SC_WAIT && c.misses==1);
    CHECK(!sc_tile(&c,0,0,&tile));
    CHECK(!sc_step(&c,0,read_page,queue_page,&io) && !io.reads);
    io.fail_read=true;
    CHECK(!sc_step(&c,8,read_page,queue_page,&io) && c.read_errors==1 && !c.completed);
    io.fail_read=false;io.fail_queue=true;
    CHECK(!sc_step(&c,8,read_page,queue_page,&io) && c.queue_errors==1 && !c.completed && !c.flight);
    io.fail_queue=false;
    CHECK(sc_step(&c,0xFFFF,read_page,queue_page,&io)==SC_CHUNK_TILES);
    u16 reads=io.reads;u8 first=((u8 *)c.staging)[0];
    CHECK(!sc_step(&c,8,read_page,queue_page,&io) && io.reads==reads && ((u8 *)c.staging)[0]==first);
    CHECK(!sc_reset(&c));sc_complete(&c,false);CHECK(c.flight==8 && !c.completed);
    flush_page(&c,&io);CHECK(c.completed==8 && !sc_tile(&c,0,0,&tile));
    CHECK(sc_require(&c,1)==SC_WAIT && c.misses==1);
    CHECK(sc_step(&c,8,read_page,queue_page,&io)==8);flush_page(&c,&io);
    CHECK(sc_tile(&c,0,0,&tile) && tile==SC_CACHE_BASE);
    for(u16 i=0;i<512;i++)CHECK(io.uploaded[i]==expected(0,i));
    CHECK(!sc_tile(&c,0,16,&tile) && !sc_tile(&c,0,0,0));
    for(u16 p=1;p<SC_SLOTS;p++)CHECK(load_page(&c,&io,p,8));
    /* Pinning is atomic on invalid input; the oldest unpinned page is evicted. */
    u16 invalid[]={0,0};CHECK(!sc_pin(&c,invalid,2) && c.slots[0].pinned);
    CHECK(sc_require(&c,18)==SC_WAIT && c.evictions==1 && c.slots[0].page==0 && c.slots[1].page==18);
    CHECK(sc_step(&c,8,read_page,queue_page,&io)==8);flush_page(&c,&io);
    CHECK(sc_step(&c,8,read_page,queue_page,&io)==8);flush_page(&c,&io);
    u16 all[SC_SLOTS];for(u16 i=0;i<SC_SLOTS;i++)all[i]=c.slots[i].page;
    CHECK(sc_pin(&c,all,SC_SLOTS));CHECK(sc_require(&c,23)==SC_FULL && c.pending==SC_SLOTS);
    CHECK(sc_pin(&c,0,0));CHECK(sc_reset(&c));
    c.clock=0xFFFFFFFCUL;
    CHECK(load_page(&c,&io,2,2) && load_page(&c,&io,3,3));
    CHECK(c.clock<20); /* Unsigned LRU age arithmetic crosses wrap safely. */
    return 0;
}

static int check_world(void)
{
    u16 pages[SC_SLOTS],column;
    for(u16 hz=50;hz<=60;hz+=10) {
        SwFade fade={0};sw_fade_begin(&fade);
        u16 frames=0;
        while(fade.state==SW_FADE_OUT) {
            u16 before=fade.level;sw_fade_tick(&fade,hz,true);frames++;
            CHECK(fade.level<=before && frames<=15);
        }
        CHECK(frames==(hz+3)/4 && fade.state==SW_FADE_LOAD && !fade.level);
        for(u16 i=0;i<1000;i++)sw_fade_tick(&fade,hz,false);
        CHECK(fade.state==SW_FADE_LOAD && !fade.level); /* Never reveal unfinished art. */
        sw_fade_tick(&fade,hz,true);
        CHECK(fade.state==SW_FADE_IN && !fade.level);
        frames=0;
        while(fade.state==SW_FADE_IN) {
            u16 before=fade.level;sw_fade_tick(&fade,hz,true);frames++;
            CHECK(fade.level>=before && frames<=15);
        }
        CHECK(frames==(hz+3)/4 && fade.state==SW_FADE_IDLE && fade.level==8);
        sw_fade_tick(&fade,hz,false);CHECK(fade.level==8);
        sw_fade_begin(&fade);sw_fade_tick(&fade,0,true);
        CHECK(fade.state==SW_FADE_OUT && fade.level==8 && !fade.phase);
    }
    for(u16 rgb=0;rgb<512;rgb++) {
        u16 color=((rgb&7)<<1)|(((rgb>>3)&7)<<5)|(((rgb>>6)&7)<<9),before=0;
        CHECK(!sw_fade_color(color,0) && sw_fade_color(color,8)==color);
        CHECK(sw_fade_color(color,99)==color);
        for(u16 level=0;level<=8;level++) {
            u16 next=sw_fade_color(color,level);
            CHECK(!(next & (u16)~0x0EEE));
            for(u16 shift=1;shift<=9;shift+=4)CHECK(((next>>shift)&7)>=((before>>shift)&7));
            before=next;
        }
    }
    for(u16 room=0;room<4;room++)CHECK(sw_camera(room*320+160)==room*320);
    CHECK(sw_camera(0)==0 && sw_camera(1280)==960);
    CHECK(sw_columns(0)==40 && sw_columns(1)==41 && sw_columns(960)==40 && !sw_columns(961));
    CHECK(sw_entering(0,2,&column) && column==40);
    CHECK(!sw_entering(2,4,&column));
    CHECK(sw_entering(8,6,&column) && column==0);
    CHECK(!sw_entering(0,320,&column));
    for(u16 camera=0;camera<=960;camera++) {
        u16 count=sw_wanted(camera,camera>832 ? 960 : camera+128,false,pages);
        CHECK(count>=6 && count<=18 && count%6==0);
        for(u16 i=0;i<count;i++) {
            CHECK(pages[i]<SC_PAGES);
            for(u16 j=0;j<i;j++)CHECK(pages[j]!=pages[i]);
        }
        /* Every visible pixel's room must retain all six of its pages. */
        for(u16 room=camera/320;room<=(camera+319)/320;room++)
            for(u16 p=room*6;p<room*6+6;p++) {
                bool found=false;for(u16 i=0;i<count;i++)found|=pages[i]==p;
                CHECK(found);
            }
    }
    CHECK(sw_wanted(319,641,false,pages)==0); /* Cover an arbitrary far teleport. */
    CHECK(sw_wanted(319,960,true,pages)==6);
    for(u16 x=0;x<160;x++)for(u16 y=0;y<24;y++)CHECK(sw_page(x,y)/6==x/40);
    CHECK(sw_page(4,8)==4 && sw_page(4,12)==3 && sw_page(4,20)==0);
    CHECK(sw_page(160,0)==SC_PAGES && sw_page(0,24)==SC_PAGES);
    return 0;
}
#endif
