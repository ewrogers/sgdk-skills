#include <stdio.h>
#include <string.h>
#include "checks.h"

/* Independent first-fit extent oracle; not a copy of SGDK's next-free search. */
enum { CAPACITY=32768, EXTENTS=128 };
static u16 arena[CAPACITY/2];
static struct {u16 offset,bytes;bool live;} extents[EXTENTS];
static u16 attempts,fail_at,invalid_frees;

void *MEM_alloc(u16 bytes)
{
    attempts++;if(!bytes || (fail_at && attempts==fail_at) || bytes>CAPACITY-2)return NULL;
    u16 size=(bytes+3)&0xFFFE;
    for(u16 offset=0;offset<=CAPACITY-size;) {
        u16 next=offset;
        for(u16 i=0;i<EXTENTS;i++)if(extents[i].live && offset<extents[i].offset+extents[i].bytes &&
            offset+size>extents[i].offset && extents[i].offset+extents[i].bytes>next)
            next=extents[i].offset+extents[i].bytes;
        if(next!=offset){offset=next;continue;}
        for(u16 i=0;i<EXTENTS;i++)if(!extents[i].live) {
            extents[i].offset=offset;extents[i].bytes=size;extents[i].live=TRUE;
            return (u8 *)arena+offset+2;
        }
    }
    return NULL;
}
void MEM_free(void *p)
{
    if(!p)return;
    for(u16 i=0;i<EXTENTS;i++)if(extents[i].live && (u8 *)arena+extents[i].offset+2==p) {
        extents[i].live=FALSE;return;
    }
    fprintf(stderr,"Invalid or repeated free\n");
    invalid_frees++;
}
void MEM_pack(void) {}
u16 MEM_getAllocated(void)
{
    u16 sum=0;for(u16 i=0;i<EXTENTS;i++)if(extents[i].live)sum+=extents[i].bytes;return sum;
}
u16 MEM_getFree(void) {return CAPACITY-MEM_getAllocated();}
u16 MEM_getLargestFreeBlock(void)
{
    u16 cursor=0,best=0;
    while(cursor<CAPACITY) {
        u16 start=CAPACITY,end=CAPACITY;
        for(u16 i=0;i<EXTENTS;i++)if(extents[i].live && extents[i].offset>=cursor && extents[i].offset<start) {
            start=extents[i].offset;end=start+extents[i].bytes;
        }
        if(start-cursor>best)best=start-cursor;
        cursor=end;
    }
    return best;
}
static int check_failures(void)
{
    MemoryLab lab;u16 base=MEM_getFree();
    fail_at=attempts+1;CHECK(!ml_init(&lab) && !lab.shared && MEM_getFree()==base);
    fail_at=0;CHECK(ml_init(&lab));
    for(u16 step=1;step<=7;step++) {
        fail_at=attempts+step;
        CHECK(!ml_heap_next(&lab) && !lab.stage && ml_intact(&lab));
        CHECK(MEM_getFree()==lab.baseline);
    }
    for(u16 step=1;step<=2;step++) {
        fail_at=attempts+step;CHECK(!ml_room_load(&lab) && !lab.room[0] && !lab.room[1]);
        CHECK(MEM_getFree()==lab.baseline);
    }
    fail_at=0;CHECK(ml_room_load(&lab));u16 used=MEM_getFree();
    fail_at=attempts+1;CHECK(!ml_stage_begin(&lab) && !lab.staging && MEM_getFree()==used);
    fail_at=0;CHECK(ml_shutdown(&lab) && MEM_getFree()==base);
    void *hole=MEM_alloc(2500),*barrier=MEM_alloc(256);CHECK(hole && barrier);
    MEM_free(hole);CHECK(ml_init(&lab));
    fail_at=attempts+8; /* Extra reservation for an SDK-style pre-existing hole. */
    CHECK(!ml_heap_next(&lab) && !lab.stage && MEM_getFree()==lab.baseline);
    fail_at=0;CHECK(ml_shutdown(&lab));MEM_free(barrier);
    CHECK(MEM_getFree()==base);
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    int error=!strcmp(argv[1],"fragments") ? check_fragments() : !strcmp(argv[1],"lifetimes") ? check_lifetimes() :
        !strcmp(argv[1],"failures") ? check_failures() : check_soak(500);
    if(invalid_frees)error=999;
    if(error)fprintf(stderr,"Memory contract failed at %d\n",error);
    return error ? 1 : 0;
}
