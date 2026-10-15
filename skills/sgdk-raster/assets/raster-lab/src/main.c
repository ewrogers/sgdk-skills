#include <genesis.h>
#include "resources.h"
#include "raster_model.h"
#include "water_split.h"

static RasterModel model;
static s16 line_scroll[RASTER_LINES];
static Map *map;
static Sprite *robot;
static Sprite *surface[10];
static Sprite *bubbles[WATER_BUBBLES], *splash;
static u16 surface_tiles[4];
static u16 hud_refresh, video_hz;
static void status_text(void);

static void begin_frame(void)
{
    /* Ordinary HUD writes follow DMA, while dry colors and the H counter
       have already been restored/armed by water_split's early V interrupt. */
    if (++hud_refresh < video_hz / 4) return;
    hud_refresh = 0;
    SYS_disableInts();
    status_text();
    SYS_enableInts();
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
    SPR_initEx(scout_image.maxNumTile +
               water_bubble.maxNumTile * WATER_BUBBLES + water_splash.maxNumTile);
    if ((u32)TILE_USER_INDEX + outpost_map_tileset0.numTile > TILE_USER_MAX_INDEX) return FALSE;
    if (!VDP_loadTileSet(&outpost_map_tileset0, TILE_USER_INDEX, DMA)) return FALSE;
    /* All ten strips share four resident frames. Re-uploading identical foam
       for every strip wastes DMA and can delay the mid-frame interrupt. */
    u16 next_tile = TILE_USER_INDEX + outpost_map_tileset0.numTile;
    for (u16 i = 0; i < 4; ++i) {
        const TileSet *tiles = water_surface.animations[0]->frames[i]->tileset;
        if ((u32)next_tile + tiles->numTile > TILE_USER_MAX_INDEX) return FALSE;
        surface_tiles[i] = next_tile;
        if (!VDP_loadTileSet(tiles, next_tile, CPU)) return FALSE;
        next_tile += tiles->numTile;
    }
    map = MAP_create(&outpost_map, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
    if (!map) return FALSE;
    /* Insert the surface first so it wins sprite overlap and the scanline
       pixel budget at the seam. PAL2 never changes during the CRAM split. */
    PAL_setPalette(PAL2, water_surface.palette->data, CPU);
    for (u16 i = 0; i < 10; ++i) {
        surface[i] = SPR_addSpriteEx(&water_surface, i * 32, model.waterline - 4,
                                    TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, surface_tiles[0]), 0);
        if (!surface[i]) return FALSE;
        SPR_setAutoAnimation(surface[i], FALSE);
        SPR_setVisibility(surface[i], VISIBLE);
    }
    robot = SPR_addSprite(&scout_image, 152, 84, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    if (!robot) return FALSE;
    SPR_setAutoAnimation(robot, FALSE);
    for (u16 i = 0; i < WATER_BUBBLES; ++i) {
        bubbles[i] = SPR_addSprite(&water_bubble, 156, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
        if (!bubbles[i]) return FALSE;
        SPR_setAutoAnimation(bubbles[i], FALSE); SPR_setVisibility(bubbles[i], HIDDEN);
    }
    splash = SPR_addSprite(&water_splash, 144, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    if (!splash) return FALSE;
    SPR_setAutoAnimation(splash, FALSE); SPR_setVisibility(splash, HIDDEN);
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
    /* Driver loading can hold interrupts while copying Z80 RAM. Finish that
       startup work before installing time-sensitive palette callbacks. */
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    Z80_setForceDelayDMA(TRUE);
    XGM2_setLoopNumber(-1);
    XGM2_play(music);
    water_split_init(dry, wet); /* Copies stack data into persistent storage. */
    if (water_split_pair_count() > 8) return FALSE; /* Surface coverage for this art. */
    water_split_publish(model.waterline, model.tint);
    SYS_setVBlankCallback(begin_frame);
    VDP_setEnable(TRUE);
    return TRUE;
}

int main(bool hard_reset)
{
    u16 hz = SYS_isPAL() ? 50 : 60;
    video_hz = hz;
    (void)hard_reset;
    JOY_init();
    raster_model_init(&model);
    if (!setup()) {
        VDP_setHInterrupt(FALSE);
        SYS_setVBlankCallback(NULL);
        water_split_stop();
        VDP_setWindowVPos(FALSE, 28);
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
        /* MAP may touch VSRAM: protect its VDP command/data sequences. Sprite
           setters/update only prepare RAM/queued DMA in SGDK 2.11, so leave
           interrupts enabled during that larger CPU workload. */
        SYS_disableInts();
        MAP_scrollTo(map, model.camera_x, 96);
        SYS_enableInts();
        for (u16 i = 0; i < 10; ++i) {
            u16 frame = (model.phase / 12 + i) & 3;
            SPR_setPosition(surface[i], i * 32, model.waterline - 4);
            SPR_setFrame(surface[i], frame);
            SPR_setVRAMTileIndex(surface[i], surface_tiles[frame]);
        }
        u16 travel = model.phase < 128 ? model.phase : 255 - model.phase;
        SPR_setPosition(robot, 152, 52 + travel);
        SPR_setAnimAndFrame(robot, 0, 4 + (model.phase / 24) % 2);
        static const s16 drift[4] = {-1, 0, 1, 0};
        for (u16 i = 0; i < WATER_BUBBLES; ++i) {
            const WaterBubble *b = &model.fx.bubbles[i];
            SPR_setVisibility(bubbles[i], b->active ? VISIBLE : HIDDEN);
            if (b->active) {
                SPR_setPosition(bubbles[i], 156 + drift[(b->age / 8) & 3], b->y);
                SPR_setFrame(bubbles[i], (b->age / 6) & 3);
            }
        }
        SPR_setVisibility(splash, model.fx.splash_left ? VISIBLE : HIDDEN);
        if (model.fx.splash_left) {
            SPR_setPosition(splash, 144, model.waterline - 20);
            SPR_setFrame(splash, (WATER_SPLASH_TICKS - model.fx.splash_left) / 6);
        }
        SPR_update();
        /* MAP queues a flat line table on camera moves. This later transfer
           supplies the final waved table. Source remains alive until flush. */
        SYS_disableInts();
        VDP_setHorizontalScrollLine(BG_A, 0, line_scroll, RASTER_LINES, DMA_QUEUE);
        /* Publish in the same critical section as the completed frame queue,
           before a pending V interrupt can consume it. */
        water_split_publish(model.waterline, model.tint);
        SYS_enableInts();
        SYS_doVBlankProcess();
    }
}
