#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "loader.h"
static CLLoader l;
static unsigned calls,queues,deny,fail,chunked;
static const uint16_t *borrowed;
static uint16_t snapshot[CL_CHUNK/2],vram[CL_BYTES/2];
static uint16_t offset;
static int decode(uint16_t sample,uint16_t codec,uint16_t part,uint16_t chunks,
                  uint8_t *out,uint16_t capacity,void *ctx) {
    (void)ctx;assert(sample==1);assert(codec==(chunked?2:1));assert(chunks==chunked);
    assert(!borrowed);assert(capacity==(chunks?512:4096));calls++;
    if(fail)return 0;
    for(unsigned i=0;i<capacity;i++)out[i]=(uint8_t)((chunks?part*512:0)+i+17);
    return 1;
}
static int queue(const uint16_t *data,uint16_t off,uint16_t bytes,void *ctx) {
    (void)ctx;assert(bytes==512);assert(off==queues*512);assert(!borrowed);
    if(deny)return 0;
    queues++;borrowed=data;offset=off;memcpy(snapshot,data,512);return 1;
}
static void reset(unsigned chunks) {
    cl_init(&l);calls=queues=deny=fail=0;chunked=chunks;borrowed=0;
    memset(vram,0,sizeof(vram));assert(cl_begin(&l,1,chunks?2:1,chunks));
}
int main(void) {
    for(unsigned chunks=0;chunks<2;chunks++) {
        reset(chunks);assert(!cl_take(&l));assert(!cl_begin(&l,0,0,0));
        for(unsigned p=0;p<8;p++) {
            deny=1;cl_step(&l,decode,queue,0);unsigned count=calls;
            cl_step(&l,decode,queue,0);assert(calls==count);assert(!l.pending);
            deny=0;cl_step(&l,decode,queue,0);assert(l.pending);assert(!cl_cancel(&l));
            assert(!cl_take(&l));cl_step(&l,decode,queue,0);assert(calls==count);
            assert(!memcmp(snapshot,borrowed,512));memcpy(vram+offset/2,borrowed,512);borrowed=0;
            cl_complete(&l);assert(l.sent==p+1);assert(l.ready==(p==7));
            cl_complete(&l);assert(l.sent==p+1);
        }
        assert(calls==(chunks?8:1));assert(queues==8);assert(cl_take(&l));assert(!cl_take(&l));
        for(unsigned i=0;i<4096;i++)assert(((uint8_t *)vram)[i]==(uint8_t)(i+17));
    }
    reset(1);fail=1;cl_step(&l,decode,queue,0);assert(l.failed && !queues && !l.ready);
    cl_step(&l,decode,queue,0);assert(calls==1);assert(cl_cancel(&l));
    assert(!cl_begin(&l,2,0,0));assert(!cl_begin(&l,0,3,0));assert(!cl_begin(&l,0,1,1));
    for(unsigned n=0;n<1000;n++) {
        assert(cl_begin(&l,1,2,1));deny=1;fail=0;cl_step(&l,decode,queue,0);
        assert(cl_cancel(&l));assert(!l.active && !l.pending && !l.ready);
    }
    return 0;
}
