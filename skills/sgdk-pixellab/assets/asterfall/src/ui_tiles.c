#include "ui_tiles.h"
void ui_tiles_set(UITiles *tiles,uint16_t x,uint16_t y,uint16_t value)
{
    if(x>=UI_WIDTH||y>=UI_HEIGHT)return;
    uint16_t *cell=&tiles->cells[y*UI_WIDTH+x];
    if(*cell!=value){*cell=value;tiles->dirty[y]=true;}
}
void ui_tiles_clear(UITiles *tiles)
{
    for(uint16_t y=0;y<UI_HEIGHT;y++)for(uint16_t x=0;x<UI_WIDTH;x++)ui_tiles_set(tiles,x,y,0);
}
