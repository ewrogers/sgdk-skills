#include "demo.h"

enum { ANIM_IDLE, ANIM_BURST };

static Sprite *actor;
static const u16 *actor_palette;
static u16 previous, scroll_phase, cycle_ticks, cycle_phase, completions;
static s16 actor_x;
static bool stopped, burst_pending, faded;
static const u16 cycle_colors[3] = {0x0E60, 0x0EC0, 0x0ECA};

bool demo_init(const Image *far_image, const Image *near_image,
               const SpriteDefinition *definition)
{
    u32 next;
    u16 tile;
    if (!far_image || !near_image || !definition || actor) return FALSE;
    SPR_init();
    tile = TILE_USER_INDEX;
    next = (u32)tile + far_image->tileset->numTile + near_image->tileset->numTile;
    /* End is exclusive; the SDK's user maximum is inclusive. */
    if (next > (u32)TILE_USER_MAX_INDEX + 1)
    {
        SPR_end();
        return FALSE;
    }
    if (!VDP_drawImageEx(BG_B, far_image, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, tile),
                         0, 0, FALSE, FALSE))
    {
        SPR_end();
        return FALSE;
    }
    tile += far_image->tileset->numTile;
    if (!VDP_drawImageEx(BG_A, near_image, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, tile),
                         0, 0, FALSE, FALSE))
    {
        SPR_end();
        return FALSE;
    }
    PAL_setPalette(PAL0, far_image->palette->data, CPU);
    PAL_setPalette(PAL1, near_image->palette->data, CPU);
    PAL_setPalette(PAL2, definition->palette->data, CPU);
    actor = SPR_addSprite(definition, 152, 112, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    if (!actor)
    {
        SPR_end();
        return FALSE;
    }
    actor_palette = definition->palette->data;
    previous = scroll_phase = cycle_ticks = cycle_phase = completions = 0;
    actor_x = 152;
    stopped = burst_pending = faded = FALSE;
    /* Initialize the pending first sprite frame before play/stop can use it. */
    SPR_update();
    return TRUE;
}

void demo_step(u16 held)
{
    u16 pressed;
    if (!actor) return;
    pressed = held & (u16)~previous;
    previous = held;

    if (pressed & BUTTON_A)
    {
        stopped = !stopped;
        SPR_setAutoAnimation(actor, !stopped);
    }
    if ((pressed & BUTTON_B) && !burst_pending)
    {
        SPR_setAnimAndFrame(actor, ANIM_BURST, 0);
        SPR_setAnimationLoop(actor, FALSE);
        SPR_setAutoAnimation(actor, TRUE);
        stopped = FALSE;
        burst_pending = TRUE;
    }
    /* Poll once and latch. The SDK predicate is not a queued event. */
    if (burst_pending && !stopped && SPR_isAnimationDone(actor))
    {
        ++completions;
        burst_pending = FALSE;
        /* Keep the final frame visible until another requested animation. */
    }
    if ((pressed & BUTTON_C) && !PAL_isDoingFade())
    {
        if (faded) PAL_fadeInPalette(PAL2, actor_palette, 30, TRUE);
        else PAL_fadeOutPalette(PAL2, 30, TRUE);
        faded = !faded;
    }

    actor_x += ((held & BUTTON_RIGHT) != 0) - ((held & BUTTON_LEFT) != 0);
    if (actor_x < 0) actor_x = 0;
    if (actor_x > 304) actor_x = 304;
    SPR_setPosition(actor, actor_x, 112);

    /* A 512-pixel wrapping plane; retain enough phase for half-speed B. */
    scroll_phase = (scroll_phase + 1) & 1023;
    VDP_setHorizontalScrollVSync(BG_A, -(s16)(scroll_phase & 511));
    VDP_setHorizontalScrollVSync(BG_B, -(s16)(scroll_phase >> 1));

    if (++cycle_ticks == 8)
    {
        u16 colors[3];
        u16 i;
        cycle_ticks = 0;
        cycle_phase = (cycle_phase + 1) % 3;
        for (i = 0; i < 3; ++i) colors[i] = cycle_colors[(i + cycle_phase) % 3];
        /* COPY is required: this stack array expires before VBlank. */
        PAL_setColors(17, colors, 3, DMA_QUEUE_COPY);
    }
    SPR_update();
}

void demo_end(void)
{
    if (!actor) return;
    /* This standalone demo owns the global fade and the sprite engine. */
    PAL_interruptFade();
    SYS_doVBlankProcess(); /* Finish any upload before releasing its owner. */
    SPR_releaseSprite(actor);
    actor = NULL;
    SPR_end();
    SYS_doVBlankProcess(); /* SPR_end queues the empty sprite table. */
}

u16 demo_completions(void)
{
    return completions;
}
