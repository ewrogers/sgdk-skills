#ifndef STREAM_IO_H
#define STREAM_IO_H
#include "stream_cache.h"
enum { SC_FRAME_BYTES=1536 };
typedef struct { u16 used,limit; } ScIO;
bool sc_rom_read(void *context,u16 page,u16 offset,void *dest,u16 bytes);
bool sc_dma_tiles(void *context,const void *source,u16 tile,u16 tiles);
bool sc_queue_words(ScIO *io,u8 location,const void *source,u16 address,u16 words,u16 step);
#endif
