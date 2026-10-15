#include <genesis.h>
#include "resources.h"
#include "split_model.h"
#include "split_video.h"

static SplitModel model;
static s16 scroll_lines[SPLIT_HEIGHT];
static Sprite *actors[4], *divider[10];

static void begin_frame(void)
{
    split_video_prepare(split_top_scroll(&model), split_bottom_scroll(&model));
}

static u16 movement(u16 held)
{
    u16 result = 0;
    if (held & BUTTON_LEFT) result |= SPLIT_LEFT;
    if (held & BUTTON_RIGHT) result |= SPLIT_RIGHT;
    if (held & BUTTON_UP) result |= SPLIT_UP;
    if (held & BUTTON_DOWN) result |= SPLIT_DOWN;
    return result;
}

static bool load_world(void)
{
    /* One immutable 512x512 region fits one 64x64 name table. Both cameras
       read it; do not create two MAP streamers fighting over the same cache. */
    Map *source = MAP_create(&outpost_map, BG_A,
                             TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
    if (!source) return FALSE;
    u16 row[128]; /* One 32-metatile row expands to two 64-cell hardware rows. */
    for (u16 y = 0; y < 32; ++y) {
        MAP_getTilemapRect(source, 0, y, 32, 1, FALSE, row);
        VDP_setTileMapData(VDP_getBGAAddress(), row, y * 128, 128, 2, CPU);
    }
    MAP_release(source); /* No queued transfers retain the local row buffer. */
    return TRUE;
}

static void number(u16 value, u16 x, u16 y)
{
    char text[6]; uintToStr(value, text, 5); VDP_drawText(text, x, y);
}

static void hud(void)
{
    VDP_drawText(model.paused ? "PAUSED " : "RUNNING", 31, 0);
    VDP_drawText(model.cpu ? "P2 CPU" : "P2 PAD", 31, 26);
    number(split_video_late(), 8, 27); number(split_video_timeouts(), 27, 27);
}

static bool setup(void)
{
    u16 blue[16], red[16];
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 64, TRUE);
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    /* The larger name tables relocate other VRAM regions. Clear reused space
       after SGDK reloads its font, before drawing either viewport's HUD. */
    VDP_clearPlane(BG_B, TRUE); VDP_clearPlane(WINDOW, TRUE);
    VDP_setTextPlane(WINDOW); VDP_setTextPalette(PAL3);
    VDP_setWindowHPos(FALSE, 0); VDP_setWindowVPos(FALSE, 2);
    PAL_setPalette(PAL3, palette_grey, CPU);
    PAL_setColor(49, 0); /* Opaque divider uses PAL3 index 1; font uses 15. */
    VDP_drawText(SYS_isPAL() ? "SPLIT LAB PAL 50 / P1 BLUE" : "SPLIT LAB NTSC 60 / P1 BLUE", 1, 0);
    VDP_drawText("DPAD MOVE A CPU B RESET START PAUSE", 1, 1);
    VDP_drawText("P2 RED / JOY 2 WHEN CPU OFF", 1, 26);
    VDP_drawText("LATE:", 2, 27); VDP_drawText("TIMEOUT:", 18, 27);
    SPR_initEx(scout_image.maxNumTile * 4 + divider_image.maxNumTile * 10);
    if ((u32)TILE_USER_INDEX + outpost_map_tileset0.numTile > TILE_USER_MAX_INDEX) return FALSE;
    if (!VDP_loadTileSet(&outpost_map_tileset0, TILE_USER_INDEX, DMA)) return FALSE;
    if (!load_world()) return FALSE;
    PAL_setPalette(PAL0, terrain_palette.data, CPU);
    for (u16 i = 0; i < 16; ++i) {
        u16 c = i < scout_image.palette->length ? scout_image.palette->data[i] : 0;
        u16 r = (c >> 1) & 7, g = (c >> 5) & 7, b = (c >> 9) & 7;
        blue[i] = r > g && r > b ? 0x0E84 : c;
        red[i] = r > g && r > b ? 0x022E : c;
    }
    PAL_setPalette(PAL1, blue, CPU); PAL_setPalette(PAL2, red, CPU);
    for (u16 view = 0; view < 2; ++view)
        for (u16 actor = 0; actor < 2; ++actor) {
            Sprite *sprite = SPR_addSprite(&scout_image, 0, 0, TILE_ATTR(PAL1 + actor, TRUE, FALSE, FALSE));
            if (!sprite) return FALSE;
            SPR_setAutoAnimation(sprite, FALSE);
            SPR_setAnimAndFrame(sprite, 0, 4);
            SPR_setVisibility(sprite, HIDDEN);
            actors[view * 2 + actor] = sprite;
        }
    /* Ten 32-pixel sprites exactly fill H40's 320-pixel scanline budget in
       this divider. Actor copies are culled away from its scanlines. */
    for (u16 i = 0; i < 10; ++i) {
        divider[i] = SPR_addSprite(&divider_image, i * 32, 108, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
        if (!divider[i]) return FALSE;
        SPR_setAutoAnimation(divider[i], FALSE);
    }
    SPR_update(); SYS_doVBlankProcess();
    VDP_setEnable(TRUE);
    split_video_init(); SYS_setVBlankCallback(begin_frame);
    return TRUE;
}

int main(bool hard_reset)
{
    u16 hz = SYS_isPAL() ? 50 : 60, refresh = 0;
    (void)hard_reset;
    JOY_init(); split_model_init(&model);
    if (!setup()) {
        SYS_setVBlankCallback(NULL); VDP_setHInterrupt(FALSE);
        VDP_setWindowVPos(FALSE, 28);
        VDP_setEnable(TRUE); VDP_drawText("SPLIT LAB SETUP FAILED", 2, 8);
        while (TRUE) SYS_doVBlankProcess();
    }
    while (TRUE) {
        u16 held = JOY_readJoypad(JOY_1), p1 = movement(held);
        if (held & BUTTON_A) p1 |= SPLIT_CPU;
        if (held & BUTTON_B) p1 |= SPLIT_RESET;
        if (held & BUTTON_START) p1 |= SPLIT_PAUSE;
        split_model_step(&model, p1, movement(JOY_readJoypad(JOY_2)), hz);
        split_scroll(&model, scroll_lines, SPLIT_HEIGHT);
        SYS_disableInts();
        for (u16 view = 0; view < 2; ++view)
            for (u16 actor = 0; actor < 2; ++actor) {
                s16 x, y;
                Sprite *sprite = actors[view * 2 + actor];
                bool visible = split_actor_position(&model, view, actor, &x, &y);
                SPR_setVisibility(sprite, visible ? VISIBLE : HIDDEN);
                if (visible) SPR_setPosition(sprite, x, y);
            }
        SPR_update();
        VDP_setHorizontalScrollLine(BG_A, 0, scroll_lines, SPLIT_HEIGHT, DMA_QUEUE);
        if (++refresh >= hz / 4) { refresh = 0; hud(); }
        SYS_enableInts();
        SYS_doVBlankProcess();
    }
}
