#include <genesis.h>
#include "game.h"
#include "resources.h"

_Static_assert(UNIT == (1 << FIX32_FRAC_BITS), "Match SGDK fix32 scaling");

static Game game;
static Clock clock;
static Sprite *player_sprite, *targets[TARGET_COUNT], *shots[SHOT_CAPACITY];

static bool setup(void)
{
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setPlaneSize(128, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setTextPlane(WINDOW);
    VDP_setTextPalette(PAL3);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 4);
    PAL_setPalette(PAL0, player_image.palette->data, CPU);
    PAL_setPalette(PAL1, player_image.palette->data, CPU);
    PAL_setPalette(PAL3, palette_grey, CPU);
    /* SGDK's sixteen system tiles are solid color-index tiles. */
    for (u16 y = 0; y < 7; ++y)
        for (u16 x = 0; x < 32; ++x)
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                TILE_SYSTEM_INDEX + (game_solid(x, y) ? 1 : 2)), x * 4, y * 4, 4, 4);

    u16 tiles = player_image.maxNumTile + TARGET_COUNT * target_image.maxNumTile
        + SHOT_CAPACITY * shot_image.maxNumTile;
    SPR_initEx(tiles);
    player_sprite = SPR_addSprite(&player_image, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
    if (!player_sprite) return FALSE;
    for (u16 i = 0; i < TARGET_COUNT; ++i)
    {
        targets[i] = SPR_addSprite(&target_image, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        if (!targets[i]) return FALSE;
    }
    for (u16 i = 0; i < SHOT_CAPACITY; ++i)
    {
        shots[i] = SPR_addSprite(&shot_image, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        if (!shots[i]) return FALSE;
    }
    return TRUE;
}

static void place(Sprite *sprite, s16 x, s16 y, bool active)
{
    s16 screen_x = x - game.camera_x;
    SPR_setVisibility(sprite, active && screen_x > -16 && screen_x < VIEW_W ? VISIBLE : HIDDEN);
    SPR_setPosition(sprite, screen_x, y);
}

static void render(void)
{
    place(player_sprite, game.x / UNIT, game.y / UNIT, TRUE);
    for (u16 i = 0; i < TARGET_COUNT; ++i)
        place(targets[i], game.targets[i].x, game.targets[i].y, game.targets[i].alive);
    for (u16 i = 0; i < SHOT_CAPACITY; ++i)
        place(shots[i], game.shots[i].x, game.shots[i].y, game.shots[i].active);
    VDP_setHorizontalScrollVSync(BG_A, -game.camera_x);
    SPR_update();
}

int main(bool hard_reset)
{
    u16 previous = 0, shown_hits = 65535, shown_live = 65535, shown_refused = 65535;
    bool paused = FALSE;
    (void)hard_reset;
    game_reset(&game);
    if (!setup())
    {
        SYS_doVBlankProcess();
        SPR_end();
        VDP_drawText("Sprite setup failed", 1, 1);
        VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    VDP_drawText("DPAD Move  A Fire  B Reset  Start Pause", 1, 1);
    VDP_drawText("Hits:    Shots:    Full:", 1, 2);
    render();
    SYS_doVBlankProcess();
    VDP_setEnable(TRUE);
    while (TRUE)
    {
        u16 buttons = JOY_readJoypad(JOY_1);
        u16 pressed = buttons & (u16)~previous;
        previous = buttons;
        if (pressed & BUTTON_START)
        {
            paused = !paused;
            VDP_drawText(paused ? "PAUSED" : "      ", 31, 2);
        }
        if (pressed & BUTTON_B)
        {
            game_reset(&game);
            clock.phase = 0;
        }
        u16 held = ((buttons & BUTTON_LEFT) ? MOVE_LEFT : 0)
            | ((buttons & BUTTON_RIGHT) ? MOVE_RIGHT : 0)
            | ((buttons & BUTTON_UP) ? MOVE_UP : 0)
            | ((buttons & BUTTON_DOWN) ? MOVE_DOWN : 0)
            | ((buttons & BUTTON_A) ? FIRE : 0);
        u16 steps = clock_steps(&clock, IS_PAL_SYSTEM ? 50 : 60, paused);
        while (steps--) game_tick(&game, held);
        u16 live = game_live_shots(&game);
        char number[8];
        if (shown_hits != game.hits) { uintToStr(game.hits, number, 2); VDP_drawText(number, 6, 2); shown_hits = game.hits; }
        if (shown_live != live) { uintToStr(live, number, 2); VDP_drawText(number, 16, 2); shown_live = live; }
        if (shown_refused != game.refused) { uintToStr(game.refused, number, 5); VDP_drawText(number, 25, 2); shown_refused = game.refused; }
        render();
        SYS_doVBlankProcess();
    }
}
