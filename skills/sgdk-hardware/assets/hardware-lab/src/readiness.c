#include <genesis.h>
#include "hardware.h"
#include "readiness.h"
#include "readiness_art.h"
#include "build_id.h"

#define READY_TILE_START 19
_Static_assert((READY_TILE_START + HL_READY_TILE_COUNT) * 32 <= HL_UPLOAD_START,
               "Readiness art must stay below the Lab's private DMA workload range");

/* All CRAM components use the Mega Drive's even 3-bit color steps. */
static const u16 ready_colors[16] = {
    0x200, 0x200, 0x422, 0x644, 0x8e4, 0xea6, 0xa88, 0xeee,
    0x4ae, 0x44e, 0x422, 0x644, 0xa88, 0x8e4, 0xea6, 0xeee
};

static void ready_text(const char *value, u16 x, u16 y, u16 pal)
{
    /* Stack-formatted counters remain valid after this function returns. */
    VDP_drawTextEx(BG_A, value, TILE_ATTR(pal, TRUE, FALSE, FALSE), x, y, DMA_QUEUE_COPY);
}

static void ready_hex(u32 value, u16 digits, u16 x, u16 y, u16 pal)
{
    /* SGDK's general formatter permits up to 15 padded digits plus NUL. */
    char text[32];
    intToHex(value, text, digits);
    ready_text(text, x, y, pal);
}

static void art(u16 offset, u16 width, u16 height, u16 x, u16 y)
{
    for (u16 row = 0; row < height; row++)
        for (u16 col = 0; col < width; col++)
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                                READY_TILE_START + offset + row * width + col), x + col, y + row);
}

static void solid(u16 color, u16 x, u16 y, u16 width, u16 height)
{
    for (u16 row = 0; row < height; row++)
        for (u16 col = 0; col < width; col++)
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 1 + color), x + col, y + row);
}

static void card(u16 y, u16 height)
{
    solid(2, 2, y, 36, height);
    art(HL_READY_CORNER_OFFSET, 1, 1, 2, y);
    art(HL_READY_CORNER_OFFSET + 1, 1, 1, 37, y);
    art(HL_READY_CORNER_OFFSET + 2, 1, 1, 2, y + height - 1);
    art(HL_READY_CORNER_OFFSET + 3, 1, 1, 37, y + height - 1);
}

void hl_ready_open(bool hard_reset, u16 entry_data, u16 entry_bss, bool pal, const HlEdLink *link)
{
    const bool data_ok = entry_data == 0x1234;
    const bool bss_ok = entry_bss == 0;
    const bool fresh = hard_reset && data_ok && bss_ok;
    u16 font_palette[16];
    PAL_setPalette(PAL0, ready_colors, CPU);
    for (u16 i = 0; i < 16; i++) font_palette[i] = ready_colors[i];
    font_palette[1] = ready_colors[2];
    PAL_setPalette(PAL1, font_palette, CPU);
    font_palette[15] = ready_colors[6];
    PAL_setPalette(PAL2, font_palette, CPU);
    font_palette[15] = ready_colors[4];
    PAL_setPalette(PAL3, font_palette, CPU);
    VDP_loadTileData(hl_ready_tiles, READY_TILE_START, HL_READY_TILE_COUNT, CPU);

    ready_text("SGDK", 2, 1, PAL0);
    ready_text("BUILD " HL_BUILD_ID, 38 - (sizeof("BUILD " HL_BUILD_ID) - 1), 1, PAL0);
    art(0, 4, 4, 3, 3);
    art(HL_READY_TITLE_OFFSET, 18, 2, 9, 3);
    ready_text("ROM RUNNING", 9, 6, PAL0);
    solid(3, 2, 8, 36, 1);
    solid(4, 2, 8, 7, 1);

    card(10, 9);
    ready_text("BOOT / RAM CHECKS", 4, 11, PAL2);
    ready_text(fresh ? "PASS" : "REVIEW", 29, 11, fresh ? PAL3 : PAL1);
    ready_text("BOOT ENTRY", 4, 12, PAL2);
    ready_text(hard_reset ? "HARD" : "SOFT", 29, 12, hard_reset ? PAL3 : PAL1);
    ready_text("INIT DATA", 4, 13, PAL2);
    ready_hex(entry_data, 4, 23, 13, PAL1);
    ready_text(data_ok ? "PASS" : "CHECK", 29, 13, data_ok ? PAL3 : PAL1);
    ready_text("ZERO / BSS", 4, 14, PAL2);
    ready_hex(entry_bss, 4, 23, 14, PAL1);
    ready_text(bss_ok ? "PASS" : "CHECK", 29, 14, bss_ok ? PAL3 : PAL1);
    ready_text("EVERDRIVE EDIO", 4, 16, PAL2);
    ready_text(link->present ? "DETECTED" : "UNKNOWN", 25, 16, link->present ? PAL3 : PAL1);
    ready_text("USB LINK", 4, 17, PAL2);

    card(20, 4);
    ready_text("RUNNING", 4, 21, PAL1);
    ready_text("V", 24, 21, PAL2);
    ready_text("LOGIC", 4, 22, PAL2);
    ready_text("MISS", 20, 22, PAL2);
    ready_text(pal ? "VIDEO PAL  / NOMINAL 50" : "VIDEO NTSC / NOMINAL 60", 2, 25, PAL0);
    ready_text("L/R PAGES  A RESET  B INIT  C ZERO", 2, 26, PAL0);
}

void hl_ready_update(u32 vints, u32 logic, u32 missed, const HlEdLink *link)
{
    const u16 head = (vints / 10) % 8;
    for (u16 i = 0; i < 8; i++) {
        u16 state = i == head ? 2 : i == ((head + 7) % 8) ? 1 : 0;
        art(HL_READY_METER_OFFSET + state, 1, 1, 15 + i, 21);
    }
    ready_hex(vints, 8, 26, 21, PAL1);
    ready_hex(logic, 8, 10, 22, PAL1);
    ready_hex(missed, 8, 25, 22, PAL1);
    ready_text(!link->present ? "UNAVAILABLE" : link->verified ? "VERIFIED   " :
               link->ever_verified ? "EXPIRED    " : "WAITING    ",
               25, 17, link->verified ? PAL3 : PAL1);
}
