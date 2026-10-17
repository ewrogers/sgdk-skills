#ifndef CL_CATALOG_H
#define CL_CATALOG_H
#include <genesis.h>
#include "loader.h"
typedef struct { const u8 *data; u32 stored; u16 codec; } CLAsset;
extern const CLAsset cl_assets[2][3];
extern const CLAsset cl_chunks[2][8];
extern const u16 cl_palette[16];
/* Known, build-generated assets only. SDK decoders are not hostile-input parsers. */
int cl_decode_asset(const CLAsset *a, u8 *out, u16 capacity, u16 expected);
int cl_decode(u16 sample,u16 codec,u16 part,u16 chunked,u8 *out,u16 capacity,void *ctx);
#endif
