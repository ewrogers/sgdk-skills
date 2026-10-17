#ifndef DESK_UI_H
#define DESK_UI_H
#include "desk.h"
enum { UI_TITLE, UI_DESK, UI_REPORT };
enum { UI_GUIDE_PAGES = 3 };
typedef struct {
    Desk desk;
    unsigned char screen, choice, guide, guide_page, cue;
} DeskUI;
void ui_init(DeskUI *ui);
void ui_frame(DeskUI *ui, unsigned short pressed, unsigned char fps);
#endif
