#include "view.h"
#include "resources.h"
#include <string.h>
#define WIDTH 40
#define HEIGHT 28
static u16 cells[WIDTH * HEIGHT], sent[WIDTH * HEIGHT];
static bool rows_dirty[HEIGHT], fits;
static Screen previous = 255;
static u16 cursor, gold_base, solid_base, sprite_base[6], effect_base, target_base, field_base,
    panel_base, cached_field = 65535, cached_effect = 65535;
static const SpriteDefinition *const sprites[] = {&art_mara, &art_ivo,  &art_neri,
                                                  &art_crab, &art_moth, &art_warden};
static const Image *const backgrounds[] = {&art_title, &art_harbor, &art_battle};
static u16 reserve(u16 count)
{
    u16 start = cursor;
    if ((u32)cursor + count > (u32)TILE_FONT_INDEX)
    {
        fits = false;
        return 0;
    }
    cursor += count;
    return start;
}
static void put(u16 tile, u16 x, u16 y)
{
    if (x >= WIDTH || y >= HEIGHT)
        return;
    u16 *cell = &cells[y * WIDTH + x];
    if (*cell != tile)
    {
        *cell = tile;
        rows_dirty[y] = true;
    }
}
static void flush(TransferMethod method)
{
    for (u16 y = 0; y < HEIGHT; y++)
        if (rows_dirty[y])
        {
            bool changed = method == DMA;
            for (u16 x = 0; x < WIDTH && !changed; x++)
                if (cells[y * WIDTH + x] != sent[y * WIDTH + x])
                    changed = true;
            if (changed)
            {
                VDP_setTileMapDataRow(BG_A, cells + y * WIDTH, y, 0, WIDTH, method);
                memcpy(sent + y * WIDTH, cells + y * WIDTH, WIDTH * 2);
            }
            rows_dirty[y] = false;
        }
}
void ui_text(const char *text, u16 x, u16 y, bool gold)
{
    u16 base = gold ? gold_base : TILE_FONT_INDEX;
    for (u16 i = 0; text[i] && x + i < WIDTH; i++)
    {
        unsigned char ch = (unsigned char)text[i];
        put(TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, base + (ch >= 32 && ch < 128 ? ch - 32 : 0)),
            x + i, y);
    }
}
void ui_number(u16 number, u16 x, u16 y, bool gold)
{
    char out[6];
    u16 n = 5;
    out[n] = 0;
    do
    {
        out[--n] = '0' + number % 10;
        number /= 10;
    } while (number);
    ui_text(out + n, x, y, gold);
}
void ui_fill(u16 x, u16 y, u16 w, u16 h, u16 color)
{
    for (u16 dy = 0; dy < h; dy++)
        for (u16 dx = 0; dx < w; dx++)
            put(TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, solid_base + color), x + dx, y + dy);
}
void ui_panel(u16 x, u16 y, u16 w, u16 h)
{
    for (u16 dy = 0; dy < h; dy++)
        for (u16 dx = 0; dx < w; dx++)
        {
            u16 sx = dx == 0 ? 0 : dx == w - 1 ? 2 : 1, sy = dy == 0 ? 0 : dy == h - 1 ? 2 : 1;
            u16 source = art_panel.tilemap->tilemap[sy * 3 + sx];
            put((source & 0xF800) | ((source & 0x07FF) + panel_base) |
                    TILE_ATTR(PAL3, TRUE, FALSE, FALSE),
                x + dx, y + dy);
        }
}
void ui_bar(u16 x, u16 y, u16 w, u16 value, u16 maximum, u16 color)
{
    ui_fill(x, y, w, 1, 2);
    u16 n = maximum ? (u32)value * w / maximum : 0;
    if (n > w)
        n = w;
    if (n)
        ui_fill(x, y, n, 1, color);
}
static void image(const Image *image, u16 x, u16 y)
{
    u16 base = reserve(image->tileset->numTile);
    if (!fits)
        return;
    VDP_loadTileSet(image->tileset, base, DMA);
    VDP_setTileMapEx(BG_B, image->tilemap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, base), x, y, 0,
                     0, image->tilemap->w, image->tilemap->h, DMA);
    PAL_setPalette(PAL0, image->palette->data, DMA);
    PAL_setPalette(PAL1, image->palette->data + 16, DMA);
}
static void begin(const Game *g)
{
    SYS_disableInts();
    VDP_setEnable(FALSE);
    fits = true;
    cursor = TILE_USER_INDEX;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_clearSprites();
    VDP_updateSprites(1, DMA);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setHilightShadow(FALSE);
    memset(cells, 0, sizeof cells);
    memset(sent, 0, sizeof sent);
    memset(rows_dirty, 0, sizeof rows_dirty);
    VDP_loadTileSet(&art_font_ivory, TILE_FONT_INDEX, DMA);
    gold_base = reserve(art_font_gold.numTile);
    VDP_loadTileSet(&art_font_gold, gold_base, DMA);
    solid_base = reserve(16);
    u32 solid[128];
    for (u16 c = 0; c < 16; c++)
        for (u16 y = 0; y < 8; y++)
            solid[c * 8 + y] = 0x11111111UL * c;
    VDP_loadTileData(solid, solid_base, 16, DMA);
    panel_base = reserve(art_panel.tileset->numTile);
    VDP_loadTileSet(art_panel.tileset, panel_base, DMA);
    PAL_setPalette(PAL3, ui_palette.data, DMA);
    VDP_setBackgroundColor(0);
    u16 background = g->screen == CAMP ? 1 : g->screen == BATTLE || g->screen == RESULT ? 2 : 0;
    if (g->screen == TITLE || g->screen == CREDITS)
        image(backgrounds[0], 4, 1);
    else if (g->screen == CAMP)
        image(backgrounds[1], 0, 2);
    else if (g->screen == BATTLE || g->screen == RESULT)
        image(backgrounds[background], 0, 1);
    PAL_setPalette(PAL2, art_mara.palette->data, DMA);
    for (u16 i = 0; i < (g->screen == PARTY                           ? 3
                         : g->screen == BATTLE || g->screen == RESULT ? 6
                                                                      : 0);
         i++)
    {
        const TileSet *tiles = sprites[i]->animations[0]->frames[0]->tileset;
        sprite_base[i] = reserve(tiles->numTile);
        if (fits)
            VDP_loadTileSet(tiles, sprite_base[i], DMA);
    }
    effect_base = reserve(art_effect.animations[0]->frames[0]->tileset->numTile);
    target_base = reserve(art_target.animations[0]->frames[0]->tileset->numTile);
    VDP_loadTileSet(art_target.animations[0]->frames[0]->tileset, target_base, DMA);
    field_base = reserve(25);
    cached_field = 65535;
    cached_effect = 65535;
    ui_draw(g);
    if (!fits)
    {
        ui_fill(0, 0, 40, 28, 1);
        ui_text("Art exceeds VRAM reservation", 3, 12, false);
    }
    flush(DMA);
    VDP_setEnable(TRUE);
    SYS_enableInts();
}
static u16 sprite(u16 count, const AnimationFrame *frame, u16 base, s16 x, s16 y, u16 palette,
                  bool foreground)
{
    for (u16 p = 0; p < (frame->numSprite & 0x7F); p++)
    {
        const FrameVDPSprite *piece = &frame->frameVDPSprites[p];
        VDP_setSpriteFull(count, x + piece->offsetX, y + piece->offsetY, piece->size,
                          TILE_ATTR_FULL(palette, foreground, FALSE, FALSE, base), count + 1);
        base += piece->numTile;
        count++;
    }
    return count;
}
static void actors(const Game *g)
{
    u16 count = 0;
    if (g->screen == CAMP)
    {
        const AnimationFrame *f =
            g->moving ? art_field_walk.animations[g->direction]->frames[g->walk_frame]
                      : art_field_idle.animations[0]->frames[g->direction];
        u16 key = g->direction * 8 + (g->moving ? g->walk_frame + 4 : 0);
        if (cached_field != key)
        {
            VDP_loadTileSet(f->tileset, field_base, DMA_QUEUE);
            cached_field = key;
        }
        count = sprite(count, f, field_base, g->x - 20, g->y - 20, PAL2, true);
    }
    else if (g->screen == PARTY)
    {
        for (u16 i = 0; i < 3; i++)
            count = sprite(count, sprites[i]->animations[0]->frames[0], sprite_base[i], 270,
                           38 + i * 48, PAL2, true);
    }
    else if (g->screen == BATTLE || g->screen == RESULT)
    {
        for (u16 i = 0; i < 6; i++)
        {
            const Fighter *actor = &g->battle.actors[i];
            if (!actor->hp)
                continue;
            s16 x, y;
            battle_position(&g->battle, i, &x, &y);
            bool resolving = g->screen == BATTLE && g->battle_menu == RESOLVE;
            const Event *e = &g->battle.event;
            if (resolving && e->target == i && e->kind == HIT && g->elapsed < 18 &&
                (g->elapsed / 3) % 2)
                continue;
            if (resolving && e->source == i && g->elapsed < 16)
                x += (i < 3 ? -1 : 1) * (g->elapsed < 8 ? g->elapsed : 16 - g->elapsed);
            y += (g->ticks / 20 + i) % 2;
            const AnimationFrame *frame = sprites[actor->species]->animations[0]->frames[0];
            count = sprite(count, frame, sprite_base[actor->species], x, y, PAL2,
                           g->screen == BATTLE && !g->paused);
        }
        if (g->screen == BATTLE && g->battle_menu == RESOLVE && g->elapsed < 24)
        {
            u16 target = g->battle.event.target;
            const Fighter *f = &g->battle.actors[target];
            const AnimationFrame *frame = art_effect.animations[0]->frames[(g->elapsed / 6) % 4];
            u16 key = (g->elapsed / 6) % 4;
            if (cached_effect != key)
            {
                VDP_loadTileSet(frame->tileset, effect_base, DMA_QUEUE);
                cached_effect = key;
            }
            s16 x, y;
            battle_position(&g->battle, target, &x, &y);
            x += f->species == ORRERY ? 16 : 0;
            y += 8;
            count = sprite(count, frame, effect_base, x, y, PAL3, !g->paused);
        }
    }
    if (g->screen == BATTLE && g->battle_menu != RESOLVE && !g->paused)
    {
        u16 target = g->battle_menu == TARGET ? g->pending.target : battle_actor(&g->battle);
        s16 x, y;
        battle_position(&g->battle, target, &x, &y);
        u16 w = g->battle.actors[target].species == ORRERY ? 64 : 32;
        u16 phase = (g->ticks / 7) % 8, bob = phase < 4 ? phase : 7 - phase;
        x += (w - 16) / 2;
        y -= 16 + bob;
        if (y < 8)
            y = 8;
        count = sprite(count, art_target.animations[0]->frames[0], target_base, x, y, PAL3, true);
    }
    if (count)
        VDP_setSpriteLink(count - 1, 0);
    else
        VDP_clearSprites();
    VDP_updateSprites(count ? count : 1, DMA_QUEUE);
}
void view_init(void)
{
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 0);
}
void view_frame(Game *g)
{
    if (previous != g->screen)
    {
        begin(g);
        previous = g->screen;
        g->dirty = false;
    }
    else if (g->dirty)
    {
        memset(cells, 0, sizeof cells);
        for (u16 y = 0; y < 28; y++)
            rows_dirty[y] = true;
        ui_draw(g);
        g->dirty = false;
    }
    actors(g);
    flush(DMA_QUEUE_COPY);
}
