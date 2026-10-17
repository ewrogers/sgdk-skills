#include <genesis.h>
#include "desk_view.h"

static DeskUI ui;
static DeskView view;
static u16 previous;
static u8 fps, tone_left, tone_kind;

/* Native 8x8 patterns. Font stays in the SDK's reserved font slots. */
static const u32 art[ART_COUNT * 8] = {
    0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
    0,0,0x22222222,0x22222222,0,0x22222222,0,0,
    0,0,0x33333333,0x33333333,0,0x33333333,0,0,
    0,0x22222222,0x23333332,0x23223232,0x23333332,0x22222222,0x02000020,0,
    0x11111111,0x11111111,0x11111111,0x22222222,0x11111111,0x11111111,0x11111111,0x11111111,
    0x11121111,0x11121111,0x11121111,0x11121111,0x11121111,0x11121111,0x11121111,0x11121111
};

static u16 actions(u16 pressed) {
    u16 result = 0;
    if (pressed & BUTTON_UP) result |= K_UP;
    if (pressed & BUTTON_DOWN) result |= K_DOWN;
    if (pressed & BUTTON_LEFT) result |= K_LEFT;
    if (pressed & BUTTON_RIGHT) result |= K_RIGHT;
    if (pressed & BUTTON_A) result |= K_A;
    if (pressed & BUTTON_B) result |= K_B;
    if (pressed & BUTTON_C) result |= K_C;
    if (pressed & BUTTON_START) result |= K_START;
    return result;
}

static void queue_row(void *context, unsigned char plane, unsigned char row,
                       const unsigned short *words) {
    (void)context;
    /* Sole DMA producer: at most 56 operations / 4480 bytes, below SDK
       defaults of 80 operations and 7200 bytes even on NTSC. */
    VDP_setTileMapDataRow(plane ? BG_B : BG_A, words, row, 0, VIEW_W, DMA_QUEUE);
}

static void sound_frame(u8 cue) {
    if (tone_left && --tone_left == 0) PSG_setEnvelope(0, 15);
    if (tone_kind == 3 && tone_left == fps / 2) PSG_setFrequency(0, 330);
    if (!cue) return;
    tone_kind = cue;
    tone_left = cue == 3 ? fps : fps / 8;
    PSG_setFrequency(0, cue == 1 ? 660 : cue == 2 ? 155 : 440);
    PSG_setEnvelope(0, cue == 2 ? 6 : 9);
}

int main(bool hard_reset) {
    u16 i, snapshot;
    u8 first = TRUE, refresh = 0;
    (void)hard_reset;
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    fps = SYS_isPAL() ? 50 : 60;
    for (i = 0; i < 4; ++i) {
        PAL_setColor(i * 16, 0xACC);
        PAL_setColor(i * 16 + 1, i == 0 ? 0xACC : i == 1 ? 0xCEE : i == 2 ? 0x6CE : 0xCCE);
        PAL_setColor(i * 16 + 2, 0x444);
        PAL_setColor(i * 16 + 3, 0x26A);
        PAL_setColor(i * 16 + 15, i == 1 ? 0x666 : i == 3 ? 0x226 : 0x222);
        PSG_setEnvelope(i, 15);
    }
    VDP_setBackgroundColor(0);
    VDP_loadTileData(art, TILE_USER_INDEX, ART_COUNT, CPU);
    Z80_loadDriver(Z80_DRIVER_NULL, TRUE);
    ui_init(&ui);
    view_init(&view);
    while (TRUE) {
        u16 pressed;
        snapshot = JOY_readJoypad(JOY_1);
        pressed = actions(snapshot & (u16)~previous);
        previous = snapshot;
        ui_frame(&ui, pressed, fps);
        sound_frame(ui.cue);
        if (first || pressed || ui.cue || ++refresh >= fps / 10) {
            refresh = 0;
            view_compose(&view, &ui, TILE_FONT_INDEX, TILE_USER_INDEX);
            view_queue(&view, queue_row, NULL);
        }
        /* No active-display text clears or writes. Source rows remain owned
           by view until this call drains the queue. */
        SYS_doVBlankProcess();
        view_complete(&view);
        if (first) { VDP_setEnable(TRUE); first = FALSE; }
    }
    return 0;
}
