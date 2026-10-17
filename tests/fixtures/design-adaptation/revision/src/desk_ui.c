#include "desk_ui.h"

UIRoute ui_route(const Desk *d, unsigned char bay) {
    UIRoute route = {0, 0, 0, 0};
    unsigned char i;
    if (bay > 3) return route;
    if (d->selected < SERVICES && d->service[d->selected].state == WAITING)
        route.required = desk_mask(d->service[d->selected].origin, bay);
    for (i = 0; i < SERVICES; ++i) if (d->service[i].state == TRANSIT) {
        const Service *s = &d->service[i];
        unsigned char overlap = route.required & desk_mask(s->origin, s->bay);
        if (s->bay == bay) route.occupied = 1;
        if (overlap || s->bay == bay) {
            unsigned char seconds = (unsigned char)((s->remaining + 59) / 60);
            route.blocked |= overlap;
            if (seconds > route.wait_seconds) route.wait_seconds = seconds;
        }
    }
    return route;
}

void ui_init(DeskUI *ui) {
    desk_reset(&ui->desk);
    ui->screen = UI_TITLE;
    ui->choice = ui->guide = ui->guide_page = ui->cue = 0;
}

void ui_frame(DeskUI *ui, unsigned short pressed, unsigned char fps) {
    ui->cue = 0;
    if (ui->guide) {
        if (pressed & K_START) {
            ui->guide = 0;
        } else if (pressed & K_LEFT) {
            if (ui->guide_page) --ui->guide_page;
        } else if (pressed & K_RIGHT) {
            if (ui->guide_page + 1 < UI_GUIDE_PAGES) ++ui->guide_page;
        } else if (pressed & K_C) {
            if (ui->guide_page + 1 < UI_GUIDE_PAGES) ++ui->guide_page;
            else ui->guide = 0;
        }
        if (!ui->guide && ui->screen == UI_DESK)
            desk_input(&ui->desk, K_START);
        return; /* Closing help cannot also dispatch or advance the clock. */
    }
    if (ui->screen == UI_DESK) {
        if (pressed & K_START) {
            ui->guide = 1;
            ui->guide_page = 0;
            desk_input(&ui->desk, K_START);
            return;
        }
        desk_input(&ui->desk, pressed);
        ui->cue = ui->desk.sound;
        desk_frame(&ui->desk, fps);
        if (ui->desk.finished) {
            ui->screen = UI_REPORT;
            ui->choice = 0;
            ui->cue = 3;
        }
        return;
    }
    if (ui->screen == UI_TITLE && (pressed & K_START)) {
        ui->guide = 1;
        ui->guide_page = 0;
    } else if (pressed & (K_UP | K_DOWN)) ui->choice ^= 1;
    else if (pressed & K_C) {
        if (ui->screen == UI_TITLE && ui->choice) {
            ui->guide = 1;
            ui->guide_page = 0;
        } else if (ui->screen == UI_REPORT && ui->choice) {
            ui->screen = UI_TITLE;
            ui->choice = 0;
        } else {
            desk_reset(&ui->desk);
            ui->screen = UI_DESK;
        }
    }
}
