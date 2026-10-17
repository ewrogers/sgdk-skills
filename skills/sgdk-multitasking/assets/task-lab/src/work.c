#include "work.h"
/* Single 68000 + exception scheduling. Volatile flags alone are not a lock. */
#define FENCE() __asm__ volatile ("" ::: "memory")
static void publish(MTWork *w,unsigned short state) { FENCE();w->state=state; }
void mt_init(MTWork *w) {
    w->cancel=w->progress=w->kind=w->variant=w->total=w->sent=w->pending=0;
    publish(w,MT_IDLE);
}
int mt_request(MTWork *w,unsigned short kind,unsigned short variant) {
    if(w->state!=MT_IDLE || kind>MT_RADAR || variant>1)return 0;
    w->kind=kind;w->variant=variant;w->total=kind==MT_DECODE?MT_PASSES:MT_SIDE;
    w->cancel=w->progress=w->sent=w->pending=0;
    publish(w,MT_REQUESTED);return 1;
}
static unsigned short noise(unsigned short x,unsigned short y,unsigned short seed) {
    unsigned int v=(unsigned int)x*374761393U+(unsigned int)y*668265263U+(unsigned int)seed*1274126177U;
    v=(v^(v>>13))*1274126177U;
    return (unsigned short)((v^(v>>16))>>24);
}
static unsigned short field(unsigned short x,unsigned short y,unsigned short spacing,unsigned short seed) {
    unsigned short gx=x/spacing,gy=y/spacing,fx=x%spacing,fy=y%spacing;
    unsigned int a=(unsigned int)noise(gx,gy,seed)*(spacing-fx)+(unsigned int)noise(gx+1,gy,seed)*fx;
    unsigned int b=(unsigned int)noise(gx,gy+1,seed)*(spacing-fx)+(unsigned int)noise(gx+1,gy+1,seed)*fx;
    return (a*(spacing-fy)+b*fy)/(spacing*spacing);
}
static unsigned short pixel(unsigned short x,unsigned short y,unsigned short variant) {
    unsigned short terrain=(field(x,y,32,15)+field(x,y,16,21))/2;
    unsigned short color=terrain>123?4:terrain>110?3:terrain>85?2:1;
    if(terrain>=119 && terrain<=123)color=5; /* visible coastline */
    int rain=0;
    for(unsigned short cell=0;cell<3;cell++) {
        int cx=24+cell*37+(variant?9:-3),cy=30+(cell*43)%83+(variant?-8:0);
        int dx=(int)x-cx,dy=(int)y-cy;
        int value=255-(dx*dx*2+dy*dy*3)/8;
        if(value>rain)rain=value;
    }
    rain+=(int)field(x,y,16,45+variant*3)/3-42;
    if(rain>45)color=6+(rain>90)+(rain>135)+(rain>180)+(rain>215)+(rain>240);
    /* Subtle map grid, visible only outside rain cells. */
    else if(x%32==0 || y%32==0)color=3;
    return color;
}
static void radar_row(MTWork *w,unsigned short y) {
    unsigned char *data=(unsigned char *)w->output;
    for(unsigned short x=0;x<MT_SIDE;x+=2) {
        unsigned short at=((y/8)*16+x/8)*32+(y%8)*4+(x%8)/2;
        data[at]=(unsigned char)(pixel(x,y,w->variant)*16+pixel(x+1,y,w->variant));
    }
}
void mt_step(MTWork *w,MTDecode decode,void *ctx) {
    if(w->state==MT_REQUESTED){FENCE();publish(w,MT_RUNNING);}
    if(w->state!=MT_RUNNING)return;
    if(w->cancel){publish(w,MT_CANCELLED);return;}
    if(w->kind==MT_DECODE) {
        if(!decode(w->variant,(unsigned char *)w->output,MT_BYTES,ctx)){publish(w,MT_FAILED);return;}
    } else radar_row(w,w->progress);
    w->progress++;
    if(w->cancel)publish(w,MT_CANCELLED);
    else if(w->progress==w->total)publish(w,MT_READY);
}
int mt_cancel(MTWork *w) {
    unsigned short state=w->state;
    if(state==MT_REQUESTED || state==MT_RUNNING){w->cancel=1;return 1;}
    if(state==MT_READY || (state==MT_UPLOADING && !w->pending)){publish(w,MT_IDLE);return 1;}
    return 0;
}
int mt_acknowledge(MTWork *w) {
    if(w->state!=MT_CANCELLED && w->state!=MT_FAILED)return 0;
    FENCE();publish(w,MT_IDLE);return 1;
}
void mt_upload_step(MTWork *w,MTQueue queue,void *ctx) {
    if(w->state==MT_READY){FENCE();publish(w,MT_UPLOADING);}
    if(w->state!=MT_UPLOADING || w->pending || w->sent==MT_BYTES)return;
    if(queue(w->output+w->sent/2,w->sent,MT_UPLOAD,ctx))w->pending=1;
}
int mt_upload_complete(MTWork *w) {
    if(w->state!=MT_UPLOADING || !w->pending)return 0;
    w->pending=0;w->sent+=MT_UPLOAD;
    if(w->sent<MT_BYTES)return 0;
    publish(w,MT_IDLE);return 1;
}
unsigned short mt_stack_used(const unsigned short *stack,unsigned short words) {
    unsigned short first=0;while(first<words && stack[first]==0xA55A)first++;
    return (words-first)*2;
}
int mt_stack_guard(const unsigned short *stack,unsigned short guard_words) {
    for(unsigned short i=0;i<guard_words;i++)if(stack[i]!=0xA55A)return 0;
    return 1;
}
