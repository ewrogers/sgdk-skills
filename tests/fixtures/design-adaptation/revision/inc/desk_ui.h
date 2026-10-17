#ifndef DESK_UI_H
#define DESK_UI_H
#include "desk.h"
enum { UI_TITLE, UI_DESK, UI_REPORT };
enum { UI_GUIDE_PAGES = 3 };
typedef struct {
    Desk desk;
    unsigned char screen, choice, guide, guide_page, cue;
} DeskUI;
typedef struct {
    unsigned char required, blocked, occupied, wait_seconds;
} UIRoute;
/* Explain the same reservations used by the dispatch model, without mutating it. */
UIRoute ui_route(const Desk *desk, unsigned char bay);
void ui_init(DeskUI *ui);
void ui_frame(DeskUI *ui, unsigned short pressed, unsigned char fps);
#endif
