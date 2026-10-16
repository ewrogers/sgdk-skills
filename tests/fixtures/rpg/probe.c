#include "game.h"
#include <string.h>
#ifdef SGDK_GCC
#include <genesis.h>
#endif
#ifndef SGDK_GCC
#include <stdio.h>
#else
static int compare_bytes(const void *a, const void *b, uint16_t size)
{
    const uint8_t *x = a, *y = b;
    for (uint16_t i = 0; i < size; i++)
        if (x[i] != y[i])
            return 1;
    return 0;
}
#define memcmp compare_bytes
#endif
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
            return __LINE__;                                                                       \
    } while (0)
static void press(Game *g, uint16_t key)
{
    game_step(g, key, key, key & (UP | DOWN | LEFT | RIGHT), 1);
}
static void force_turn(Battle *b, uint8_t actor)
{
    b->order[0] = actor;
    b->cursor = 0;
}
static int checks(void)
{
    for (uint16_t hz = 50; hz <= 60; hz += 10)
    {
        FrameClock clock = {0};
        uint16_t elapsed = 0, nav = 0, repeats = 0;
        for (uint16_t frame = 0; frame < hz * 3; frame++)
        {
            elapsed += clock_step(&clock, hz, RIGHT, frame ? 0 : RIGHT, &nav);
            if (nav)
                repeats++;
        }
        CHECK(elapsed == 180 && repeats >= 23 && repeats <= 25);
        clock_step(&clock, hz, 0, 0, &nav);
        CHECK(!nav && !clock.held_age);
    }
    Campaign c;
    campaign_new(&c);
    CHECK(c.party[0].hp == 100 && c.party[2].mp == 32);
    CHECK(party_level(24) == 1 && party_level(25) == 2 && party_level(65) == 3 &&
          party_level(65535) == 4);
    Stats before = member_stats(&c, 0);
    CHECK(campaign_equip(&c, 0, 1));
    Stats after = member_stats(&c, 0);
    CHECK(after.attack == before.attack + 5 && after.defense == before.defense + 2 &&
          after.speed == before.speed - 2);
    CHECK(!campaign_equip(&c, 3, 1) && !campaign_equip(&c, 0, 2));
    CHECK(!campaign_item(&c, 0, 0) && c.items[0] == 6);
    c.party[0].hp = 99;
    CHECK(campaign_item(&c, 0, 0) && c.party[0].hp == 100 && c.items[0] == 5);
    c.party[1].hp = 0;
    CHECK(!campaign_item(&c, 0, 1));
    CHECK(campaign_item(&c, 2, 1) && c.party[1].hp == 56);
    c.party[2].mp = 31;
    CHECK(campaign_item(&c, 1, 2) && c.party[2].mp == 32);
    CHECK(!campaign_item(&c, 3, 0) && !campaign_item(&c, 0, 3));
    campaign_rest(&c);
    Battle b;
    battle_begin(&b, &c, 0, 123);
    Battle twin;
    Campaign clone = c;
    battle_begin(&twin, &clone, 0, 123);
    CHECK(battle_commit(&b, &c, (Action){STRIKE, 0, 3}) &&
          battle_commit(&twin, &clone, (Action){STRIKE, 0, 3}));
    CHECK(b.rng == twin.rng && b.event.amount == twin.event.amount);
    battle_begin(&b, &c, 0, 123);
    for (uint8_t i = 0; i < 6; i++)
        b.actors[i].stats.speed = 10;
    b.cursor = 5;
    b.order[5] = 0;
    CHECK(battle_commit(&b, &c, (Action){GUARD, 0, 0}));
    for (uint8_t i = 0; i < 6; i++)
        CHECK(b.order[i] == i);
    clone = c;
    clone.party[2].hp = 0;
    battle_begin(&b, &clone, 0, 123);
    CHECK(battle_commit(&b, &clone, (Action){GUARD, 0, 0}) && battle_actor(&b) == 1);
    battle_begin(&b, &c, 0, 123);
    CHECK(battle_actor(&b) == 0 && b.actors[3].hp == 44 && b.actors[5].hp == 0);
    Battle unchanged;
    Campaign unchanged_c;
    memcpy(&unchanged, &b, sizeof b);
    memcpy(&unchanged_c, &c, sizeof c);
    CHECK(!battle_commit(&b, &c, (Action){STRIKE, 0, 0}));
    CHECK(!battle_commit(&b, &c, (Action){ART, 2, 3}));
    CHECK(!battle_commit(&b, &c, (Action){ITEM, 0, 0}));
    CHECK(!memcmp(&b, &unchanged, sizeof b) && !memcmp(&c, &unchanged_c, sizeof c));
    CHECK(battle_commit(&b, &c, (Action){ART, 0, 3}) && b.actors[3].soaked == 2 &&
          b.actors[0].mp == 16);
    force_turn(&b, 1);
    CHECK(battle_commit(&b, &c, (Action){ART, 0, 3}) && b.event.combo);
    force_turn(&b, 2);
    b.actors[2].mp = 0;
    memcpy(&unchanged, &b, sizeof b);
    CHECK(!battle_commit(&b, &c, (Action){ART, 0, 0}) && !memcmp(&b, &unchanged, sizeof b));
    b.actors[2].mp = 32;
    b.actors[0].hp = 0;
    CHECK(!battle_target(&b, (Action){ART, 0, 0}, 0) && battle_target(&b, (Action){ART, 1, 0}, 0));
    CHECK(battle_commit(&b, &c, (Action){ART, 1, 0}) && b.actors[0].hp == 50);
    force_turn(&b, 0);
    CHECK(battle_commit(&b, &c, (Action){GUARD, 0, 0}));
    CHECK(b.actors[0].guarding);
    force_turn(&b, 3);
    b.actors[3].hp = 1;
    b.actors[3].species = ORRERY;
    b.round = 1;
    CHECK(battle_enemy(&b) && b.event.kind == CHARGE && b.actors[3].charging);
    force_turn(&b, 3);
    CHECK(battle_enemy(&b) && b.event.kind == HIT && !b.actors[3].charging);
    for (uint8_t encounter = 0; encounter < 3; encounter++)
    {
        battle_begin(&b, &c, encounter, 42);
        for (uint8_t i = 0; i < 6; i++)
            if (b.actors[i].hp)
            {
                int16_t x, y;
                CHECK(battle_position(&b, i, &x, &y));
                int16_t w = b.actors[i].species == ORRERY ? 64 : 32, h = i < 3 ? 48 : w;
                CHECK(x >= 0 && y >= 8 && x + w <= 320 && y + h <= 136);
                for (uint8_t j = i + 1; j < 6; j++)
                    if (b.actors[j].hp)
                    {
                        int16_t xx, yy;
                        CHECK(battle_position(&b, j, &xx, &yy));
                        int16_t ww = b.actors[j].species == ORRERY ? 64 : 32, hh = j < 3 ? 48 : ww;
                        CHECK(x + w <= xx || xx + ww <= x || y + h <= yy || yy + hh <= y);
                    }
            }
    }
    campaign_new(&c);
    battle_begin(&b, &c, 2, 42);
    force_turn(&b, 0);
    CHECK(battle_target(&b, (Action){STRIKE, 0, 3}, 3) &&
          battle_target(&b, (Action){STRIKE, 0, 4}, 4));
    CHECK(!battle_target(&b, (Action){STRIKE, 0, 5}, 5));
    b.actors[4].hp = 0;
    CHECK(!battle_target(&b, (Action){STRIKE, 0, 4}, 4) &&
          battle_target(&b, (Action){STRIKE, 0, 3}, 3));
    for (uint8_t encounter = 0; encounter < 3; encounter++)
    {
        campaign_rest(&c);
        battle_begin(&b, &c, encounter, 42 + encounter);
        uint16_t steps = 0;
        while (b.outcome == LIVE && steps++ < 500)
        {
            uint8_t actor = battle_actor(&b);
            if (actor >= 3)
            {
                CHECK(battle_enemy(&b));
                continue;
            }
            uint8_t target = 3;
            while (target < 6 && !b.actors[target].hp)
                target++;
            Action a = {STRIKE, 0, target};
            if (actor == 0 && b.actors[0].mp >= 4)
                a = (Action){ART, 0, target};
            else if (actor == 1 && b.actors[1].mp >= 5)
                a = (Action){ART, 0, target};
            else if (actor == 2 && b.actors[2].mp >= 4)
            {
                for (uint8_t i = 0; i < 3; i++)
                    if (b.actors[i].hp && b.actors[i].hp + 30 < b.actors[i].stats.hp)
                    {
                        a = (Action){ART, 0, i};
                        break;
                    }
            }
            CHECK(battle_commit(&b, &c, a));
        }
        CHECK(b.outcome == WON && steps < 500);
        uint16_t gold = c.gold;
        CHECK(battle_claim(&b, &c) && c.gold > gold);
        gold = c.gold;
        CHECK(!battle_claim(&b, &c) && c.gold == gold);
    }
    CHECK(c.complete && c.route == 2 && c.xp == 125 && party_level(c.xp) == 4);
    uint8_t bytes[SAVE_BYTES];
    campaign_encode(&c, bytes, 65535);
    Campaign decoded = {0};
    uint16_t sequence = 0;
    CHECK(campaign_decode(&decoded, bytes, &sequence) && sequence == 65535 && decoded.complete &&
          decoded.xp == c.xp);
    for (uint8_t i = 0; i < SAVE_BYTES; i++)
    {
        bytes[i] ^= 1;
        Campaign old;
        memcpy(&old, &decoded, sizeof old);
        CHECK(!campaign_decode(&decoded, bytes, &sequence) && !memcmp(&old, &decoded, sizeof old));
        bytes[i] ^= 1;
    }
    Game g;
    game_init(&g, false);
    press(&g, DOWN);
    press(&g, A);
    CHECK(g.screen == TITLE && !g.load_request);
    g.selection = 0;
    press(&g, A);
    CHECK(g.screen == HELP);
    press(&g, A);
    CHECK(g.screen == CAMP);
    press(&g, C);
    CHECK(g.screen == ROUTE);
    g.selection = 2;
    press(&g, A);
    CHECK(g.screen == ROUTE);
    g.selection = 0;
    press(&g, A);
    CHECK(g.screen == BATTLE && g.save_request);
    press(&g, START);
    CHECK(g.paused);
    Battle paused;
    memcpy(&paused, &g.battle, sizeof paused);
    uint32_t paused_ticks = g.ticks;
    for (uint16_t i = 0; i < 120; i++)
        game_step(&g, A, 0, 0, 1);
    CHECK(g.ticks == paused_ticks && !memcmp(&paused, &g.battle, sizeof paused));
    press(&g, START);
    CHECK(!g.paused);
    battle_begin(&g.battle, &g.campaign, 2, 42);
    force_turn(&g.battle, 0);
    g.battle_menu = COMMAND;
    g.selection = 0;
    press(&g, A);
    CHECK(g.battle_menu == TARGET && g.pending.target == 3);
    press(&g, RIGHT);
    CHECK(g.pending.target == 4);
    press(&g, RIGHT);
    CHECK(g.pending.target == 3);
    press(&g, LEFT);
    CHECK(g.pending.target == 4);
    g.battle.actors[4].hp = 0;
    press(&g, RIGHT);
    CHECK(g.pending.target == 3);
    press(&g, RIGHT);
    CHECK(g.pending.target == 3);
    press(&g, B);
    CHECK(g.battle_menu == COMMAND);
    g.battle.outcome = LOST;
    g.battle_menu = RESOLVE;
    game_step(&g, 0, 0, 0, 120);
    CHECK(g.screen == BATTLE);
    press(&g, A);
    CHECK(g.screen == RESULT);
    press(&g, A);
    CHECK(g.screen == CAMP && g.campaign.party[0].hp == g.checkpoint.party[0].hp);
    battle_begin(&g.battle, &g.campaign, 0, 42);
    g.screen = BATTLE;
    g.battle_menu = RESOLVE;
    g.elapsed = 0;
    g.battle.outcome = WON;
    game_step(&g, 0, 0, 0, 64);
    CHECK(g.screen == BATTLE);
    game_step(&g, 0, 0, 0, 1);
    CHECK(g.screen == RESULT);
    uint16_t reward = g.campaign.gold;
    for (uint16_t i = 0; i < 120; i++)
        game_step(&g, 0, 0, 0, 1);
    CHECK(g.campaign.gold == reward);
    return 0;
}
#ifdef SGDK_GCC
#include <genesis.h>
#include "resources.h"
static int resources(void)
{
    const Image *backgrounds[] = {&art_title, &art_harbor, &art_battle};
    const SpriteDefinition *actors[] = {&art_mara, &art_ivo,  &art_neri,
                                        &art_crab, &art_moth, &art_warden};
    uint16_t base = 16 + 96 + 16 + art_panel.tileset->numTile + 25 +
                    art_effect.animations[0]->frames[0]->tileset->numTile + 4;
    uint16_t actor_tiles = 0, max_tiles = 0;
    CHECK(art_font_ivory.numTile == 96 && art_font_gold.numTile == 96 && ui_palette.length >= 16);
    for (uint8_t i = 0; i < 6; i++)
    {
        const AnimationFrame *f = actors[i]->animations[0]->frames[0];
        actor_tiles += f->tileset->numTile;
        CHECK(actors[i]->numAnimation == 1 && actors[i]->animations[0]->numFrame == 1);
        CHECK(actors[i]->w == (i == 5 ? 64 : 32) && actors[i]->h == (i < 3    ? 48
                                                                     : i == 5 ? 64
                                                                              : 32));
        for (uint8_t color = 0; color < 16; color++)
            CHECK(actors[i]->palette->data[color] == art_mara.palette->data[color]);
    }
    for (uint8_t i = 0; i < 3; i++)
    {
        uint16_t total = base + backgrounds[i]->tileset->numTile + actor_tiles;
        CHECK(total <= 1440);
        if (total > max_tiles)
            max_tiles = total;
        CHECK(backgrounds[i]->palette->length == 64);
        for (uint16_t tile = 0; tile < backgrounds[i]->tilemap->w * backgrounds[i]->tilemap->h;
             tile++)
            CHECK(((backgrounds[i]->tilemap->tilemap[tile] >> 13) & 3) <= 1);
    }
    CHECK(art_field_idle.w == 40 && art_field_idle.h == 40 &&
          art_field_idle.animations[0]->numFrame == 4);
    CHECK(art_field_walk.numAnimation == 4);
    for (uint8_t a = 0; a < 4; a++)
    {
        CHECK(art_field_walk.animations[a]->numFrame == 4);
        for (uint8_t f = 0; f < 4; f++)
            CHECK(art_field_walk.animations[a]->frames[f]->tileset->numTile <= 25);
    }
    Campaign c;
    campaign_new(&c);
    uint16_t maximum_sprites = 0, maximum_pixels = 0, maximum_total = 0;
    for (uint8_t encounter = 0; encounter < 3; encounter++)
    {
        Battle b;
        battle_begin(&b, &c, encounter, 42);
        uint16_t total = 2;
        for (uint8_t i = 0; i < 6; i++)
            if (b.actors[i].hp)
                total += actors[b.actors[i].species]->animations[0]->frames[0]->numSprite & 0x7F;
        CHECK(total <= 80);
        if (total > maximum_total)
            maximum_total = total;
        for (int16_t line = 0; line < 224; line++)
        {
            uint16_t sprites = 2, pixels = 48; /* Conservative effect + marker on every row. */
            for (uint8_t i = 0; i < 6; i++)
                if (b.actors[i].hp)
                {
                    int16_t x, y;
                    CHECK(battle_position(&b, i, &x, &y));
                    const AnimationFrame *f = actors[b.actors[i].species]->animations[0]->frames[0];
                    for (uint8_t p = 0; p < (f->numSprite & 0x7F); p++)
                    {
                        const FrameVDPSprite *piece = &f->frameVDPSprites[p];
                        uint16_t width = (((piece->size >> 2) & 3) + 1) * 8,
                                 height = ((piece->size & 3) + 1) * 8;
                        if (line >= y + piece->offsetY && line < y + piece->offsetY + height)
                        {
                            sprites++;
                            pixels += width;
                        }
                    }
                }
            CHECK(sprites <= 20 && pixels <= 320);
            if (sprites > maximum_sprites)
                maximum_sprites = sprites;
            if (pixels > maximum_pixels)
                maximum_pixels = pixels;
        }
    }
    volatile uint16_t *output = (volatile uint16_t *)0xC00000;
    *output = sizeof(Campaign);
    *output = sizeof(Battle);
    *output = sizeof(Game);
    *output = max_tiles;
    *output = maximum_total;
    *output = maximum_sprites;
    *output = maximum_pixels;
    return 0;
}
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    int error = checks();
    return error ? error : resources();
}
#else
int main(void)
{
    int error = checks();
    if (error)
        fprintf(stderr, "Adventure check failed: %d\n", error);
    return error ? 1 : 0;
}
#endif
