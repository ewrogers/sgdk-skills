#include <genesis.h>
#include "workload.h"
#include "resources.h"

static PerfWorld world;
static PerfWindow window;
static Sprite *sprites[PERF_CAPACITY];
/* Permanent ROM source; BG_B's unused 64x32 tilemap is the transfer destination. */
static const u16 transfer_pad[2048] = {0};
static const u16 dma_steps[] = {0, 512, 2048, 4096};
static u16 count = 32, mode = 1, dma_step;
static bool animated, overlay = true;
static bool window_overlay;
static u32 dma_refused;

/* Readable with a compatible debugger even when the text overlay is hidden. */
volatile u32 perf_last_fps;
volatile u16 perf_last_cpu, perf_queued_bytes;
volatile PerfContacts perf_last_contacts;

static void print_number(u32 value, u16 x, u16 y, u16 width)
{
    char text[12];
    uintToStr(value, text, 1);
    if (strlen(text) > width) {
        for (u16 i = 0; i < width; i++) text[i] = '#';
        text[width] = 0;
    }
    VDP_drawTextFill(text, x, y, width);
}

static void metrics(void)
{
    if (!overlay) return;
    VDP_drawText("Hz:     CPU est:     %", 1, 4);
    print_number(perf_last_fps, 5, 4, 3);
    print_number(perf_last_cpu, 18, 4, 3);
    VDP_drawText("Checks:      Hits:", 1, 5);
    print_number(perf_last_contacts.candidates, 9, 5, 5);
    print_number(perf_last_contacts.contacts, 19, 5, 4);
    VDP_drawText("DMA queued:       Refused:", 1, 6);
    print_number(perf_queued_bytes, 13, 6, 5);
    print_number(dma_refused, 27, 6, 6);
    VDP_drawText("Window:     /120  Mean:     Peak:", 1, 7);
    print_number(window.samples, 9, 7, 3);
    print_number(perf_window_mean(&window), 25, 7, 3);
    print_number(window.peak_cpu, 35, 7, 3);
    VDP_drawText("Missed refreshes:      DMA peak:", 1, 8);
    print_number(window.missed_refreshes, 19, 8, 5);
    print_number(window.peak_dma, 33, 8, 5);
    VDP_drawTextFill(window.frozen ?
        (window_overlay ? "Frozen window: metrics on" : "Frozen window: metrics off") :
        window.warmup ? "Warming up" : "Recording window", 1, 9, 38);
}

static void headings(void)
{
    VDP_drawText("SGDK PERFORMANCE LAB", 1, 0);
    VDP_drawText(mode == 0 ? "ALL PAIRS / SPREAD  " :
                 mode == 1 ? "GRID / SPREAD       " : "GRID / ONE SCANLINE ", 1, 1);
    VDP_drawText("Bodies:     Anim:      DMA extra:", 1, 2);
    print_number(count, 9, 2, 3);
    VDP_drawText(animated ? "On " : "Off", 19, 2);
    print_number(dma_steps[dma_step], 34, 2, 4);
    VDP_drawText("Up/Down count  Left/Right case", 1, 25);
    VDP_drawText("A animation  B DMA  C metrics", 1, 26);
    VDP_drawText("Start repeats the same run", 1, 27);
}

static void restart(void)
{
    perf_reset(&world, count);
    perf_window_reset(&window);
    window_overlay = overlay;
    dma_refused = 0;
    headings();
    for (u16 i = 0; i < PERF_CAPACITY; i++)
        SPR_setVisibility(sprites[i], i < count ? VISIBLE : HIDDEN);
}

static void render(void)
{
    u16 frame = perf_frame(&world, animated);
    for (u16 i = 0; i < count; i++) {
        SPR_setPosition(sprites[i], world.bodies[i].x,
                        mode == 2 ? 144 : (s16)(80 + world.bodies[i].y));
        SPR_setFrame(sprites[i], (s16)frame);
    }
    SPR_update();
    if (dma_steps[dma_step] &&
        !DMA_transfer(DMA_QUEUE, DMA_VRAM, (void *)transfer_pad, VDP_BG_B,
                      dma_steps[dma_step] / 2, 2)) dma_refused++;
    perf_queued_bytes = DMA_getQueueTransferSize();
}

static bool setup(void)
{
    VDP_setEnable(false);
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, true);
    DMA_setMaxQueueSize(128);  /* Configure before placing work in the queue. */
    PAL_setPalette(PAL0, palette_grey, CPU);
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0x88BBFF));
    PAL_setPalette(PAL1, orb_image.palette->data, CPU);
    SPR_initEx(PERF_CAPACITY * orb_image.maxNumTile);
    for (u16 i = 0; i < PERF_CAPACITY; i++) {
        sprites[i] = SPR_addSprite(&orb_image, 0, 0, TILE_ATTR(PAL1, false, false, false));
        if (!sprites[i]) return false;
        SPR_setAutoAnimation(sprites[i], false);
    }
    return true;
}

int main(bool hard_reset)
{
    u16 previous = 0, hud_ticks = 0;
    u32 last_refresh;
    (void)hard_reset;
    if (!setup()) {
        SPR_end();
        VDP_drawText("Sprite setup failed", 1, 1);
        VDP_setEnable(true);
        while (true) SYS_doVBlankProcess();
    }
    restart(); render();
    SYS_doVBlankProcess();
    VDP_setEnable(true);
    last_refresh = vtimer;
    while (true) {
        u16 held = JOY_readJoypad(JOY_1);
        u16 pressed = held & (u16)~previous;
        bool changed = false;
        previous = held;
        if ((pressed & BUTTON_UP) && count < PERF_CAPACITY) { count += 8; changed = true; }
        if ((pressed & BUTTON_DOWN) && count > 0) { count -= 8; changed = true; }
        if (pressed & BUTTON_LEFT) { mode = (u16)((mode + 2) % 3); changed = true; }
        if (pressed & BUTTON_RIGHT) { mode = (u16)((mode + 1) % 3); changed = true; }
        if (pressed & BUTTON_A) { animated = !animated; changed = true; }
        if (pressed & BUTTON_B) { dma_step = (u16)((dma_step + 1) % 4); changed = true; }
        if (pressed & BUTTON_C) {
            overlay = !overlay;
            for (u16 y = 4; y <= 9; y++) VDP_clearTextLine(y);
            if (!overlay) changed = true;
            else window.frozen = true;  /* Reveal the hidden run without replacing it. */
            hud_ticks = 0;
        }
        if (changed || (pressed & BUTTON_START)) { restart(); hud_ticks = 0; }
        perf_move(&world);
        perf_last_contacts = mode == 0 ? perf_all_pairs(&world, 0) : perf_grid(&world, 0);
        render();
        perf_last_fps = SYS_getFPS();  /* One shared-counter call per game update. */
        perf_last_cpu = SYS_getCPULoad();
        if (hud_ticks++ % 30 == 0) metrics();
        SYS_doVBlankProcess();
        u32 now = vtimer;
        perf_window_push(&window, perf_last_cpu, perf_queued_bytes, now - last_refresh);
        last_refresh = now;
    }
    return 0;
}
