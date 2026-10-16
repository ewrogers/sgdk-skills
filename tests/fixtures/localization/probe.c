#include <genesis.h>
#include "resources.h"
#include "checks.h"

/* Keep the direct-entry ABI intact even though SGDK startup ignores main's result. */
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int error=check_numbers();if(error)return error;
    error=check_layout();if(error)return error;
    error=check_preferences();if(error)return error;
    error=check_controller();if(error)return error;
    /* The SDK returns TRUE, not a tile count. Zero tiles avoids device data writes. */
    const TileSet empty={COMPRESSION_NONE,0,language_font.tiles};
    if(VDP_loadTileSet(&empty,LOC_FONT_BASE,CPU)!=TRUE)return 1002;
    volatile u16 *port=(volatile u16 *)0xC00000;
    if(language_font.numTile!=LOC_FONT_TILES || language_font.compression!=COMPRESSION_NONE ||
       ferry_tiles.numTile!=48 || LOC_FONT_BASE+LOC_FONT_TILES>(u16)TILE_FONT_INDEX)return 1000;
    *port=LOC_GLYPH_COUNT;*port=LOC_FONT_TILES;*port=sizeof(LanguageLab);*port=sizeof(LocLayout);
    for(u8 language=0;language<LOC_LANGUAGE_COUNT;language++) {
        u8 name[9];for(u16 i=0;i<8;i++)name[i]='W'-31;name[8]=0;
        for(u16 id=0;id<LOC_MESSAGE_COUNT;id++) {
            u8 text[LOC_TEXT_MAX];LocLayout layout;
            if(!loc_expand(language,id,name,65535,text,sizeof(text)) || !loc_layout(text,loc_boxes[id],&layout))return 1001;
            u16 width=0;
            for(u16 y=0;y<layout.lines;y++)for(u16 x=0;x<loc_boxes[id].width;x++)
                if(layout.cells[y][x] && width<x+1)width=x+1;
            *port=layout.lines;*port=width;
        }
    }
    for(u16 i=0;i<LOC_FONT_TILES*8;i++) {
        u32 row=language_font.tiles[i];*port=(u16)(row>>16);*port=(u16)row;
    }
    return 0;
}
