#ifndef CARPET_UI_TEST_H
#define CARPET_UI_TEST_H
#include "game.h"
#include <stdio.h>
#include <string.h>
#define TRUE true
#define FALSE false
#define PAL3 3
#define TILE_FONT_INDEX 1400
#define DMA_QUEUE_COPY 1
#define TILE_ATTR_FULL(p,priority,v,h,tile) ((tile)|((priority)?0x8000:0))
typedef enum { BG_A,BG_B,WINDOW } VDPPlane;
extern u16 panel_tile,big_font_tile,hud_tile,health_tile,label_tile,banner_tile,selected_panel_tile;
void VDP_setTileMapDataRowEx(VDPPlane,const u16 *,u16,u16,u16,u16,u16);
void VDP_setTileMapXY(VDPPlane,u16,u16,u16);
void VDP_clearPlane(VDPPlane,bool);
void VDP_fillTileMapRect(VDPPlane,u16,u16,u16,u16,u16);
void sprite_text(const char *,s16,s16);
void ui_changed(const Game *);
void ui_frame(const Game *);
#endif
