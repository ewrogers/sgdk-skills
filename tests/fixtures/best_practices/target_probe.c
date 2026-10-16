#include <genesis.h>
#include "checked.h"
#include "upload.h"
/* Internal SDK startup/inspection symbols used only by this direct-entry test.
 * Applications use normal SGDK startup and do not access this buffer. */
extern void MEM_init(void);
extern u16 *dmaDataBuffer;
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)
_Static_assert(sizeof(int) == 4 && sizeof(long) == 4 && sizeof(void *) == 4, "selected ABI");
_Static_assert(sizeof(u16) == 2 && sizeof(u32) == 4, "selected SDK widths");

__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(bp_example_checks() == 0);
    MEM_init(); DMA_initEx(16,65535,2048); DMA_setAutoFlush(false);
    CHECK(dmaDataBuffer != NULL);
    u16 colors[16]; for (u16 i = 0; i < 16; i++) colors[i] = i * 2;
    SYS_setInterruptMaskLevel(2);
    CHECK(bp_queue_palette(3,colors));
    CHECK(SYS_getInterruptMaskLevel() == 2 && DMA_getQueueSize() == 1);
    colors[0] = 42;
    for (u16 i = 0; i < 16; i++) CHECK(dmaDataBuffer[i] == i * 2);
    SYS_disableInts(); CHECK(bp_queue_palette(0,colors));
    CHECK(SYS_getInterruptMaskLevel() == 7); SYS_enableInts();
    CHECK(SYS_getInterruptMaskLevel() == 2);
    CHECK(!bp_queue_palette(4,colors) && !bp_queue_palette(0,NULL) && DMA_getQueueSize() == 2);
    DMA_clearQueue();
    for (u16 i = 0; i < DMA_getMaxQueueSize(); i++)
        CHECK(DMA_queueDma(DMA_CRAM,colors,0,16,2));
    u16 count = DMA_getQueueSize();
    CHECK(!bp_queue_palette(0,colors));
    CHECK(DMA_getQueueSize() == count && SYS_getInterruptMaskLevel() == 2);
    DMA_clearQueue(); CHECK(bp_queue_palette(0,colors));
    DMA_clearQueue();
    u16 *reserved = DMA_allocateAndQueueDma(DMA_VRAM,0,DMA_getBufferSize()/2,2);
    CHECK(reserved != NULL);
    for (u16 i = 0; i < DMA_getBufferSize()/2; i++) reserved[i] = 0;
    CHECK(!bp_queue_palette(0,colors) && SYS_getInterruptMaskLevel() == 2 && DMA_getQueueSize() == 1);
    DMA_clearQueue(); CHECK(bp_queue_palette(0,colors));
    SYS_setInterruptMaskLevel(7);
    return 0;
}
