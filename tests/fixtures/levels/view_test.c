#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <genesis.h>
#include "resources.h"
#include "level_view.h"

static const u16 colors[16] = {0};
const u16 palette_grey[16] = {0};
const TileSet outpost_map_tileset0 = {64};
const Palette terrain_palette = {16, colors};
const MapDefinition outpost_map = {0};
const SpriteDefinition scout_image = {4, &terrain_palette};
u16 test_tile_end = 80;
static Map instance;
static Sprite actor;
static bool fail_map, fail_sprite, fail_tiles, enabled, pending, live;
static unsigned releases, ends, flushes, full, incremental, writes;
static unsigned animation_calls;
static s16 last_animation, last_frame;
static bool last_flip;
static char cells[28][41];

void VDP_setEnable(bool value) { enabled = value; }
void VDP_setScreenWidth320(void) {}
void VDP_setScreenHeight224(void) {}
void VDP_setPlaneSize(u16 w, u16 h, bool update) { assert(w == 64 && h == 32 && update); }
void VDP_setScrollingMode(u16 h, u16 v) { assert(h == HSCROLL_PLANE && v == VSCROLL_PLANE); }
void VDP_setWindowHPos(u16 right, u16 p) { assert(!right && p == 0); }
void VDP_setWindowVPos(u16 down, u16 p) { assert(down && p == 24); }
void VDP_setTextPlane(VDPPlane p) { assert(p == WINDOW); }
void VDP_setTextPalette(u16 p) { assert(p == PAL3); }
void VDP_setTextPriority(u16 p) { assert(p); }
void VDP_drawText(const char *s, u16 x, u16 y)
{ assert(x + strlen(s) <= 40 && y >= 24 && y < 28); memcpy(cells[y]+x, s, strlen(s)); writes++; }
u16 uintToStr(u32 n, char *out, u16 width)
{ assert(n <= 1024 && width == 4); snprintf(out, 5, "%04u", (unsigned)n); return 4; }
void PAL_setPalette(u16 p, const u16 *data, TransferMethod tm)
{ assert(!enabled && tm == CPU); assert((p == 0 || p == 1) ? data == colors : p == 3 && data == palette_grey); }
bool SYS_doVBlankProcess(void) { pending = FALSE; flushes++; return TRUE; }
void SPR_initEx(u16 n) { assert(!enabled && n == 4); }
void SPR_end(void) { assert(!pending); ends++; }
Sprite *SPR_addSprite(const SpriteDefinition *def, s16 x, s16 y, u16 attr)
{ assert(def == &scout_image && x == 0 && y == 0 && attr == TILE_ATTR(PAL1,FALSE,FALSE,FALSE)); return fail_sprite ? NULL : &actor; }
void SPR_setPosition(Sprite *s, s16 x, s16 y) { assert(s == &actor && x > -32 && x < 320 && y > -32 && y < 192); }
void SPR_update(void) { pending = TRUE; }
void SPR_setAutoAnimation(Sprite *s, bool value) { assert(s == &actor && !value); }
void SPR_setHFlip(Sprite *s, bool value) { assert(s == &actor); last_flip = value; }
void SPR_setAnimAndFrame(Sprite *s, s16 animation, s16 frame)
{ assert(s == &actor && animation >= 0 && animation < 3 && frame >= 0 && frame < 8);
  last_animation = animation; last_frame = frame; animation_calls++; }
bool VDP_loadTileSet(const TileSet *set, u16 index, TransferMethod tm)
{ assert(!enabled && set == &outpost_map_tileset0 && index == 16 && tm == DMA); return !fail_tiles; }
Map *MAP_create(const MapDefinition *def, VDPPlane p, u16 base)
{ assert(!live && def == &outpost_map && p == BG_A && base == 16); if (fail_map) return NULL; live = TRUE; return &instance; }
void MAP_scrollToEx(Map *m, u32 x, u32 y, bool force)
{ assert(live && m == &instance && x <= 704 && y <= 832); pending = TRUE; if (force) { assert(!enabled); full++; } else incremental++; }
void MAP_release(Map *m) { assert(m == &instance && live && !pending); live = FALSE; releases++; }

int main(void)
{
    World world = {0};
    unsigned old;
    world.x = 64 * WORLD_UNIT; world.y = 64 * WORLD_UNIT;
    memset(cells, ' ', sizeof cells);
    assert(level_view_start() && !enabled && live);
    level_view_draw(&world, FALSE, FALSE);
    assert(full == 1 && enabled && !pending);
    assert(last_animation == 0 && last_frame == 4 && !last_flip);
    assert(!memcmp(cells[25]+2, "CAM X:0000 Y:0000", 17));
    old = writes;
    level_view_draw(&world, FALSE, FALSE);
    assert(pending && writes == old && incremental == 1);
    assert(animation_calls == 1);
    world.animation_ticks = 48; level_view_draw(&world, FALSE, FALSE);
    assert(last_frame == 6);
    world.walking = TRUE; world.facing = WORLD_FACE_LEFT; world.animation_ticks = 8;
    level_view_draw(&world, FALSE, FALSE);
    assert(last_animation == 1 && last_frame == 1 && last_flip);
    old = animation_calls; level_view_draw(&world, TRUE, FALSE);
    assert(animation_calls == old && last_frame == 1);
    /* Stop must drain pending stream work before freeing either owner. */
    level_view_stop(); assert(releases == 1 && ends == 1 && !pending && !live);
    level_view_stop(); assert(releases == 1 && ends == 1);
    assert(level_view_start());
    world.x = 944 * WORLD_UNIT; world.y = 944 * WORLD_UNIT;
    world.camera_x = 704; world.camera_y = 832;
    level_view_draw(&world, TRUE, TRUE);
    assert(full == 2 && !pending && !memcmp(cells[25]+30, "PAUSED ", 7));
    assert(!memcmp(cells[25]+8, "0704", 4) && !memcmp(cells[25]+15, "0832", 4));
    level_view_stop();
    old = releases; fail_map = TRUE;
    assert(!level_view_start() && !live && releases == old); fail_map = FALSE;
    fail_sprite = TRUE;
    assert(!level_view_start() && !live && releases == old+1); fail_sprite = FALSE;
    fail_tiles = TRUE;
    assert(!level_view_start() && !live); fail_tiles = FALSE;
    test_tile_end = 79; /* 16+64 is one tile beyond this exclusive limit. */
    assert(!level_view_start() && !live);
    assert(flushes > 5);
    puts("view");
    return 0;
}
