#ifndef MT_WORK_H
#define MT_WORK_H
_Static_assert(sizeof(unsigned short)==2 && sizeof(int)==4,"16-bit word / 32-bit int required");
#define MT_BYTES 8192
#define MT_SIDE 128
#define MT_PASSES 8
#define MT_UPLOAD 512
/* Lab mailbox, not a general C threading primitive or SDK scheduler. */
enum { MT_IDLE,MT_REQUESTED,MT_RUNNING,MT_READY,MT_UPLOADING,MT_CANCELLED,MT_FAILED };
enum { MT_DECODE,MT_RADAR };
typedef struct {
    volatile unsigned short state,cancel,progress;
    unsigned short kind,variant,total,sent,pending;
    unsigned short output[MT_BYTES/2];
} MTWork;
typedef int (*MTDecode)(unsigned short variant,unsigned char *out,unsigned short capacity,void *ctx);
typedef int (*MTQueue)(const unsigned short *data,unsigned short offset,unsigned short bytes,void *ctx);
void mt_init(MTWork *w);
int mt_request(MTWork *w,unsigned short kind,unsigned short variant);
void mt_step(MTWork *w,MTDecode decode,void *ctx);
int mt_cancel(MTWork *w);
int mt_acknowledge(MTWork *w);
void mt_upload_step(MTWork *w,MTQueue queue,void *ctx);
int mt_upload_complete(MTWork *w);
unsigned short mt_stack_used(const unsigned short *stack,unsigned short words);
int mt_stack_guard(const unsigned short *stack,unsigned short guard_words);
#endif
