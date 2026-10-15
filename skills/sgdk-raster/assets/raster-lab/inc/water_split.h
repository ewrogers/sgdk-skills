#ifndef RASTER_LAB_WATER_SPLIT_H
#define RASTER_LAB_WATER_SPLIT_H
#include <genesis.h>

/* Owns H/V interrupts, PAL0/PAL1 and R15. Publish a complete frame before
   its VBlank transfer; the V interrupt restores dry colors before DMA work. */
void water_split_init(const u16 *dry, const u16 *wet);
void water_split_publish(u16 line, bool enabled);
void water_split_prepare(u16 line, bool enabled);
void water_split_stop(void);
u16 water_split_pair_count(void);
u16 water_split_late(void);
u16 water_split_timeouts(void);
u16 water_split_last_line(void);
u16 water_split_setup_late(void);
HINTERRUPT_CALLBACK water_split_interrupt(void);
#endif
