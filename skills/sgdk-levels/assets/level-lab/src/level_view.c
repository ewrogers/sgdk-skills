#include <genesis.h>
#include "resources.h"
#include "level_view.h"

static Map *map;
static Sprite *robot;
static bool sprites_started, drawn, old_pause;
static s16 old_x, old_y;
static s16 old_animation = -1, old_frame = -1;

static void position_text(s16 x, s16 y)
{
    char text[5];
    /* Camera coordinates are clamped to 0..1024, four digits plus NUL. */
    uintToStr((u16)x, text, 4);
    VDP_drawText(text, 8, 25);
    uintToStr((u16)y, text, 4);
    VDP_drawText(text, 15, 25);
}

void level_view_stop(void)
{
    /* Drain MAP's DMA/scroll work while its source storage is still alive. */
    SYS_doVBlankProcess();
    if (map) { MAP_release(map); map = NULL; }
    if (sprites_started) { SPR_end(); sprites_started = FALSE; }
    robot = NULL;
    drawn = FALSE;
    old_animation = old_frame = -1;
}

bool level_view_start(void)
{
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(TRUE, 24);
    VDP_setTextPlane(WINDOW);
    VDP_setTextPalette(PAL3);
    VDP_setTextPriority(TRUE);
    SPR_initEx(scout_image.maxNumTile);
    sprites_started = TRUE;
    /* Exclusive end: this index is the first reserved sprite pattern. */
    if ((u32)TILE_USER_INDEX + outpost_map_tileset0.numTile > TILE_USER_MAX_INDEX)
        goto failed;
    if (!VDP_loadTileSet(&outpost_map_tileset0, TILE_USER_INDEX, DMA)) goto failed;
    PAL_setPalette(PAL0, terrain_palette.data, CPU);
    PAL_setPalette(PAL1, scout_image.palette->data, CPU);
    PAL_setPalette(PAL3, palette_grey, CPU);
    map = MAP_create(&outpost_map, BG_A,
                     TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
    if (!map) goto failed;
    robot = SPR_addSprite(&scout_image, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    if (!robot) goto failed;
    SPR_setAutoAnimation(robot, FALSE);
    VDP_drawText("LEVEL LAB / OUTPOST", 2, 24);
    VDP_drawText("CAM X:0000 Y:0000", 2, 25);
    VDP_drawText("A:HOME B:WARP START:PAUSE", 2, 26);
    return TRUE;

failed:
    level_view_stop();
    VDP_drawText("LEVEL LAB SETUP FAILED", 2, 25);
    VDP_setEnable(TRUE);
    return FALSE;
}

void level_view_draw(const World *world, bool paused, bool teleport)
{
    bool redraw = !drawn || teleport;
    s16 animation = world->facing == WORLD_FACE_UP ? 2 :
                    world->facing == WORLD_FACE_DOWN ? 0 : 1;
    s16 frame = world->walking ? world->animation_ticks / 8 : 4 + world->animation_ticks / 24;
    if (redraw) VDP_setEnable(FALSE);
    /* One stream operation per rendered frame, after all simulation steps. */
    MAP_scrollToEx(map, (u16)world->camera_x, (u16)world->camera_y, redraw);
    SPR_setPosition(robot, (s16)(world->x / WORLD_UNIT - world->camera_x - 2),
                           (s16)(world->y / WORLD_UNIT - world->camera_y - 4));
    SPR_setHFlip(robot, world->facing == WORLD_FACE_LEFT);
    if (animation != old_animation || frame != old_frame) {
        SPR_setAnimAndFrame(robot, animation, frame);
        old_animation = animation;
        old_frame = frame;
    }
    SPR_update();
    if (!drawn || old_x != world->camera_x || old_y != world->camera_y)
        position_text(world->camera_x, world->camera_y);
    if (!drawn || old_pause != paused)
        VDP_drawText(paused ? "PAUSED " : "EXPLORE", 30, 25);
    old_x = world->camera_x;
    old_y = world->camera_y;
    old_pause = paused;
    drawn = TRUE;
    if (redraw) {
        /* Full map fills and distant warps are flushed while blanked. */
        SYS_doVBlankProcess();
        VDP_setEnable(TRUE);
    }
}
