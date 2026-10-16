#ifndef MEMORY_LAB_H
#define MEMORY_LAB_H
#include <genesis.h>

enum { ML_BLOCKS=6, ML_GUARDS=16, ML_BLOCK_BYTES=1024, ML_REQUEST=1536, ML_SHARED=512, ML_STAGE=128 };
typedef struct {
    void *shared,*blocks[ML_BLOCKS],*tail,*guards[ML_GUARDS],*room[2],*staging;
    u16 baseline,largest_baseline,stage,failures,cycles,room_loads;
    bool request_ok,healthy;
} MemoryLab;

bool ml_init(MemoryLab *lab);
bool ml_heap_next(MemoryLab *lab);
bool ml_try_request(MemoryLab *lab);
bool ml_heap_clear(MemoryLab *lab);
bool ml_room_load(MemoryLab *lab);
bool ml_room_unload(MemoryLab *lab);
bool ml_stage_begin(MemoryLab *lab);
void ml_stage_complete(MemoryLab *lab,bool drained);
bool ml_shutdown(MemoryLab *lab);
bool ml_intact(const MemoryLab *lab);
#endif
