#include <genesis.h>
#include "resources.h"
#include "stream_io.h"

_Static_assert(sizeof(stream_pages)==SC_PAGES*SC_PAGE_TILES*SC_TILE_BYTES,"Page catalog size changed");

bool sc_rom_read(void *context,u16 page,u16 offset,void *dest,u16 bytes)
{
    (void)context;
    if(page>=SC_PAGES || !dest || !bytes || bytes>SC_CHUNK_TILES*SC_TILE_BYTES ||
       (offset%SC_TILE_BYTES) || (bytes%SC_TILE_BYTES) || offset>SC_PAGE_TILES*SC_TILE_BYTES-bytes)return false;
    const u8 *source=stream_pages+(u32)page*SC_PAGE_TILES*SC_TILE_BYTES+offset;
    /* FAR_SAFE returns a temporary view, not an owned DMA source. Copy now.
     * Banking must be enabled in both game and SDK for a banked cartridge. */
    const void *view=FAR_SAFE(source,bytes);
    memcpy(dest,view,bytes);
    return true;
}

bool sc_queue_words(ScIO *io,u8 location,const void *source,u16 address,u16 words,u16 step)
{
    if(!io || !source || !words || words>SC_FRAME_BYTES/2 || io->used>io->limit ||
       words*2>io->limit-io->used || DMA_getQueueSize()>=DMA_getMaxQueueSize() || !DMA_canQueue(location,words))return false;
    if(!DMA_queueDma(location,(void *)source,address,words,step))return false;
    io->used+=words*2;return true;
}

bool sc_dma_tiles(void *context,const void *source,u16 tile,u16 tiles)
{
    if(!tiles || tiles>SC_CHUNK_TILES || tile<SC_CACHE_BASE ||
       tile+tiles>SC_CACHE_BASE+SC_SLOTS*SC_PAGE_TILES)return false;
    return sc_queue_words(context,DMA_VRAM,source,tile*SC_TILE_BYTES,tiles*SC_TILE_BYTES/2,2);
}
