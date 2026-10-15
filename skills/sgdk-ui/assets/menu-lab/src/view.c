#include <genesis.h>
#include <libres.h>
#include "resources.h"
#include "view.h"

static Menu previous;
static bool drawn;
static const u32 grid_tile[8] = {
    0x11111111, 0x10000000, 0x10000000, 0x10000000,
    0x10000000, 0x10000000, 0x10000000, 0x10000000
};

static void field(VDPPlane plane, u16 palette, u16 x, u16 y,
                  const char *text, u16 width)
{
    char cells[39];
    if (width > 38 || x + width > 40 || y >= 28) return;
    if (!menu_field(cells, sizeof(cells), text, width)) return;
    VDP_drawTextEx(plane, cells, TILE_ATTR(palette, TRUE, FALSE, FALSE), x, y, CPU);
}

static void number(u32 value, u16 x, u16 y, u16 width)
{
    char cells[11];
    if (menu_number(cells, sizeof(cells), value, width))
        field(WINDOW, PAL0, x, y, cells, width);
}

static void heading(const char *text)
{
    field(WINDOW, PAL1, 2, 1, text, 36);
}

static void choices(const Menu *menu)
{
    static const char *const title[] = {"PLAY", "OPTIONS", "CREDITS"};
    static const char *const pause[] = {"RESUME", "RESTART ROUND", "TITLE SCREEN"};
    static const char *const results[] = {"PLAY AGAIN", "TITLE SCREEN"};
    u16 count = menu->screen == MENU_RESULTS ? 2 : 3;
    u16 i;
    for (i = 0; i < count; i++) {
        const char *label;
        if (menu->screen == MENU_OPTIONS) {
            static const char *const difficulty[] = {"EASY", "NORMAL", "HARD"};
            label = i == 0 ? difficulty[menu->difficulty] :
                    i == 1 ? (menu->custom_font ? "LAB FONT" : "SGDK FONT") : "BACK";
            field(BG_A, PAL0, 7, 11 + i * 3, i == 0 ? "DIFFICULTY" : i == 1 ? "TYPEFACE" : "", 12);
        } else label = menu->screen == MENU_TITLE ? title[i] :
                         menu->screen == MENU_PAUSE ? pause[i] : results[i];
        field(BG_A, PAL1, 4, 11 + i * 3, i == menu->selected ? ">" : "", 1);
        field(BG_A, i == menu->selected ? PAL1 : PAL0,
              menu->screen == MENU_OPTIONS ? 22 : 7, 11 + i * 3, label,
              menu->screen == MENU_OPTIONS ? 15 : 30);
    }
}

static void screen(const Menu *menu)
{
    /* Large changes are rare. Blank while clearing/rebuilding the screen. */
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_clearPlane(WINDOW, TRUE);
    VDP_setHorizontalScroll(BG_B, 0);
    if (menu->screen == MENU_PLAY) {
        VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 4, 64, 24);
        heading("MENU LAB / HUD");
        field(WINDOW, PAL1, 2, 2, "SCORE", 5);
        field(WINDOW, PAL1, 25, 2, "TIME", 4);
        field(BG_A, PAL0, 2, 25, "A: SCORE   B: CLEAR   START: PAUSE", 36);
    } else if (menu->screen == MENU_TITLE) {
        heading("SGDK MENU LAB");
        field(BG_A, PAL0, 7, 6, "TEXT, MENUS AND A FIXED HUD", 30);
        field(BG_A, PAL0, 2, 25, "D-PAD: CHOOSE   A / START: SELECT", 36);
    } else if (menu->screen == MENU_OPTIONS) {
        heading("OPTIONS");
        field(BG_A, PAL0, 2, 6, "HOLD A DIRECTION TO REPEAT", 36);
        field(BG_A, PAL0, 2, 25, "LEFT / RIGHT / A: CHANGE   B: BACK", 36);
    } else if (menu->screen == MENU_CREDITS) {
        heading("CREDITS");
        field(BG_A, PAL0, 5, 10, "ORIGINAL MENU LAB AND PIXEL FONT", 33);
        field(BG_A, PAL1, 5, 13, "SGDK SKILLS / MIT LICENSE", 33);
        field(BG_A, PAL0, 5, 18, "BUILT WITH SGDK", 33);
        field(BG_A, PAL0, 2, 25, "A / B / START: BACK", 36);
    } else if (menu->screen == MENU_PAUSE) {
        heading("PAUSED");
        field(BG_A, PAL0, 7, 6, "ROUND TIME IS FROZEN", 30);
        field(BG_A, PAL0, 2, 25, "A: SELECT   B / START: RESUME", 36);
    } else {
        heading("ROUND COMPLETE");
        field(BG_A, PAL0, 7, 6, "FINAL SCORE", 30);
        {
            char score[7];
            menu_number(score, sizeof(score), menu->score, 6);
            field(BG_A, PAL1, 7, 8, score, 6);
        }
        field(BG_A, PAL0, 2, 25, "A / START: SELECT", 36);
    }
    VDP_setEnable(TRUE);
}

void view_init(void)
{
    drawn = false;
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 4);
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x101018));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xCCDDEE));
    PAL_setColor(31, RGB24_TO_VDPCOLOR(0x88BBFF));
    PAL_setColor(33, RGB24_TO_VDPCOLOR(0x222233));
    VDP_loadTileData(grid_tile, TILE_USER_INDEX, 1, DMA);
}

void view_draw(const Menu *menu)
{
    bool changed = !drawn || previous.screen != menu->screen;
    if (!drawn || previous.custom_font != menu->custom_font) {
        VDP_setEnable(FALSE);
        VDP_loadFont(menu->custom_font ? &lab_font : &font_default, DMA);
        VDP_setEnable(TRUE);
    }
    if (changed) screen(menu);
    if (menu->screen == MENU_PLAY) {
        if (changed || previous.score != menu->score) number(menu->score, 9, 2, 6);
        if (changed || (previous.remaining_ms + 999) / 1000 != (menu->remaining_ms + 999) / 1000)
            number((menu->remaining_ms + 999) / 1000, 31, 2, 2);
        if (changed || previous.marker_x != menu->marker_x || previous.marker_y != menu->marker_y) {
            if (!changed) field(BG_A, PAL0, previous.marker_x, previous.marker_y, "", 1);
            field(BG_A, PAL1, menu->marker_x, menu->marker_y, "+", 1);
        }
        VDP_setHorizontalScroll(BG_B, -(s16)menu->scroll);
    } else if (menu->screen != MENU_CREDITS &&
               (changed || previous.selected != menu->selected ||
                previous.difficulty != menu->difficulty || previous.custom_font != menu->custom_font))
        choices(menu);
    previous = *menu;
    drawn = true;
}
