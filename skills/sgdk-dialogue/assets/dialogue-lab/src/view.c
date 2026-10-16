#include "view.h"
#include "panel.h"
#include "dialogue_data.h"
#include "resources.h"

static u16 canvas[28][40],previous[28][40];
static u16 font_tile,panel_tile,icon_tile,world_tile,portrait_tile[2][5];
static u16 old_revision=65535;
static u8 old_story=255,old_arrow=255;
static bool drawn,old_active,old_paused;
static const SpriteDefinition *const portraits[]={&courier_portrait,&keeper_portrait};

static void text(const char *value,u16 x,u16 y,u8 style)
{
    if (y >= 28 || style > 5) return;
    while (*value && x < 40)
    {
        u8 glyph=(u8)*value++; if (glyph < 32 || glyph > 126) glyph='?';
        canvas[y][x++]=TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,font_tile+style*96+glyph-32);
    }
}

static void panel(u16 x,u16 y,u16 width,u16 height)
{
    if (!panel_fits(x,y,width,height)) return;
    for (u16 row=0; row<height; row++)
        for (u16 column=0; column<width; column++)
            canvas[y+row][x+column]=TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,panel_tile+panel_piece(column,row,width,height));
}

static void pane(const DlgPane *value,u16 index)
{
    const DlgNode *node=value->node;
    if (!node) return;
    const u16 y=index ? 18 : 3;
    const u16 x=node->portrait == DL_NO_PORTRAIT ? 3 : 8;
    panel(1,y,38,8);
    if (node->flags & DL_NAMEPLATE)
    {
        panel(8,y-1,strlen(node->speaker)+4,3);
        text(node->speaker,10,y,1);
    }
    for (u16 row=0; row<DL_ROWS; row++)
        for (u16 column=0; column<dlg_columns(node); column++)
        {
            const DlgCell cell=value->cells[row*DL_COLUMNS+column];
            if (cell.glyph) canvas[y+2+row][x+column]=TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,font_tile+cell.style*96+cell.glyph-32);
        }
}

static void compose(const DialogueLab *lab,u8 arrow)
{
    memset(canvas,0,sizeof canvas);
    text("DIALOGUE LAB",2,0,1);
    text(lab->paused ? "PAUSED" : "START: PAUSE",26,0,lab->paused ? 1 : 0);
    if (!lab->active)
    {
        panel(4,6,32,16);
        text("LETTERS FROM LANTERN QUAY",8,8,1);
        text("CHOOSE A CONVERSATION",9,10,0);
        for (u16 i=0; i<LAB_STORY_COUNT; i++)
        {
            text(i == lab->story ? ">" : " ",7,12+i*3,1);
            text(lab_titles[i],9,12+i*3,i == lab->story ? 1 : 0);
        }
        text("PORTRAITS / STYLES / BRANCHES",6,23,2);
        text("D-PAD CHOOSE   A OPEN",10,25,0);
        return;
    }
    const Dialogue *dialogue=&lab->dialogue;
    pane(&dialogue->panes[0],0); pane(&dialogue->panes[1],1);
    text("A NEXT  B STORIES  C REPLAY",6,27,0);
    if (dialogue->state == DL_ERROR)
    {
        panel(5,12,30,5); text("DIALOGUE DATA ERROR",11,14,1); return;
    }
    if (dialogue->state == DL_DONE)
    {
        panel(7,12,26,5); text("A: BACK TO STORIES",11,14,1); return;
    }
    if (dialogue->state == DL_CHOOSE)
    {
        const DlgNode *node=&dialogue->program->nodes[dialogue->node];
        panel(7,12,32,5);
        for (u16 i=0; i<dialogue->visible_count; i++)
        {
            text(i == dialogue->selected ? ">" : " ",8,13+i,1);
            text(node->choices[dialogue->visible[i]].label,10,13+i,i == dialogue->selected ? 1 : 0);
        }
    }
    else if (dialogue->state == DL_PAGE_WAIT || dialogue->state == DL_NODE_WAIT)
    {
        const u16 row=dialogue->program->nodes[dialogue->node].pane ? 24 : 9;
        canvas[row][36]=TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,icon_tile+(dlg_has_more(dialogue) ? arrow : 2));
    }
}

