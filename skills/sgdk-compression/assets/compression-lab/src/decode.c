#include "catalog.h"
int cl_decode_asset(const CLAsset *a,u8 *out,u16 capacity,u16 expected) {
    if(!a || !out || !expected || capacity<expected || !a->stored || a->codec>2)return 0;
    if(a->codec==COMPRESSION_NONE) {
        if(a->stored!=expected)return 0;
        memcpy(out,a->data,expected);return 1;
    }
    return unpack(a->codec,(u8 *)a->data,out)==expected;
}
int cl_decode(u16 sample,u16 codec,u16 part,u16 chunked,u8 *out,u16 capacity,void *ctx) {
    (void)ctx;
    if(sample>1 || codec>2 || (chunked && (codec!=2 || part>=CL_PARTS)))return 0;
    return cl_decode_asset(chunked ? &cl_chunks[sample][part] : &cl_assets[sample][codec],
                           out,capacity,chunked ? CL_CHUNK : CL_BYTES);
}

int cl_matches_original(u16 sample,const u16 *data) {
    if(sample>1 || !data)return 0;
    const u8 *expected=cl_assets[sample][0].data;
    const u8 *actual=(const u8 *)data;
    for(u16 i=0;i<CL_BYTES;i++)if(actual[i]!=expected[i])return 0;
    return 1;
}
