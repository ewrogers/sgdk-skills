#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "desk_view.h"

static DeskView view;
static unsigned short fake_vram[VIEW_PLANES][VIEW_CELLS];
static const unsigned short *borrowed[VIEW_MAX_ROWS];
static unsigned char planes[VIEW_MAX_ROWS], rows[VIEW_MAX_ROWS], queued;
static unsigned short captured[VIEW_MAX_ROWS][VIEW_W];

static void queue(void *context, unsigned char plane, unsigned char row,
                   const unsigned short *words) {
    (void)context;
    assert(queued < VIEW_MAX_ROWS && plane < VIEW_PLANES && row < VIEW_H);
    assert(view.pending);
    borrowed[queued] = words; planes[queued] = plane; rows[queued] = row;
    memcpy(captured[queued], words, sizeof captured[queued]);
    ++queued;
}

static void drain(void) {
    unsigned char i;
    for (i = 0; i < queued; ++i) {
        assert(memcmp(borrowed[i], captured[i], sizeof captured[i]) == 0);
        memcpy(fake_vram[planes[i]] + rows[i] * VIEW_W, borrowed[i], sizeof captured[i]);
    }
    view_complete(&view);
    assert(memcmp(fake_vram, view.next, sizeof fake_vram) == 0);
    queued = 0;
}

static void border(unsigned short x, unsigned short y,
                    unsigned short w, unsigned short h) {
    unsigned short i;
    for (i = x; i < x + w; ++i) {
        assert(view.next[0][y * VIEW_W + i] == ((16 + ART_H) | (1 << 13)));
        assert(view.next[0][(y + h - 1) * VIEW_W + i] == ((16 + ART_H) | (1 << 13)));
    }
    for (i = y + 1; i < y + h - 1; ++i) {
        if (view.next[0][i * VIEW_W + x + w - 1] != ((16 + ART_V) | (1 << 13)))
            fprintf(stderr, "Panel right border overwritten at %u,%u (tile %u)\n",
                    x + w - 1, i, view.next[0][i * VIEW_W + x + w - 1]);
        assert(view.next[0][i * VIEW_W + x] == ((16 + ART_V) | (1 << 13)));
        assert(view.next[0][i * VIEW_W + x + w - 1] == ((16 + ART_V) | (1 << 13)));
    }
}

static void compose(DeskUI *ui) {
    assert(view_compose(&view, ui, 1400, 16));
    assert(view.clipped == 0);
    /* Text can fit the screen yet overwrite its own panel's border. */
    if (ui->guide) border(1, 4, 38, 19);
    else if (ui->screen == UI_TITLE) border(1, 4, 38, 12);
    else if (ui->screen == UI_REPORT) border(1, 4, 38, 14);
    else {
        border(1, 4, 18, 10); border(21, 4, 18, 10); border(1, 14, 38, 9);
    }
}

static void layout(DeskUI *ui) {
    unsigned char count;
    compose(ui);
    count = view_queue(&view, queue, NULL);
    assert(count == queued && count <= VIEW_MAX_ROWS);
    assert((unsigned)count * VIEW_W * 2 <= 4480);
    if (count) {
        assert(!view_compose(&view, ui, 1400, 16));
        assert(view_queue(&view, queue, NULL) == 0);
    }
    drain();
    compose(ui);
    assert(view_queue(&view, queue, NULL) == 0); /* No blank/repaint for unchanged fields. */
    assert(!view.pending);
}

static void help_and_actions(void) {
    DeskUI ui;
    unsigned char i;
    unsigned short tick, remaining, phase;
    unsigned long waited;
    ui_init(&ui);
    ui_frame(&ui, K_START, 60);
    assert(ui.guide && ui.screen == UI_TITLE);
    for (i = 0; i < 10; ++i) ui_frame(&ui, K_RIGHT, 60);
    assert(ui.guide_page == 2);
    for (i = 0; i < 10; ++i) ui_frame(&ui, K_LEFT, 60);
    assert(ui.guide_page == 0);
    ui_frame(&ui, K_START, 60);
    ui_frame(&ui, K_C, 60);
    assert(ui.screen == UI_DESK && ui.desk.service[0].state == WAITING);
    ui_frame(&ui, K_C, 60);
    assert(ui.desk.service[0].state == TRANSIT && ui.cue == 1);
    ui_frame(&ui, K_A, 50);
    tick = ui.desk.tick; remaining = ui.desk.service[0].remaining;
    phase = ui.desk.phase; waited = ui.desk.wait_ticks;
    ui_frame(&ui, K_START, 50);
    for (i = 0; i < 100; ++i) ui_frame(&ui, K_A | K_DOWN, 50);
    assert(ui.desk.held && ui.desk.tick == tick && ui.desk.phase == phase);
    assert(ui.desk.wait_ticks == waited && ui.desk.service[0].remaining == remaining);
    ui_frame(&ui, K_C, 50); ui_frame(&ui, K_C, 50); ui_frame(&ui, K_C, 50);
    assert(!ui.guide && !ui.desk.help && ui.desk.held && ui.desk.tick == tick);
    ui_frame(&ui, K_A, 50);
    assert(!ui.desk.held);
    tick = ui.desk.tick;
    ui_frame(&ui, K_START, 50);
    ui_frame(&ui, K_START, 50);
    assert(!ui.guide && !ui.desk.held && ui.desk.tick == tick);

    /* Close the final page with C while dispatch really is possible. */
    ui_init(&ui); ui_frame(&ui, K_C, 60);
    for (tick = 0; tick < 600; ++tick) ui_frame(&ui, 0, 60);
    assert(desk_route_ok(&ui.desk, ui.desk.selected, ui.desk.bay));
    tick = ui.desk.tick;
    ui_frame(&ui, K_START, 60);
    ui_frame(&ui, K_C, 60); ui_frame(&ui, K_C, 60); ui_frame(&ui, K_C, 60);
    assert(!ui.guide && ui.desk.tick == tick);
    assert(desk_count(&ui.desk, TRANSIT) == 0);
}

int main(void) {
    DeskUI ui;
    unsigned short i;
    unsigned char page;
    help_and_actions();
    ui_init(&ui); view_init(&view);
    layout(&ui);
    ui.choice = 1; layout(&ui);
    ui.guide = 1;
    for (page = 0; page < 3; ++page) { ui.guide_page = page; layout(&ui); }
    ui.guide = 0;
    ui_frame(&ui, K_UP, 60); ui_frame(&ui, K_C, 60);
    assert(ui.screen == UI_DESK);
    layout(&ui);
    ui_frame(&ui, K_C, 60); layout(&ui);
    for (i = 0; i < 5400; ++i) {
        ui_frame(&ui, i % 73 == 0 ? K_C : i % 41 == 0 ? K_RIGHT : 0, 60);
        if (i % 6 == 0) layout(&ui);
    }
    assert(ui.screen == UI_REPORT);
    layout(&ui);
    ui.choice = 1; layout(&ui);
    ui_frame(&ui, K_C, 60); assert(ui.screen == UI_TITLE); layout(&ui);
    puts("Guide/hold/navigation, bounded layouts, stable queued sources and changed-row updates pass.");
    return 0;
}