static void draw_portraits(const DialogueLab *lab)
{
    u16 count=0;
    if (lab->active)
    {
        const Dialogue *dialogue=&lab->dialogue;
        for (u16 pane_id=0; pane_id<2; pane_id++)
        {
            const DlgNode *node=dialogue->panes[pane_id].node;
            if (!node || node->portrait == DL_NO_PORTRAIT) continue;
            const bool talking=dialogue->state == DL_TYPING && node == &dialogue->program->nodes[dialogue->node];
            const u16 frame_id=talking ? (lab->clock_ms/110)%5 : (lab->clock_ms%3000 < 120 ? 3 : 0);
            const AnimationFrame *frame=portraits[node->portrait]->animations[0]->frames[frame_id];
            u16 tile=portrait_tile[node->portrait][frame_id];
            for (u16 i=0; i<(frame->numSprite & 0x7F); i++)
            {
                const FrameVDPSprite *piece=&frame->frameVDPSprites[i];
                VDP_setSpriteFull(count,24+piece->offsetX,(pane_id ? 160 : 40)+piece->offsetY,piece->size,
                    TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,tile),count+1);
                tile+=piece->numTile; count++;
            }
        }
    }
    if (!count) { VDP_setSpriteFull(0,-64,-64,SPRITE_SIZE(1,1),0,0); count=1; }
    VDP_setSpriteLink(count-1,0);
    /* The persistent SDK cache is consumed at the next VBlank before mutation. */
    VDP_updateSprites(count,DMA_QUEUE);
}

bool view_init(void)
{
    if (dialogue_fonts.numTile != 576 || panel_tiles.numTile != 9 || icon_tiles.numTile != 4 || world_tiles.numTile != 16) return false;
    u16 next=TILE_USER_INDEX;
    font_tile=next; next+=dialogue_fonts.numTile;
    panel_tile=next; next+=panel_tiles.numTile;
    icon_tile=next; next+=icon_tiles.numTile;
    world_tile=next; next+=world_tiles.numTile;
    for (u16 p=0; p<2; p++)
    {
        const SpriteDefinition *definition=portraits[p];
        if (definition->w != 32 || definition->h != 32 || definition->maxNumSprite > 4 || definition->numAnimation != 1 || definition->animations[0]->numFrame != 5) return false;
        for (u16 f=0; f<5; f++)
        {
            portrait_tile[p][f]=next;
            const AnimationFrame *frame=definition->animations[0]->frames[f];
            if (frame->tileset->numTile > 16 || (frame->numSprite & 0x7F) > 4) return false;
            next+=frame->tileset->numTile;
        }
    }
    if (next > TILE_FONT_INDEX) return false;
    SYS_disableInts();
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setWindowHPos(FALSE,0); VDP_setWindowVPos(FALSE,0);
    VDP_clearPlane(BG_A,TRUE); VDP_clearPlane(BG_B,TRUE); VDP_clearPlane(WINDOW,TRUE);
    PAL_setPalette(PAL0,world_palette.data,DMA); PAL_setPalette(PAL1,ui_palette.data,DMA);
    PAL_setPalette(PAL2,courier_portrait.palette->data,DMA);
    VDP_loadTileSet(&dialogue_fonts,font_tile,DMA); VDP_loadTileSet(&panel_tiles,panel_tile,DMA);
    VDP_loadTileSet(&icon_tiles,icon_tile,DMA); VDP_loadTileSet(&world_tiles,world_tile,DMA);
    for (u16 p=0; p<2; p++) for (u16 f=0; f<5; f++)
        VDP_loadTileSet(portraits[p]->animations[0]->frames[f]->tileset,portrait_tile[p][f],DMA);
    for (u16 y=0; y<28; y++) for (u16 x=0; x<40; x++)
    {
        u16 tile=y < 18 ? ((x*17+y*11)%59 == 0 ? 1 : 0) : y < 23 ? 4 : (x+y)%3 == 0 ? 6 : 5;
        if (x >= 30 && x <= 34 && y >= 5 && y < 18) tile=y == 5 ? 9 : x == 32 && y%4 == 0 ? 8 : 3;
        if (y == 17 && (x == 6 || x == 17 || x == 27)) tile=7;
        VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,world_tile+tile),x,y);
    }
    VDP_clearSprites();
    memset(previous,255,sizeof previous); drawn=false;
    VDP_setEnable(TRUE); SYS_enableInts();
    return true;
}

static bool row_changed(u16 row)
{
    for (u16 x=0; x<40; x++) if (previous[row][x] != canvas[row][x]) return true;
    return false;
}

void view_frame(const DialogueLab *lab)
{
    const u8 arrow=(lab->clock_ms/250)%2;
    if (!drawn || old_active != lab->active || old_story != lab->story || old_paused != lab->paused ||
        old_revision != lab->dialogue.revision || old_arrow != arrow)
    {
        compose(lab,arrow);
        for (u16 row=0; row<28; row++) if (row_changed(row))
        {
            VDP_setTileMapDataRowEx(BG_A,canvas[row],0,row,0,40,DMA_QUEUE_COPY);
            memcpy(previous[row],canvas[row],sizeof canvas[row]);
        }
        drawn=true; old_active=lab->active; old_story=lab->story; old_paused=lab->paused;
        old_revision=lab->dialogue.revision; old_arrow=arrow;
    }
    draw_portraits(lab);
}
