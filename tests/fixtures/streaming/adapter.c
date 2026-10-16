#include <assert.h>
#include <genesis.h>
#include "stream_io.h"
#include "resources.h"
const u8 stream_pages[12288]={0};
static u8 mapping[256];
static const u8 *borrowed;
static u16 queue_count,queue_max=64,queued_bytes,words_seen,to_seen,step_seen;
static bool reject;
const void *test_far(const void *source,u16 bytes)
{
    assert(bytes>0 && bytes<=sizeof(mapping));
    unsigned offset=(unsigned)((const u8 *)source-stream_pages);
    for(u16 i=0;i<bytes;i++)mapping[i]=(u8)(offset+i+7);
    return mapping;
}
u16 DMA_getQueueSize(void){return queue_count;}
u16 DMA_getMaxQueueSize(void){return queue_max;}
bool DMA_canQueue(u8 location,u16 words){(void)location;return queued_bytes+words*2<=SC_FRAME_BYTES;}
bool DMA_queueDma(u8 location,void *source,u16 to,u16 words,u16 step)
{
    assert(location==DMA_VRAM);
    if(reject)return false;
    borrowed=source;words_seen=words;to_seen=to;step_seen=step;
    queue_count++;queued_bytes+=words*2;return true;
}

int main(void)
{
    StreamCache c;sc_init(&c);ScIO io={0,SC_FRAME_BYTES};
    assert(!sc_rom_read(&io,24,0,c.staging,32));
    assert(!sc_rom_read(&io,0,1,c.staging,32));
    assert(!sc_rom_read(&io,0,480,c.staging,64));
    assert(!sc_rom_read(&io,0,0,c.staging,0));
    assert(!sc_rom_read(&io,0,0,c.staging,288));
    assert(!sc_dma_tiles(&io,c.staging,SC_CACHE_BASE-1,1));
    assert(!sc_dma_tiles(&io,c.staging,SC_CACHE_BASE+SC_SLOTS*16-1,2));
    assert(sc_require(&c,3)==SC_WAIT);
    io.used=SC_FRAME_BYTES-128;
    assert(!sc_step(&c,8,sc_rom_read,sc_dma_tiles,&io) && !c.completed && !c.flight && !queue_count);
    io.used=0;queue_count=queue_max;
    assert(!sc_step(&c,8,sc_rom_read,sc_dma_tiles,&io) && !c.flight);
    queue_count=0;reject=true;
    assert(!sc_step(&c,8,sc_rom_read,sc_dma_tiles,&io) && !c.flight);
    reject=false;
    assert(sc_step(&c,8,sc_rom_read,sc_dma_tiles,&io)==8);
    assert(words_seen==128 && to_seen==SC_CACHE_BASE*32 && step_seen==2 && io.used==256);
    /* An unrelated FAR call invalidates every byte in the temporary view. The
     * queued source must still be the cache's owned RAM, not this mapping. */
    memset(mapping,0xEE,sizeof(mapping));
    assert(borrowed==(const u8 *)c.staging && borrowed!=mapping);
    for(u16 i=0;i<256;i++)assert(borrowed[i]==(u8)(i+7));
    u16 tile;assert(!sc_tile(&c,3,0,&tile) && !sc_reset(&c));
    sc_complete(&c,false);assert(c.flight==8);
    sc_complete(&c,true);assert(c.completed==8 && !sc_tile(&c,3,0,&tile));
    return 0;
}
