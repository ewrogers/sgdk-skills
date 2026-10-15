#include <genesis.h>
#include "resources.h"
#include "raster_model.h"
#include "water_split.h"

static RasterModel model;
static s16 line_scroll[RASTER_LINES];
static Map *map;
static Sprite *robot;

static void begin_frame(void)
{
    /* SGDK invokes this AFTER its queued DMA and scroll/fade processing. */
    water_split_prepare(model.waterline, model.tint);
}

static u16 controls(u16 held)
{
    u16 result = 0;
    if (held & BUTTON_LEFT) result |= RASTER_LEFT;
    if (held & BUTTON_RIGHT) result |= RASTER_RIGHT;
    if (held & BUTTON_UP) result |= RASTER_UP;
    if (held & BUTTON_DOWN) result |= RASTER_DOWN;
    if (held & BUTTON_A) result |= RASTER_TINT;
    if (held & BUTTON_B) result |= RASTER_WAVES;
    if (held & BUTTON_C) result |= RASTER_MUSIC;
    if (held & BUTTON_START) result |= RASTER_PAUSE;
    return result;
}

static void status_text(void)
{
    char text[6];
    uintToStr(model.waterline, text, 3); VDP_drawText(text, 5, 3);
    uintToStr(water_split_last_line(), text, 3); VDP_drawText(text, 12, 3);
    uintToStr(water_split_late(), text, 5); VDP_drawText(text, 17, 3);
    uintToStr(water_split_timeouts(), text, 5); VDP_drawText(text, 24, 3);
    uintToStr(water_split_setup_late(), text, 5); VDP_drawText(text, 31, 3);
    VDP_drawText(model.tint ? "TINT " : "DRY  ", 24, 0);
    VDP_drawText(model.waves ? "WAVE " : "FLAT ", 29, 0);
    VDP_drawText(model.paused ? "STOP" : "RUN ", 34, 0);
}

static bool setup(void)
{
    u16 dry[32], wet[32];
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 4);
    VDP_setTextPlane(WINDOW);
    VDP_setTextPalette(PAL3);
    PAL_setPalette(PAL3, palette_grey, CPU);
    VDP_drawText(SYS_isPAL() ? "RASTER LAB PAL 50" : "RASTER LAB NTSC 60", 1, 0);
    VDP_drawText("UP/DOWN WATER   LEFT/RIGHT CAMERA", 1, 1);
    VDP_drawText("A TINT B WAVES C MUSIC START PAUSE", 1, 2);
    VDP_drawText("LINE000 IRQ000 L00000 T00000 S00000", 1, 3);
    SPR_initEx(scout_image.maxNumTile);
    if ((u32)TILE_USER_INDEX + outpost_map_tileset0.numTile > TILE_USER_MAX_INDEX) return FALSE;
    if (!VDP_loadTileSet(&outpost_map_tileset0, TILE_USER_INDEX, DMA)) return FALSE;
    map = MAP_create(&outpost_map, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
    if (!map) return FALSE;
    robot = SPR_addSprite(&scout_image, 152, 84, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    if (!robot) return FALSE;
    SPR_setAutoAnimation(robot, FALSE);
    for (u16 i = 0; i < 16; ++i) {
        dry[i] = i < terrain_palette.length ? terrain_palette.data[i] : 0;
        dry[16 + i] = i < scout_image.palette->length ? scout_image.palette->data[i] : 0;
    }
    dry[0] = dry[16] = 0;
    raster_tint(dry, wet, 32);
    PAL_setColors(0, dry, 32, CPU);
    MAP_scrollToEx(map, 0, 96, TRUE);
    SPR_update();
    SYS_doVBlankProcess(); /* Initial full map/sprite transfer while hidden. */
    VDP_setEnable(TRUE);
    water_split_init(dry, wet); /* Copies stack data into persistent storage. */
    SYS_setVBlankCallback(begin_frame);
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    Z80_setForceDelayDMA(TRUE);
    XGM2_setLoopNumber(-1);
    XGM2_play(music);
    return TRUE;
}

int main(bool hard_reset)
{
    u16 refresh = 0, hz = SYS_isPAL() ? 50 : 60;
    (void)hard_reset;
    JOY_init();
    raster_model_init(&model);
    if (!setup()) {
        VDP_setHInterrupt(FALSE);
        SYS_setVBlankCallback(NULL);
        VDP_setEnable(TRUE);
        VDP_drawText("RASTER LAB SETUP FAILED", 2, 12);
        while (TRUE) SYS_doVBlankProcess();
    }
    while (TRUE) {
        bool music_before = model.music;
        raster_model_step(&model, controls(JOY_readJoypad(JOY_1)), hz);
        if (music_before != model.music) {
            if (model.music) XGM2_play(music);
            else XGM2_stop();
        }
        raster_scroll(&model, line_scroll, RASTER_LINES);
        /* H interrupt owns VDP ports during its split. Protect ALL ordinary
           port work, not just the CRAM writes. Keep the protected work short. */
        SYS_disableInts();
        MAP_scrollTo(map, model.camera_x, 96);
        u16 travel = model.phase < 128 ? model.phase : 255 - model.phase;
        SPR_setPosition(robot, 152, 52 + travel);
        SPR_setAnimAndFrame(robot, 0, 4 + (model.phase / 24) % 2);
        SPR_update();
        /* MAP queues a flat line table on camera moves. This later transfer
           supplies the final waved table. Source remains alive until flush. */
        VDP_setHorizontalScrollLine(BG_A, 0, line_scroll, RASTER_LINES, DMA_QUEUE);
        if (++refresh >= hz / 4) { refresh = 0; status_text(); }
        SYS_enableInts();
        SYS_doVBlankProcess();
    }
}
