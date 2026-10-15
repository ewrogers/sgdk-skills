#ifndef RASTER_FAKE_GENESIS_H
#define RASTER_FAKE_GENESIS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#define TRUE true
#define FALSE false
#define HINTERRUPT_CALLBACK void
typedef enum { CPU = 0, DMA, DMA_QUEUE, DMA_QUEUE_COPY } TransferMethod;
typedef enum { BG_A = 0, BG_B = 1, WINDOW = 2 } VDPPlane;
#define VDP_WRITE_VSRAM_ADDR(adr) ((0x40000000u + ((u32)((adr) & 0x7F) << 16)) + 0x10)
#define VDP_VBLANK_FLAG (1 << 3)
#define VDP_HBLANK_FLAG (1 << 2)
#define VDP_FIFOEMPTY_FLAG (1 << 9)
#define VDP_WRITE_CRAM_ADDR(adr) (0xC0000000u | ((u32)((adr) & 0x3FFF) << 16) | (((adr) >> 14) & 3))
typedef void VoidCallback(void);
void VDP_setHInterrupt(bool enabled);
void VDP_setHIntCounter(u8 value);
void VDP_setAutoInc(u8 value);
void VDP_setVerticalScroll(VDPPlane plane, s16 value);
void VDP_setWindowVPos(u16 down, u16 position);
u8 VDP_getReg(u16 index);
void SYS_setHIntCallback(VoidCallback *callback);
void SYS_disableInts(void);
void SYS_enableInts(void);
void PAL_setColors(u16 index, const u16 *colors, u16 count, TransferMethod transfer);
#endif
