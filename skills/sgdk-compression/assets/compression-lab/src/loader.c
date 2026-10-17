#include "loader.h"
void cl_init(CLLoader *l) {
    l->sample=l->codec=l->parts=l->sent=l->pending=l->decoded=l->ready=l->failed=l->active=0;
}
int cl_begin(CLLoader *l,unsigned short sample,unsigned short codec,unsigned short chunked) {
    if(l->active || l->pending || sample>1 || codec>2 || (chunked && codec!=2))return 0;
    l->sample=sample;l->codec=codec;l->parts=chunked ? CL_PARTS : 1;
    l->sent=l->pending=l->decoded=l->ready=l->failed=0;l->active=1;return 1;
}
void cl_step(CLLoader *l,CLDecode decode,CLQueue queue,void *ctx) {
    if(!l->active || l->pending || l->ready || l->failed)return;
    unsigned short offset=l->sent*CL_CHUNK;
    if(!l->decoded) {
        unsigned short bytes=l->parts==1 ? CL_BYTES : CL_CHUNK;
        unsigned char *out=(unsigned char *)l->data+(l->parts==1 ? 0 : offset);
        if(!decode(l->sample,l->codec,l->sent,l->parts!=1,out,bytes,ctx)) {
            l->failed=1;return;
        }
        l->decoded=1;
    }
    if(queue(l->data+offset/2,offset,CL_CHUNK,ctx))l->pending=1;
}
void cl_complete(CLLoader *l) {
    if(!l->pending)return;
    l->pending=0;l->sent++;
    if(l->parts!=1)l->decoded=0;
    if(l->sent==CL_PARTS)l->ready=1;
}
int cl_cancel(CLLoader *l) {
    if(l->pending)return 0;
    l->active=l->ready=l->failed=0;return 1;
}
int cl_take(CLLoader *l) {
    if(!l->ready || l->pending || l->failed)return 0;
    l->active=l->ready=0;return 1;
}
