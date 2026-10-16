#include "upload.h"

bool bp_queue_palette(u16 slot, const u16 colors[16])
{
    if (slot >= 4 || !colors) return false;
    /* Copy 32 bytes while the queue cannot be flushed by an interrupt.
     * The SDK pairs are nestable in foreground code, preserving the mask. */
    SYS_disableInts();
    const bool accepted = DMA_copyAndQueueDma(DMA_CRAM, (void *)colors,
                                             slot * 32, 16, 2);
    SYS_enableInts();
    return accepted;
}
