#ifndef ASTERFALL_UI_TILES_H
#define ASTERFALL_UI_TILES_H
#include "game.h"
#define UI_WIDTH 40
#define UI_HEIGHT 28
typedef struct {uint16_t cells[UI_WIDTH*UI_HEIGHT];bool dirty[UI_HEIGHT];} UITiles;
void ui_tiles_set(UITiles *tiles,uint16_t x,uint16_t y,uint16_t value);
void ui_tiles_clear(UITiles *tiles);
#endif
