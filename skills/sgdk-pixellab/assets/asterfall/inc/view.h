#ifndef ASTERFALL_VIEW_H
#define ASTERFALL_VIEW_H
#include <genesis.h>
#include "game.h"
void view_init(void);
void view_frame(Game *g);
void ui_draw(const Game *g);
void ui_tick(const Game *g);
void ui_text(const char *text,u16 x,u16 y,bool gold);
void ui_number(u16 number,u16 x,u16 y,bool gold);
void ui_panel(u16 x,u16 y,u16 w,u16 h);
void ui_bar(u16 x,u16 y,u16 w,u16 value,u16 maximum,u16 color);
void ui_image(const Image *image,u16 palette,u16 x,u16 y);
void ui_large(const char *text,u16 x,u16 y);
void ui_fill(u16 x,u16 y,u16 w,u16 h,u16 color);
void actors_reset(void);
void actors_frame(const Game *g);
void sound_frame(const Game *g);
u16 art_load(const Image *image,u16 palette,bool load_palette);
extern u16 image_base,panel_base,solid_base,gold_font_base,title_font_base,actor_base;
#endif
