#include "demo.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Sprite instance;
static u16 colors[16];
static Palette palette = {colors};
static TileSet far_tiles = {3}, near_tiles = {5};
static Image far_image = {&palette, &far_tiles}, near_image = {&palette, &near_tiles};
static SpriteDefinition definition = {&palette};
u16 mock_user_max = 100;
static int init_calls, end_calls, draw_calls, fail_draw, update_calls, release_calls;
static int set_animation_calls, automatic_calls, fade_in, fade_out, color_calls, flushes;
static bool fail_sprite, done, automatic = true, looping = true, busy;
static s16 position, scroll_a, scroll_b;
static u16 drawn_tiles[2], copied_colors[3];
static char events[128];
static size_t event_count;

static void event(char c) { if (event_count + 1 < sizeof events) events[event_count++] = c; }
void SPR_init(void) { ++init_calls; mock_user_max = 100; }
void SPR_end(void) { ++end_calls; event('E'); }
Sprite *SPR_addSprite(const SpriteDefinition *d, s16 x, s16 y, u16 attr)
{
    assert(d == &definition && x == 152 && y == 112 && attr == TILE_ATTR(PAL2, 0, 0, 0));
    return fail_sprite ? NULL : &instance;
}
void SPR_releaseSprite(Sprite *s) { assert(s == &instance); ++release_calls; event('R'); }
void SPR_update(void) { ++update_calls; }
void SPR_setAutoAnimation(Sprite *s, bool value)
{ assert(s == &instance); automatic = value; ++automatic_calls; }
void SPR_setAnimAndFrame(Sprite *s, s16 animation, s16 frame)
{ assert(s == &instance && animation == 1 && frame == 0); ++set_animation_calls; done = false; }
void SPR_setAnimationLoop(Sprite *s, bool value) { assert(s == &instance); looping = value; }
bool SPR_isAnimationDone(Sprite *s) { assert(s == &instance); return done; }
void SPR_setPosition(Sprite *s, s16 x, s16 y) { assert(s == &instance && y == 112); position = x; }
bool VDP_drawImageEx(VDPPlane plane, const Image *image, u16 attr, u16 x, u16 y, bool pal, bool dma)
{
    assert(draw_calls < 2 && x == 0 && y == 0 && !pal && !dma);
    assert(plane == (draw_calls == 0 ? BG_B : BG_A));
    assert(image == (draw_calls == 0 ? &far_image : &near_image));
    assert((attr >> 13) == (draw_calls == 0 ? PAL0 : PAL1));
    drawn_tiles[draw_calls++] = attr & 0x7FF;
    return draw_calls != fail_draw;
}
void VDP_setHorizontalScrollVSync(VDPPlane p, s16 value)
{ assert(p == BG_A || p == BG_B); if (p == BG_A) scroll_a = value; else scroll_b = value; }
void PAL_setPalette(u16 p, const u16 *data, TransferMethod tm)
{ assert(p <= PAL2 && data == colors && tm == CPU); }
void PAL_setColors(u16 first, const u16 *data, u16 count, TransferMethod tm)
{
    assert(first == 17 && count == 3 && tm == DMA_QUEUE_COPY);
    memcpy(copied_colors, data, sizeof copied_colors); ++color_calls;
}
void PAL_fadeInPalette(u16 p, const u16 *data, u16 frames, bool async)
{ assert(p == PAL2 && data == colors && frames == 30 && async); ++fade_in; busy = true; }
void PAL_fadeOutPalette(u16 p, u16 frames, bool async)
{ assert(p == PAL2 && frames == 30 && async); ++fade_out; busy = true; }
bool PAL_isDoingFade(void) { return busy; }
void PAL_interruptFade(void) { busy = false; event('I'); }
bool SYS_doVBlankProcess(void) { ++flushes; event('V'); return true; }

static void start(void) { assert(demo_init(&far_image, &near_image, &definition)); }
static void tick(int count, u16 held) { while (count-- > 0) demo_step(held); }

