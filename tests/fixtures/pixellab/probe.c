#include <genesis.h>
#include "resources.h"
#include "expected.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(native_tile.numTile == 1);
    for (u16 i=0;i<8;i++) CHECK(native_tile.tiles[i] == expected_tile[i]);
    CHECK(robot.w == 32 && robot.h == 32 && robot.numAnimation == 1);
    CHECK(robot.animations[0]->numFrame == 5 && robot.maxNumSprite <= 4);
    CHECK(key.w == 16 && key.h == 16 && key.animations[0]->numFrame == 1);
    CHECK(chest.w == 32 && chest.h == 16 && chest.animations[0]->numFrame == 1);
    const SpriteDefinition *defs[]={&robot,&key,&chest};
    u16 frame_tiles=0;
    for(u16 d=0;d<3;d++)
    {
        CHECK(defs[d]->palette->length == 16);
        for(u16 c=0;c<16;c++) CHECK(defs[d]->palette->data[c] == expected_palette[c]);
        for(u16 f=0;f<defs[d]->animations[0]->numFrame;f++)
        {
            const AnimationFrame *frame=defs[d]->animations[0]->frames[f];
            CHECK((frame->numSprite & 0x7F) <= 4);
            frame_tiles+=frame->tileset->numTile;
        }
    }
    CHECK(background.tileset->numTile > 0 && background.tileset->numTile <= 960);
    CHECK(title.tileset->numTile > 0 && title.tileset->numTile <= 1120);
    vu16 *port=(vu16 *)VDP_DATA_PORT;
    *port=frame_tiles; *port=background.tileset->numTile; *port=title.tileset->numTile;
    return 0;
}
