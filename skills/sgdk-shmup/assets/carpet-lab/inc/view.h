#ifndef CARPET_VIEW_H
#define CARPET_VIEW_H
#include <genesis.h>
#include "game.h"
#define FRAME_CACHE_TILES 560
#define BANNER_LINE_TILES 24

typedef enum { ART_HERO,ART_BOSS,ART_TEAPOT,ART_KITE,ART_BEETLE,ART_SUN,ART_MOON,ART_STORM,ART_EFFECT,ART_BUTTON,ART_RIBBON,ART_COUNT } Art;
typedef struct { const SpriteDefinition *definition; u16 tiles[16],palette; } Visual;
extern Visual visuals[ART_COUNT];
extern u16 panel_tile,big_font_tile,hud_tile,health_tile,label_tile,banner_tile,selected_panel_tile;
bool assets_start(void);
bool assets_scene(const Game *game);
const char *assets_error(void);
void sprites_begin(void);
void sprites_invalidate_text(void);
void sprite_draw(Art art,s16 x,s16 y,u16 frame,bool flip);
void sprite_text(const char *text,s16 x,s16 y);
void sprite_shield(s16 x,s16 y,u16 charges);
void sprites_end(void);
void ui_changed(const Game *game);
void ui_frame(const Game *game);
bool view_start(void);
bool view_frame(const Game *game);
void view_error_screen(void);
void sound_start(void);
void sound_frame(const Game *game);
#endif
