#include "game.h"
#ifdef __m68k__
#include <genesis.h>
#else
#include <string.h>
#endif

bool game_solid(s16 column, s16 row)
{
    return column < 0 || column >= 32 || row <= 0 || row >= 6
        || (column == 10 && row <= 3) || (column == 20 && row >= 3);
}

static bool blocked_position(fix32 x, fix32 y, s16 size)
{
    /* Check bounds before division: C truncates negative division toward zero. */
    if (size <= 0 || x < 0 || y < 0 || x > (WORLD_W - size) * UNIT || y > (WORLD_H - size) * UNIT)
        return TRUE;
    for (s16 row = y / (32 * UNIT); row <= (y + size * UNIT - 1) / (32 * UNIT); ++row)
        for (s16 col = x / (32 * UNIT); col <= (x + size * UNIT - 1) / (32 * UNIT); ++col)
            if (game_solid(col, row)) return TRUE;
    return FALSE;
}

bool game_blocked(s16 x, s16 y, s16 size)
{
    return blocked_position((s32)x * UNIT, (s32)y * UNIT, size);
}

bool game_overlap(s16 ax, s16 ay, s16 aw, s16 ah,
                  s16 bx, s16 by, s16 bw, s16 bh)
{
    return aw > 0 && ah > 0 && bw > 0 && bh > 0 &&
        ax < (s32)bx + bw && bx < (s32)ax + aw &&
        ay < (s32)by + bh && by < (s32)ay + ah;
}

void game_reset(Game *game)
{
    static const Target targets[TARGET_COUNT] = {
        {200, 96, TRUE}, {280, 160, TRUE}, {440, 64, TRUE},
        {580, 144, TRUE}, {760, 64, TRUE}, {940, 144, TRUE}
    };
    memset(game, 0, sizeof(*game));
    game->x = 48 * UNIT;
    game->y = 96 * UNIT;
    game->facing_x = 1;
    memcpy(game->targets, targets, sizeof targets);
}

bool game_try_fire(Game *game)
{
    for (u16 i = 0; i < SHOT_CAPACITY; ++i)
    {
        Shot *shot = &game->shots[i];
        if (shot->active) continue;
        *shot = (Shot){game->x / UNIT + (ACTOR_SIZE - SHOT_SIZE) / 2,
                       game->y / UNIT + (ACTOR_SIZE - SHOT_SIZE) / 2,
                       game->facing_x, game->facing_y, 180, TRUE};
        return TRUE;
    }
    return FALSE;
}

static void move_axis(Game *game, s16 amount, bool horizontal)
{
    /* At most two candidate positions per axis at this game's speed. */
    while (amount != 0)
    {
        s16 step = amount > UNIT ? UNIT : amount < -UNIT ? -UNIT : amount;
        s32 x = game->x + (horizontal ? step : 0);
        s32 y = game->y + (horizontal ? 0 : step);
        if (blocked_position(x, y, ACTOR_SIZE)) break;
        game->x = x;
        game->y = y;
        amount -= step;
    }
}

static void update_shots(Game *game)
{
    for (u16 i = 0; i < SHOT_CAPACITY; ++i)
    {
        Shot *shot = &game->shots[i];
        if (!shot->active) continue;
        if (--shot->life == 0) { shot->active = FALSE; continue; }
        /* Four one-pixel steps prevent skipping a thin target or wall. */
        for (u16 step = 0; step < 4 && shot->active; ++step)
        {
            shot->x += shot->dx;
            shot->y += shot->dy;
            if (game_blocked(shot->x, shot->y, SHOT_SIZE)) { shot->active = FALSE; break; }
            for (u16 target = 0; target < TARGET_COUNT; ++target)
            {
                Target *other = &game->targets[target];
                if (other->alive && game_overlap(shot->x, shot->y, SHOT_SIZE, SHOT_SIZE,
                                                other->x, other->y, ACTOR_SIZE, ACTOR_SIZE))
                {
                    other->alive = FALSE;
                    shot->active = FALSE;
                    ++game->hits;
                    break;
                }
            }
        }
    }
}

void game_tick(Game *game, u16 held)
{
    s16 dx = !!(held & MOVE_RIGHT) - !!(held & MOVE_LEFT);
    s16 dy = !!(held & MOVE_DOWN) - !!(held & MOVE_UP);
    s16 speed = dx && dy ? 1086 : 1536; /* Approximate diagonal normalization. */
    if (dx) { game->facing_x = dx; game->facing_y = 0; }
    else if (dy) { game->facing_x = 0; game->facing_y = dy; }
    move_axis(game, dx * speed, TRUE);
    move_axis(game, dy * speed, FALSE);
    update_shots(game); /* Retire old shots before admitting a new one. */
    if (game->cooldown) --game->cooldown;
    if ((held & FIRE) && game->cooldown == 0)
    {
        if (!game_try_fire(game)) ++game->refused;
        game->cooldown = 8;
    }
    s32 camera = game->x / UNIT + ACTOR_SIZE / 2 - VIEW_W / 2;
    if (camera < 0) camera = 0;
    if (camera > WORLD_W - VIEW_W) camera = WORLD_W - VIEW_W;
    game->camera_x = camera;
}

u16 game_live_shots(const Game *game)
{
    u16 count = 0;
    for (u16 i = 0; i < SHOT_CAPACITY; ++i) count += game->shots[i].active;
    return count;
}

u16 clock_steps(Clock *clock, u16 refresh_hz, bool paused)
{
    if (paused || (refresh_hz != 50 && refresh_hz != 60)) { clock->phase = 0; return 0; }
    clock->phase += 60;
    u16 steps = clock->phase / refresh_hz;
    clock->phase %= refresh_hz;
    return steps;
}