int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *test = argv[1];
    if (!strcmp(test, "allocation"))
    {
        /* SPR_init changes the available range: don't budget before it. */
        mock_user_max = 65535; far_tiles.numTile = 80; near_tiles.numTile = 5;
        start(); assert(drawn_tiles[0] == 16 && drawn_tiles[1] == 96);
        assert(update_calls == 1 && init_calls == 1);
    }
    else if (!strcmp(test, "overflow"))
    {
        far_tiles.numTile = 65535;
        assert(!demo_init(&far_image, &near_image, &definition));
        assert(end_calls == 1 && draw_calls == 0);
        demo_step(BUTTON_B); assert(update_calls == 0);
    }
    else if (!strcmp(test, "one_tile_over"))
    {
        far_tiles.numTile = 81; near_tiles.numTile = 5;
        assert(!demo_init(&far_image, &near_image, &definition));
        assert(end_calls == 1 && draw_calls == 0);
    }
    else if (!strcmp(test, "draw_failure"))
    {
        fail_draw = 2;
        assert(!demo_init(&far_image, &near_image, &definition));
        assert(end_calls == 1 && draw_calls == 2 && update_calls == 0);
    }
    else if (!strcmp(test, "sprite_failure"))
    {
        fail_sprite = true;
        assert(!demo_init(&far_image, &near_image, &definition));
        assert(end_calls == 1 && release_calls == 0 && update_calls == 0);
    }
    else if (!strcmp(test, "initialization"))
    {
        assert(!demo_init(NULL, &near_image, &definition)); assert(init_calls == 0);
        start(); assert(!demo_init(&far_image, &near_image, &definition)); assert(init_calls == 1);
    }
    else if (!strcmp(test, "pause_edges"))
    {
        start(); tick(10, BUTTON_A); assert(!automatic && automatic_calls == 1);
        demo_step(0); demo_step(BUTTON_A); assert(automatic && automatic_calls == 2);
        assert(update_calls == 13); /* One per step, even when playback stopped. */
    }
    else if (!strcmp(test, "completion"))
    {
        start(); demo_step(BUTTON_B); assert(!looping && automatic);
        tick(4, BUTTON_B); assert(set_animation_calls == 1 && demo_completions() == 0);
        done = true; tick(10, 0); assert(demo_completions() == 1);
        demo_step(BUTTON_B); assert(set_animation_calls == 2 && demo_completions() == 1);
        done = true; demo_step(0); assert(demo_completions() == 2);
    }
    else if (!strcmp(test, "paused_completion"))
    {
        start(); demo_step(BUTTON_B); demo_step(BUTTON_A); done = true;
        tick(3, 0); assert(demo_completions() == 0);
        demo_step(BUTTON_A); assert(demo_completions() == 1);
    }
    else if (!strcmp(test, "fade"))
    {
        start(); demo_step(BUTTON_C); tick(8, BUTTON_C); assert(fade_out == 1 && fade_in == 0);
        demo_step(0); demo_step(BUTTON_C); assert(fade_in == 0); /* Busy: ignore. */
        busy = false; demo_step(0); demo_step(BUTTON_C); assert(fade_in == 1);
    }
    else if (!strcmp(test, "cycle"))
    {
        start(); demo_step(BUTTON_C); tick(6, 0); assert(color_calls == 0);
        demo_step(0); assert(color_calls == 1 && busy);
        assert(copied_colors[0] == 0x0EC0 && copied_colors[1] == 0x0ECA && copied_colors[2] == 0x0E60);
        tick(16, 0); assert(color_calls == 3 && copied_colors[0] == 0x0E60);
    }
    else if (!strcmp(test, "scroll"))
    {
        start(); demo_step(0); assert(scroll_a == -1 && scroll_b == 0);
        tick(510, 0); assert(scroll_a == -511 && scroll_b == -255);
        demo_step(0); assert(scroll_a == 0 && scroll_b == -256);
        tick(511, 0); assert(scroll_a == -511 && scroll_b == -511);
        demo_step(0); assert(scroll_a == 0 && scroll_b == 0);
    }
    else if (!strcmp(test, "movement"))
    {
        start(); tick(200, BUTTON_LEFT); assert(position == 0);
        tick(400, BUTTON_RIGHT); assert(position == 304);
        demo_step(BUTTON_LEFT | BUTTON_RIGHT); assert(position == 304);
    }
    else if (!strcmp(test, "teardown"))
    {
        start(); demo_step(BUTTON_C); demo_end();
        assert(!strcmp(events, "IVREV") && flushes == 2 && release_calls == 1 && !busy);
        int updates = update_calls; demo_step(BUTTON_B); demo_end();
        assert(updates == update_calls && end_calls == 1);
        draw_calls = 0; start(); assert(demo_completions() == 0);
    }
    else { fprintf(stderr, "unknown test: %s\n", test); return 2; }
    puts(test);
    return 0;
}
