#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <genesis.h>
#include "view.h"

const TileSet lab_font = {0, 96, 0}, font_default = {0, 96, 0};
static char cells[3][32][65];
static u16 attrs[3][32][64];
static unsigned writes, clears, fonts, palettes;
static bool enabled;
static s16 scroll;
static const TileSet *font;

void VDP_drawTextEx(VDPPlane plane, const char *s, u16 attr, u16 x, u16 y, TransferMethod tm)
{
    size_t n = strlen(s);
    assert(x + n <= 40 && y < 28 && tm == CPU);
    assert((attr & 0x1800) == 0 && (attr & 0x8000));
    for (size_t i = 0; i < n; i++) {
        assert((unsigned char)s[i] >= 32 && (unsigned char)s[i] <= 126);
        cells[plane][y][x+i] = s[i]; attrs[plane][y][x+i] = attr;
    }
    writes++;
}
void VDP_setEnable(bool value) { enabled = value; }
void VDP_clearPlane(VDPPlane p, bool wait)
{
    assert(!enabled && wait); clears++;
    memset(cells[p], ' ', sizeof(cells[p]));
}
void VDP_setHorizontalScroll(VDPPlane plane, s16 value) { assert(plane == BG_B); scroll = value; }
void VDP_fillTileMapRect(VDPPlane plane, u16 tile, u16 x, u16 y, u16 w, u16 h)
{ assert(plane == BG_B && tile == TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, TILE_USER_INDEX));
  assert(x == 0 && y == 4 && w == 64 && h == 24); }
void VDP_setScreenWidth320(void) {}
void VDP_setScreenHeight224(void) {}
void VDP_setPlaneSize(u16 w, u16 h, bool setup) { assert(w == 64 && h == 32 && setup); }
void VDP_setWindowHPos(u16 right, u16 pos) { assert(!right && pos == 0); }
void VDP_setWindowVPos(u16 down, u16 pos) { assert(!down && pos == 4); }
void PAL_setColor(u16 index, u16 value)
{ assert(!enabled && (index == 0 || index == 15 || index == 31 || index == 33)); (void)value; palettes++; }
void VDP_loadTileData(const u32 *data, u16 index, u16 count, TransferMethod tm)
{ assert(!enabled && data && index == TILE_USER_INDEX && count == 1 && tm == DMA); }
u16 VDP_loadFont(const TileSet *value, TransferMethod tm)
{ assert(!enabled && tm == DMA); font = value; fonts++; return TRUE; }

int main(void)
{
    Menu m;
    unsigned old;
    menu_init(&m); view_init(); view_draw(&m);
    assert(enabled && palettes == 4 && fonts == 1 && font == &lab_font && clears == 3);
    assert(!memcmp(&cells[WINDOW][1][2], "SGDK MENU LAB", 13));
    assert(cells[BG_A][11][4] == '>' && attrs[BG_A][11][7] == TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    old = writes; view_draw(&m); assert(writes == old);
    m.selected = 1; view_draw(&m);
    assert(cells[BG_A][11][4] == ' ' && cells[BG_A][14][4] == '>');
    assert(clears == 3 && palettes == 4);
    m.screen = MENU_OPTIONS; m.selected = 0; m.difficulty = 1; view_draw(&m);
    assert(!memcmp(&cells[BG_A][11][22], "NORMAL", 6));
    m.difficulty = 0; view_draw(&m);
    assert(!memcmp(&cells[BG_A][11][22], "EASY           ", 15));
    m.custom_font = false; view_draw(&m);
    assert(fonts == 2 && font == &font_default && enabled);
    m.screen = MENU_PLAY; m.score = 100000; m.remaining_ms = 12000;
    m.marker_x = 20; m.marker_y = 15; view_draw(&m);
    assert(!memcmp(&cells[WINDOW][2][9], "100000", 6));
    assert(!memcmp(&cells[WINDOW][2][31], "12", 2));
    old = writes; m.scroll = 99; m.remaining_ms = 11983; view_draw(&m);
    assert(writes == old && scroll == -99);
    m.remaining_ms = 10999; m.score = 0; m.marker_x++; view_draw(&m);
    assert(!memcmp(&cells[WINDOW][2][9], "000000", 6));
    assert(!memcmp(&cells[WINDOW][2][31], "11", 2));
    assert(cells[BG_A][15][20] == ' ' && cells[BG_A][15][21] == '+');
    m.screen = MENU_PAUSE; view_draw(&m); old = writes;
    view_draw(&m); assert(writes == old && scroll == 0);
    m.screen = MENU_RESULTS; m.score = 100; view_draw(&m);
    assert(!memcmp(&cells[BG_A][8][7], "000100", 6));
    m.screen = MENU_CREDITS; view_draw(&m);
    assert(!memcmp(&cells[WINDOW][1][2], "CREDITS", 7));
    puts("view");
    return 0;
}
