#ifndef CL_LOADER_H
#define CL_LOADER_H
_Static_assert(sizeof(unsigned short)==2, "16-bit short required");
#define CL_BYTES 4096
#define CL_CHUNK 512
#define CL_PARTS 8
/* This Lab's resource contract, not an SGDK codec or asynchronous unpack API. */
typedef struct {
    unsigned short data[CL_BYTES/2];
    unsigned short sample, codec, parts, sent, pending, decoded, ready, failed;
    unsigned short active;
} CLLoader;
typedef int (*CLDecode)(unsigned short sample, unsigned short codec, unsigned short part,
                        unsigned short chunked, unsigned char *out, unsigned short capacity, void *ctx);
typedef int (*CLQueue)(const unsigned short *data, unsigned short offset, unsigned short bytes, void *ctx);
void cl_init(CLLoader *l);
int cl_begin(CLLoader *l, unsigned short sample, unsigned short codec, unsigned short chunked);
void cl_step(CLLoader *l, CLDecode decode, CLQueue queue, void *ctx);
void cl_complete(CLLoader *l);
int cl_cancel(CLLoader *l);
int cl_take(CLLoader *l);
#endif
