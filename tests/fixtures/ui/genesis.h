#ifndef GENESIS_H
#define GENESIS_H
#include <stdbool.h>
#include <stdint.h>
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef enum { BG_A, BG_B, WINDOW } VDPPlane;
typedef enum { CPU, DMA, DMA_QUEUE, DMA_QUEUE_COPY } TransferMethod;
typedef struct { u16 compression, numTile; const u32 *tiles; } TileSet;
#define TRUE true
#define FALSE false
#define PAL0 0
#define PAL1 1
#define PAL2 2
#define TILE_USER_INDEX 16
#define TILE_ATTR(p, pr, vf, hf) (((p) << 13) | ((pr) << 15) | ((vf) << 12) | ((hf) << 11))
#define TILE_ATTR_FULL(p, pr, vf, hf, tile) (TILE_ATTR(p, pr, vf, hf) | (tile))
/* SGDK rounds each 8-bit channel before selecting its three VDP bits. */
static inline u16 test_color(u32 c)
{
    u32 r = ((c >> 16) & 255) + 16;
    u32 g = ((c >> 8) & 255) + 16;
    u32 b = (c & 255) + 16;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (u16)(((r >> 4) & 14) | (g & 224) | ((b << 4) & 3584));
}
#define RGB24_TO_VDPCOLOR(c) test_color(c)
void VDP_drawTextEx(VDPPlane, const char *, u16, u16, u16, TransferMethod);
void VDP_setEnable(bool);
void VDP_clearPlane(VDPPlane, bool);
void VDP_setHorizontalScroll(VDPPlane, s16);
void VDP_fillTileMapRect(VDPPlane, u16, u16, u16, u16, u16);
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16, u16, bool);
void VDP_setWindowHPos(u16, u16);
void VDP_setWindowVPos(u16, u16);
void PAL_setColor(u16, u16);
void VDP_loadTileData(const u32 *, u16, u16, TransferMethod);
u16 VDP_loadFont(const TileSet *, TransferMethod);
#endif
