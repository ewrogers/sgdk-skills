#include <string.h>
#include "stream_cache.h"
enum { DMA_VRAM,DMA_CRAM };
const void *test_far(const void *source,u16 bytes);
#define FAR_SAFE(source,bytes) test_far(source,bytes)
u16 DMA_getQueueSize(void);
u16 DMA_getMaxQueueSize(void);
bool DMA_canQueue(u8 location,u16 words);
bool DMA_queueDma(u8 location,void *source,u16 to,u16 words,u16 step);
