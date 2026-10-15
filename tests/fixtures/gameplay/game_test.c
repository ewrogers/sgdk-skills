#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Game game;

static void run(const char *name)
{
    game_reset(&game);
    if (!strcmp(name, "reset"))
    {
        game_tick(&game, MOVE_RIGHT | FIRE);
        game_reset(&game);
        assert(game.x == 48 * UNIT && game.y == 96 * UNIT);
        assert(game.facing_x == 1 && game.facing_y == 0);
        assert(game_live_shots(&game) == 0 && game.hits == 0 && game.refused == 0);
        for (u16 i = 0; i < TARGET_COUNT; i++) assert(game.targets[i].alive);
    }
    else if (!strcmp(name, "movement"))
    {
        game_tick(&game, MOVE_RIGHT);
        assert(game.x == 48 * UNIT + 1536 && game.y == 96 * UNIT);
        game_tick(&game, MOVE_LEFT | MOVE_RIGHT | MOVE_UP | MOVE_DOWN);
        assert(game.x == 48 * UNIT + 1536 && game.y == 96 * UNIT);
        game_tick(&game, MOVE_LEFT | MOVE_UP);
        assert(game.x == 48 * UNIT + 1536 - 1086 && game.y == 96 * UNIT - 1086);
        /* Squared diagonal speed is within 0.1% of straight speed. */
        assert(2L * 1086 * 1086 > 1536LL * 1536 * 999 / 1000);
        assert(2L * 1086 * 1086 < 1536LL * 1536 * 1001 / 1000);
    }
    else if (!strcmp(name, "bounds"))
    {
        assert(game_blocked(-1, 64, 12));
        assert(game_blocked(0, -1, 12));
        assert(game_blocked(1024, 64, 12));
        assert(game_blocked(64, 224, 12));
        assert(game_blocked(64, 64, 0));
        assert(!game_blocked(308, 64, 12));
        assert(game_blocked(309, 64, 12));
        assert(!game_blocked(340, 128, 12));
        assert(game_blocked(340, 127, 12));
    }
    else if (!strcmp(name, "wall_slide"))
    {
        game.x = 308 * UNIT;
        game.y = 64 * UNIT;
        game_tick(&game, MOVE_RIGHT | MOVE_DOWN);
        assert(game.x == 308 * UNIT && game.y == 64 * UNIT + 1086);
        game.x = 308 * UNIT - UNIT / 2;
        game_tick(&game, MOVE_RIGHT);
        assert(game.x <= 308 * UNIT && game.x > 307 * UNIT);
        for (u16 i = 0; i < 1000; i++) game_tick(&game, MOVE_UP);
        assert(game.y >= 32 * UNIT && game.y < 33 * UNIT);
    }
    else if (!strcmp(name, "overlap"))
    {
        assert(!game_overlap(0, 0, 12, 12, 12, 0, 12, 12));
        assert(game_overlap(0, 0, 12, 12, 11, 0, 12, 12));
        assert(!game_overlap(0, 0, 0, 12, 0, 0, 12, 12));
        assert(game_overlap(32760, 0, 12, 12, 32765, 0, 12, 12));
    }
    else if (!strcmp(name, "pool"))
    {
        for (u16 i = 0; i < SHOT_CAPACITY; i++) assert(game_try_fire(&game));
        u8 before[sizeof(game)];
        memcpy(before, &game, sizeof(game));
        assert(!game_try_fire(&game));
        assert(!memcmp(&game, before, sizeof(game)));
        game.shots[5].active = FALSE;
        assert(game_try_fire(&game));
        assert(game.shots[5].active && game_live_shots(&game) == SHOT_CAPACITY);
    }
    else if (!strcmp(name, "admission"))
    {
        for (u16 i = 0; i < SHOT_CAPACITY; i++) assert(game_try_fire(&game));
        game.shots[0].life = 1;
        game_tick(&game, FIRE);
        assert(game_live_shots(&game) == SHOT_CAPACITY && game.refused == 0);
        assert(game.shots[0].x == 52 && game.shots[0].life == 180);
        game.cooldown = 0;
        game_tick(&game, FIRE);
        assert(game.refused == 1 && game_live_shots(&game) == SHOT_CAPACITY);
    }
    else if (!strcmp(name, "cooldown"))
    {
        for (u16 tick = 0; tick < 8; tick++) game_tick(&game, FIRE);
        assert(game_live_shots(&game) == 1);
        game_tick(&game, FIRE);
        assert(game_live_shots(&game) == 2);
    }
    else if (!strcmp(name, "sweep"))
    {
        game.targets[0].x = 100;
        game.targets[0].y = 96;
        game.shots[0] = (Shot){93, 96, 1, 0, 180, TRUE};
        game_tick(&game, 0);
        assert(!game.targets[0].alive && !game.shots[0].active && game.hits == 1);
        game_tick(&game, 0);
        assert(game.hits == 1);
        game.shots[1] = (Shot){316, 64, 1, 0, 180, TRUE};
        game_tick(&game, 0);
        assert(!game.shots[1].active);
    }
    else if (!strcmp(name, "offscreen"))
    {
        game.shots[0] = (Shot){900, 140, 1, 0, 180, TRUE};
        game_tick(&game, 0);
        assert(game.shots[0].x == 904 && game.camera_x == 0);
    }
    else if (!strcmp(name, "camera"))
    {
        game_tick(&game, 0);
        assert(game.camera_x == 0);
        game.x = 500 * UNIT; game.y = 64 * UNIT;
        game_tick(&game, 0);
        assert(game.camera_x == 346 && game.x == 500 * UNIT);
        game.x = 1012 * UNIT;
        game_tick(&game, 0);
        assert(game.camera_x == WORLD_W - VIEW_W);
    }
    else if (!strcmp(name, "regions"))
    {
        for (u16 hz = 50; hz <= 60; hz += 10)
        {
            Clock clock = {0};
            u16 total = 0;
            for (u16 frame = 0; frame < hz * 3; frame++)
            {
                u16 steps = clock_steps(&clock, hz, FALSE);
                assert(steps >= 1 && steps <= 2);
                total += steps;
            }
            assert(total == 180 && clock.phase == 0);
        }
    }
    else if (!strcmp(name, "pause"))
    {
        Clock clock = {49};
        assert(clock_steps(&clock, 50, TRUE) == 0 && clock.phase == 0);
        assert(clock_steps(&clock, 50, FALSE) == 1);
        assert(clock_steps(&clock, 0, FALSE) == 0 && clock.phase == 0);
        assert(clock_steps(&clock, 59, FALSE) == 0);
    }
    else assert(!"unknown case");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    run(argv[1]);
    puts(argv[1]);
    return 0;
}
