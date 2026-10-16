#ifndef MEMORY_CHECKS_H
#define MEMORY_CHECKS_H
#include "memory_lab.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)

static int check_fragments(void)
{
    u16 before=MEM_getFree();MemoryLab lab;
    CHECK(ml_init(&lab));CHECK(before-lab.baseline==ML_SHARED+2);
    CHECK(ml_try_request(&lab) && MEM_getFree()==lab.baseline);
    CHECK(ml_heap_next(&lab) && lab.stage==1 && MEM_getLargestFreeBlock()==512);
    void *pinned=lab.blocks[1];
    CHECK(ml_heap_next(&lab) && lab.stage==2 && !lab.request_ok);
    CHECK(MEM_getFree()>ML_REQUEST && MEM_getLargestFreeBlock()==1026);
    CHECK(ml_heap_next(&lab) && lab.stage==3 && !lab.request_ok);
    CHECK(lab.blocks[1]==pinned && ml_intact(&lab));
    CHECK(ml_heap_next(&lab) && !lab.stage && lab.cycles==1);
    CHECK(MEM_getFree()==lab.baseline && ml_try_request(&lab));
    CHECK(ml_shutdown(&lab) && !lab.shared && MEM_getFree()==before);
    return 0;
}

static int check_lifetimes(void)
{
    u16 before=MEM_getFree();MemoryLab lab;CHECK(ml_init(&lab));
    CHECK(!ml_stage_begin(&lab));
    CHECK(ml_room_load(&lab));CHECK(!ml_room_load(&lab));
    CHECK(lab.baseline-MEM_getFree()==1540);
    void *shared=lab.shared,*room=lab.room[0];
    CHECK(ml_stage_begin(&lab) && !ml_stage_begin(&lab));
    void *borrowed=lab.staging;u16 free_before=MEM_getFree();
    for(u16 i=0;i<30;i++) {
        ml_stage_complete(&lab,FALSE);
        CHECK(lab.staging==borrowed && MEM_getFree()==free_before && ml_intact(&lab));
        CHECK(!ml_room_unload(&lab) && !ml_heap_clear(&lab) && !ml_shutdown(&lab));
    }
    CHECK(lab.shared==shared && lab.room[0]==room);
    ml_stage_complete(&lab,TRUE);CHECK(!lab.staging && MEM_getFree()==free_before+130);
    ml_stage_complete(&lab,TRUE); /* Repeated acknowledgment cannot double free. */
    CHECK(ml_room_unload(&lab) && !lab.room[0] && !lab.room[1]);
    CHECK(lab.shared==shared && MEM_getFree()==lab.baseline);
    CHECK(ml_room_unload(&lab) && ml_shutdown(&lab) && MEM_getFree()==before);
    return 0;
}

static int check_soak(u16 iterations)
{
    MemoryLab lab;u16 before=MEM_getFree();CHECK(ml_init(&lab));
    for(u16 i=0;i<iterations;i++) {
        for(u16 step=0;step<4;step++)CHECK(ml_heap_next(&lab));
        CHECK(ml_room_load(&lab) && ml_stage_begin(&lab));
        ml_stage_complete(&lab,FALSE);CHECK(!ml_room_unload(&lab));
        ml_stage_complete(&lab,TRUE);CHECK(ml_room_unload(&lab));
        CHECK(MEM_getFree()==lab.baseline && MEM_getLargestFreeBlock()==lab.largest_baseline);
    }
    CHECK(lab.cycles==iterations && lab.room_loads==iterations && ml_shutdown(&lab) && MEM_getFree()==before);
    return 0;
}
#endif
