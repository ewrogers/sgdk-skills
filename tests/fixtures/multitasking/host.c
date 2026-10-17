#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "work.h"
static struct {unsigned short lo[8];MTWork work;unsigned short hi[8];} box;
static unsigned calls,fail,cancel_inside,deny,queues;
static const unsigned short *borrowed;
static unsigned char loan[512],vram[MT_BYTES];
static MTWork *w=&box.work;
static int decode(unsigned short variant,unsigned char *out,unsigned short capacity,void *ctx) {
    (void)ctx;assert(capacity==MT_BYTES);calls++;
    for(unsigned i=0;i<capacity;i++)out[i]=(unsigned char)(variant*7+i);
    if(cancel_inside)assert(mt_cancel(w));
    return !fail;
}
static int queue(const unsigned short *data,unsigned short offset,unsigned short bytes,void *ctx) {
    (void)ctx;assert(bytes==512);assert(offset==queues*512);assert(!borrowed);
    if(deny)return 0;
    borrowed=data;memcpy(loan,data,512);queues++;return 1;
}
static void reset(void) {
    memset(&box,0,sizeof(box));for(unsigned i=0;i<8;i++)box.lo[i]=box.hi[i]=0xA55A;
    mt_init(w);calls=fail=cancel_inside=deny=queues=0;borrowed=0;
}
static void guards(void){for(unsigned i=0;i<8;i++)assert(box.lo[i]==0xA55A && box.hi[i]==0xA55A);}
static void core(void) {
    reset();assert(!mt_request(w,2,0));assert(!mt_request(w,0,2));
    assert(mt_request(w,MT_DECODE,1));MTWork saved=*w;
    assert(!mt_request(w,MT_RADAR,0));assert(!memcmp(w,&saved,sizeof(saved)));
    for(unsigned i=0;i<MT_PASSES;i++) {
        mt_upload_step(w,queue,0);assert(!queues);mt_step(w,decode,0);
        assert(w->progress==i+1);assert(w->state==(i+1==MT_PASSES?MT_READY:MT_RUNNING));
    }
    assert(calls==8);mt_step(w,decode,0);assert(calls==8);
    for(unsigned part=0;part<16;part++) {
        deny=1;mt_upload_step(w,queue,0);assert(!w->pending);assert(w->sent==part*512);
        deny=0;mt_upload_step(w,queue,0);assert(w->pending);assert(!mt_cancel(w));
        assert(!mt_request(w,MT_RADAR,0));mt_step(w,decode,0);assert(calls==8);
        assert(!memcmp(loan,borrowed,512));memcpy(vram+part*512,borrowed,512);borrowed=0;
        assert(mt_upload_complete(w)==(part==15));assert(!mt_upload_complete(w));
        assert(w->sent==(part+1)*512);
    }
    assert(w->state==MT_IDLE);for(unsigned i=0;i<MT_BYTES;i++)assert(vram[i]==(unsigned char)(i+7));guards();
    reset();assert(mt_request(w,MT_DECODE,0));assert(mt_cancel(w));mt_step(w,decode,0);
    assert(w->state==MT_CANCELLED && !calls);assert(mt_acknowledge(w));
    assert(mt_request(w,MT_DECODE,0));cancel_inside=1;mt_step(w,decode,0);
    assert(w->state==MT_CANCELLED && calls==1);assert(!mt_request(w,MT_RADAR,0));assert(mt_acknowledge(w));
    cancel_inside=0;fail=1;assert(mt_request(w,MT_DECODE,0));mt_step(w,decode,0);
    assert(w->state==MT_FAILED);mt_upload_step(w,queue,0);assert(!queues);assert(mt_acknowledge(w));
    for(unsigned n=0;n<200;n++) {
        assert(mt_request(w,MT_RADAR,n%2));mt_step(w,decode,0);assert(w->progress==1);
        /* Withheld scheduling leaves the output and progress untouched. */
        saved=*w;for(unsigned i=0;i<100;i++)assert(!mt_request(w,MT_DECODE,0));
        assert(!memcmp(w,&saved,sizeof(saved)));assert(mt_cancel(w));mt_step(w,decode,0);
        assert(w->state==MT_CANCELLED);assert(mt_acknowledge(w));guards();
    }
    reset();assert(mt_request(w,MT_DECODE,0));for(unsigned i=0;i<8;i++)mt_step(w,decode,0);
    assert(mt_cancel(w));assert(w->state==MT_IDLE);
    assert(mt_request(w,MT_DECODE,0));for(unsigned i=0;i<8;i++)mt_step(w,decode,0);
    deny=1;mt_upload_step(w,queue,0);assert(w->state==MT_UPLOADING && !w->pending);
    assert(mt_cancel(w));assert(w->state==MT_IDLE);guards();
    unsigned short stack[1024];for(unsigned i=0;i<1024;i++)stack[i]=0xA55A;
    assert(!mt_stack_used(stack,1024));assert(mt_stack_guard(stack,32));
    stack[974]=1;assert(mt_stack_used(stack,1024)==100);stack[4]=0;
    assert(!mt_stack_guard(stack,32));
}
int main(int argc,char **argv) {
    if(argc==1){core();return 0;}
    reset();unsigned variant=argv[1][0]=='1';assert(mt_request(w,MT_RADAR,variant));
    for(unsigned row=0;row<128;row++){mt_step(w,decode,0);guards();assert(w->progress==row+1);}
    assert(w->state==MT_READY);assert(fwrite(w->output,1,MT_BYTES,stdout)==MT_BYTES);return 0;
}
