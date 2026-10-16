#ifndef ADVENTURE_VIEW_H
#define ADVENTURE_VIEW_H
#include <genesis.h>
#include "game.h"
void view_init(void);
void view_frame(Game *g);
void ui_draw(const Game *g);
void ui_text(const char *text, u16 x, u16 y, bool gold);
void ui_number(u16 number, u16 x, u16 y, bool gold);
void ui_panel(u16 x, u16 y, u16 w, u16 h);
void ui_fill(u16 x, u16 y, u16 w, u16 h, u16 color);
void ui_bar(u16 x, u16 y, u16 w, u16 value, u16 maximum, u16 color);
void sound_frame(const Game *g);
bool save_load(Campaign *campaign);
bool save_write(const Campaign *campaign);
#endif
