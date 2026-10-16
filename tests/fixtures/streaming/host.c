#include <stdio.h>
#include <string.h>
#include "checks.h"

static int soak(void)
{
    StreamCache c;TestIO io={0};sc_init(&c);u32 random=7;
    for(u16 frame=0;frame<10000;frame++) {
        random=random*1664525UL+1013904223UL;
        u16 room=(u16)(random>>24)%4;
        u16 pages[6];for(u16 i=0;i<6;i++)pages[i]=room*6+i;
        CHECK(sc_pin(&c,pages,6));
        for(u16 i=0;i<6;i++)if(sc_require(&c,pages[i])!=SC_AVAILABLE)break;
        u16 sent=sc_step(&c,(frame%8)+1,read_page,queue_page,&io);
        CHECK(sent<=SC_CHUNK_TILES && sent<=(frame%8)+1);
        if(frame%3==0)sc_complete(&c,false);else flush_page(&c,&io);
        for(u16 i=0;i<SC_SLOTS;i++) {
            const ScSlot *s=&c.slots[i];
            if(!s->state)continue;
            CHECK(c.slot_for[s->page]==i);
            for(u16 j=0;j<i;j++)CHECK(!c.slots[j].state || c.slots[j].page!=s->page);
            if(s->state==SC_READY)for(u16 n=0;n<512;n++)CHECK(io.uploaded[i*512+n]==expected(s->page,n));
        }
    }
    flush_page(&c,&io);return 0;
}
static bool ready_view(const StreamCache *c,u16 camera)
{
    u16 tile;
    for(u16 p=camera/320*6;p<=((camera+319)/320)*6+5;p++)
        if(!sc_tile(c,p,0,&tile))return false;
    return true;
}
static int walk(void)
{
    for(u16 hz=50;hz<=60;hz+=10)for(u16 budget=2;budget<=8;budget+=6) {
        StreamCache c;TestIO io={0};sc_init(&c);
        u16 pages[SC_SLOTS],count=sw_wanted(0,128,false,pages);
        CHECK(sc_pin(&c,pages,count));
        for(u16 i=0;i<count;i++)CHECK(load_page(&c,&io,pages[i],budget));
        u16 camera=0,phase=0;bool right=true;
        for(u16 frame=0;frame<2200;frame++) {
            phase+=60;u16 steps=phase/hz;phase%=hz;
            if(camera==960)right=false;
            if(!camera)right=true;
            u16 after=right ? (camera+steps*2>960 ? 960 : camera+steps*2) : (camera<steps*2 ? 0 : camera-steps*2);
            u16 left=after<128 ? 0 : after-128,right_edge=after>832 ? 960 : after+128;
            count=sw_wanted(left,right_edge,false,pages);CHECK(count && sc_pin(&c,pages,count));
            for(u16 i=0;i<count;i++)if(sc_require(&c,pages[i])!=SC_AVAILABLE)break;
            sc_step(&c,budget,read_page,queue_page,&io);
            CHECK(ready_view(&c,camera)); /* Never evict a still-visible page. */
            CHECK(ready_view(&c,after)); /* Enough lead time even at 64 B/frame PAL. */
            camera=after;flush_page(&c,&io);
        }
        CHECK(c.evictions>0 && !c.read_errors && !c.queue_errors);
    }
    return 0;
}

int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    int error=!strcmp(argv[1],"core") ? check_core() : !strcmp(argv[1],"world") ? check_world() : !strcmp(argv[1],"walk") ? walk() : soak();
    if(error)fprintf(stderr,"Streaming contract failed at line %d\n",error);
    return error ? 1 : 0;
}
