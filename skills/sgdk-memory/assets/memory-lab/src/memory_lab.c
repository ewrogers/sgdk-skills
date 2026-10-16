#include "memory_lab.h"

static void release(void **owner)
{
    if(*owner){MEM_free(*owner);*owner=NULL;}
}

static void paint(void *block,u16 bytes,u8 marker)
{
    u8 *p=block;for(u16 i=0;i<bytes;i++)p[i]=marker;
}

static bool marked(const void *block,u16 bytes,u8 marker)
{
    const u8 *p=block;for(u16 i=0;i<bytes;i++)if(p[i]!=marker)return FALSE;
    return TRUE;
}

bool ml_init(MemoryLab *lab)
{
    *lab=(MemoryLab){0};
    lab->shared=MEM_alloc(ML_SHARED);
    if(!lab->shared)return FALSE;
    paint(lab->shared,ML_SHARED,0x35);
    lab->baseline=MEM_getFree();lab->largest_baseline=MEM_getLargestFreeBlock();
    lab->healthy=TRUE;
    return TRUE;
}

bool ml_intact(const MemoryLab *lab)
{
    if(!lab->shared || !marked(lab->shared,ML_SHARED,0x35))return FALSE;
    for(u16 i=0;i<ML_BLOCKS;i++)
        if(lab->blocks[i] && !marked(lab->blocks[i],ML_BLOCK_BYTES,(u8)(i+1)))return FALSE;
    if(lab->room[0] && !marked(lab->room[0],1024,0x61))return FALSE;
    if(lab->room[1] && !marked(lab->room[1],512,0x72))return FALSE;
    if(lab->staging && !marked(lab->staging,ML_STAGE,0x55))return FALSE;
    return TRUE;
}

bool ml_heap_clear(MemoryLab *lab)
{
    if(lab->staging || lab->room[0] || lab->room[1])return FALSE;
    for(u16 i=0;i<ML_BLOCKS;i++)release(&lab->blocks[i]);
    for(u16 i=0;i<ML_GUARDS;i++)release(&lab->guards[i]);
    release(&lab->tail);MEM_pack();lab->stage=0;
    lab->healthy=lab->healthy && ml_intact(lab) && MEM_getFree()==lab->baseline &&
        MEM_getLargestFreeBlock()==lab->largest_baseline;
    return lab->healthy;
}

bool ml_try_request(MemoryLab *lab)
{
    void *trial=MEM_alloc(ML_REQUEST);
    lab->request_ok=trial!=NULL;
    if(trial){paint(trial,ML_REQUEST,0x26);MEM_free(trial);}
    else if(lab->failures<9999)lab->failures++;
    lab->healthy=lab->healthy && ml_intact(lab);
    return lab->request_ok;
}

bool ml_heap_next(MemoryLab *lab)
{
    if(lab->room[0] || lab->room[1] || lab->staging)return FALSE;
    switch(lab->stage) {
    case 0:
        for(u16 i=0;i<ML_BLOCKS;i++) {
            lab->blocks[i]=MEM_alloc(ML_BLOCK_BYTES);
            if(!lab->blocks[i]){ml_heap_clear(lab);return FALSE;}
            paint(lab->blocks[i],ML_BLOCK_BYTES,(u8)(i+1));
        }
        /* Reserve the tail so a large free tail cannot hide the separated holes. */
        u16 largest=MEM_getLargestFreeBlock();
        if(largest<=514){ml_heap_clear(lab);return FALSE;}
        lab->tail=MEM_alloc(largest-514); /* Header included: leaves 512 allocator bytes. */
        if(!lab->tail){ml_heap_clear(lab);return FALSE;}
        /* SDK initialization can leave other holes. Reserve those too, without
           touching its live allocations, so they cannot satisfy the demo request. */
        u16 guard=0;
        while((largest=MEM_getLargestFreeBlock())>512) {
            if(guard==ML_GUARDS){ml_heap_clear(lab);return FALSE;}
            lab->guards[guard]=MEM_alloc(largest-2);
            if(!lab->guards[guard]){ml_heap_clear(lab);return FALSE;}
            guard++;
        }
        lab->stage=1;break;
    case 1:
        for(u16 i=0;i<ML_BLOCKS;i+=2)release(&lab->blocks[i]);
        lab->stage=2;ml_try_request(lab);break;
    case 2:
        MEM_pack();lab->stage=3;ml_try_request(lab);break;
    default:
        if(!ml_heap_clear(lab))return FALSE;
        if(lab->cycles<9999)lab->cycles++;
        break;
    }
    lab->healthy=lab->healthy && ml_intact(lab);
    return lab->healthy;
}

bool ml_room_load(MemoryLab *lab)
{
    if(lab->stage || lab->room[0] || lab->room[1] || lab->staging)return FALSE;
    void *first=MEM_alloc(1024);if(!first)return FALSE;
    void *second=MEM_alloc(512);
    if(!second){MEM_free(first);return FALSE;} /* No half-loaded room. */
    paint(first,1024,0x61);paint(second,512,0x72);
    lab->room[0]=first;lab->room[1]=second;
    if(lab->room_loads<9999)lab->room_loads++;
    return TRUE;
}

bool ml_room_unload(MemoryLab *lab)
{
    if(lab->staging || lab->stage)return FALSE; /* Drain borrowed source before releasing this owner. */
    release(&lab->room[1]);release(&lab->room[0]);MEM_pack();
    lab->healthy=lab->healthy && ml_intact(lab) && MEM_getFree()==lab->baseline;
    return lab->healthy;
}

bool ml_stage_begin(MemoryLab *lab)
{
    if(!lab->room[0] || lab->staging)return FALSE;
    lab->staging=MEM_alloc(ML_STAGE);if(!lab->staging)return FALSE;
    paint(lab->staging,ML_STAGE,0x55);return TRUE;
}

void ml_stage_complete(MemoryLab *lab,bool drained)
{
    if(drained)release(&lab->staging);
}

bool ml_shutdown(MemoryLab *lab)
{
    if(lab->staging)return FALSE;
    if(lab->stage)ml_heap_clear(lab);
    ml_room_unload(lab);ml_heap_clear(lab);release(&lab->shared);
    return lab->healthy;
}
