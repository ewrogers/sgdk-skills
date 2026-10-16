#include "panel.h"

bool panel_fits(u16 x, u16 y, u16 width, u16 height)
{
    return x < 40 && y < 28 && width >= 2 && height >= 2 && width <= 40-x && height <= 28-y;
}

u8 panel_piece(u16 x, u16 y, u16 width, u16 height)
{
    if (width < 2 || height < 2 || x >= width || y >= height) return 255;
    const u8 column=x == 0 ? 0 : x == width-1 ? 2 : 1;
    const u8 row=y == 0 ? 0 : y == height-1 ? 2 : 1;
    return row*3+column;
}
