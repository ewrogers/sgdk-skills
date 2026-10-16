#include <genesis.h>
#include "dialogue.h"
#include "dialogue_data.h"
#include "lab.h"
#include "panel.h"
#include "resources.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
static DialogueLab lab;
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(dlg_validate(&lab_program));
    CHECK(dialogue_fonts.numTile == 576 && panel_tiles.numTile == 9 && icon_tiles.numTile == 4 && world_tiles.numTile == 16);
    const SpriteDefinition *portraits[]={&courier_portrait,&keeper_portrait};
    u16 tiles=16+576+9+4+16;
    for (u16 p=0; p<2; p++)
    {
        CHECK(portraits[p]->w == 32 && portraits[p]->h == 32 && portraits[p]->animations[0]->numFrame == 5);
        CHECK(portraits[p]->maxNumSprite <= 4);
        for (u16 f=0; f<5; f++)
        {
            const AnimationFrame *frame=portraits[p]->animations[0]->frames[f];
            CHECK(frame->tileset->numTile <= 16 && (frame->numSprite & 0x7F) <= 4);
            tiles+=frame->tileset->numTile;
        }
        for (u16 color=0; color<16; color++) CHECK(portraits[p]->palette->data[color] == courier_portrait.palette->data[color]);
    }
    /* Direct-entry CPU harness skips VDP_init. Supply SGDK 2.11 default
     * tilemap boundary; this checks resource placement, not VDP startup. */
    maps_addr=0xC000;
    CHECK(tiles <= TILE_FONT_INDEX);
    CHECK(panel_fits(1,3,38,8) && panel_fits(1,18,38,8));
    CHECK(panel_piece(0,0,38,8) == 0 && panel_piece(37,7,38,8) == 8 && !panel_fits(65535,0,2,2));
    lab_init(&lab); lab_frame(&lab,LAB_A,60); CHECK(lab.active && lab.dialogue.state == DL_TYPING);
    lab_frame(&lab,LAB_A,60); CHECK(lab.dialogue.state == DL_TYPING);
    lab_frame(&lab,0,60); lab_frame(&lab,LAB_A,60); CHECK(lab.dialogue.state == DL_PAGE_WAIT);
    CHECK(dlg_has_more(&lab.dialogue));
    lab_frame(&lab,0,60); lab_frame(&lab,LAB_A,60); CHECK(lab.dialogue.state == DL_TYPING);
    lab_frame(&lab,0,60); lab_frame(&lab,LAB_A,60); CHECK(lab.dialogue.state == DL_NODE_WAIT);
    lab_frame(&lab,0,60); lab_frame(&lab,LAB_A,60); CHECK(lab.dialogue.panes[0].node && lab.dialogue.panes[1].node);
    lab_frame(&lab,LAB_PAUSE,60); const u16 token=lab.dialogue.token; const u32 time=lab.clock_ms;
    for (u16 i=0; i<60; i++) lab_frame(&lab,LAB_A,60);
    CHECK(lab.dialogue.token == token && lab.clock_ms == time);
    lab_frame(&lab,LAB_PAUSE,60); CHECK(!lab.paused);
    /* Every authored story can be driven to an ending without exceeding bounds. */
    for (u16 story=0; story<LAB_STORY_COUNT; story++)
    {
        CHECK(dlg_start(&lab.dialogue,&lab_program,lab_starts[story],0));
        u16 steps=0;
        while (lab.dialogue.state != DL_DONE && steps++ < 100)
        {
            if (lab.dialogue.state == DL_CHOOSE) dlg_step(&lab.dialogue,0,DL_UP);
            dlg_step(&lab.dialogue,0,DL_ACCEPT);
            CHECK(lab.dialogue.state != DL_ERROR && lab.dialogue.row < DL_ROWS);
        }
        CHECK(lab.dialogue.state == DL_DONE);
        dlg_step(&lab.dialogue,1000,0); CHECK(lab.dialogue.events == 0);
    }
    vu16 *port=(vu16 *)VDP_DATA_PORT;
    *port=tiles; *port=sizeof(Dialogue); *port=sizeof(DialogueLab);
    return 0;
}
