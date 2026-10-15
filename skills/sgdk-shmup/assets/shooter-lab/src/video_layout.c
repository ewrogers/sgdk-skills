#include "game.h"

static bool disjoint(u16 a,u32 size_a,u16 b,u32 size_b)
{
    return (u32)a+size_a<=b || (u32)b+size_b<=a;
}

bool game_video_layout_valid(u16 width,u16 height,u16 a,u16 b,u16 window)
{
    /* H40 Window is always 64x32 entries and its base is 4 KiB aligned,
     * independently of the scrolling planes' dimensions. */
    if ((width!=32 && width!=64 && width!=128) ||
        (height!=32 && height!=64 && height!=128) ||
        (width==128 && height!=32) || (height==128 && width!=32)) return false;
    u32 size=(u32)width*height*2;
    if ((a&0x3FF) || (b&0x1FFF) || (window&0xFFF)) return false;
    if ((u32)a+size>65536UL || (u32)b+size>65536UL || (u32)window+4096>65536UL) return false;
    return disjoint(a,size,b,size) && disjoint(a,size,window,4096) && disjoint(b,size,window,4096);
}
