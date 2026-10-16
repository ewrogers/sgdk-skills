#include "view.h"
#include "resources.h"
/* check_vram.py reads these SGDK resource layouts from the linked 68000 ROM. */
_Static_assert(__builtin_offsetof(SpriteDefinition,animations)==10,"SpriteDefinition layout changed");
_Static_assert(__builtin_offsetof(Animation,frames)==2,"Animation layout changed");
_Static_assert(__builtin_offsetof(AnimationFrame,tileset)==2,"AnimationFrame layout changed");
_Static_assert(__builtin_offsetof(Image,tileset)==4,"Image layout changed");
_Static_assert(__builtin_offsetof(TileSet,numTile)==2,"TileSet layout changed");
Visual visuals[ART_COUNT];
u16 panel_tile,big_font_tile,hud_tile,health_tile,label_tile,banner_tile,selected_panel_tile;
static u16 boss_tile;
static s16 current_scene=-1;
static const char *error="UNKNOWN SETUP ERROR";
const char *assets_error(void) { return error; }
#define FAIL(message) do { error=message; return false; } while (0)

static bool cache(Art art,const SpriteDefinition *definition,u16 palette,u16 *tile)
{
    if (definition->numAnimation!=1 || definition->animations[0]->numFrame>16 || definition->maxNumSprite>4) FAIL("UNSUPPORTED SPRITE GEOMETRY");
    u16 count; u16 **frames=SPR_loadAllFrames(definition,*tile,&count);
    if (!frames) FAIL("FRAME CACHE ALLOCATION");
    visuals[art].definition=definition; visuals[art].palette=palette;
    for (u16 i=0;i<definition->animations[0]->numFrame;i++) visuals[art].tiles[i]=frames[0][i];
    MEM_free(frames); *tile+=count;
    if (*tile>TILE_FONT_INDEX) FAIL("SPRITE VRAM RESERVATION");
    return true;
}
/* Count the same distinct TileSets that SPR_loadAllFrames caches. */
static u16 frame_tiles(const SpriteDefinition *definition)
{
    const Animation *animation=definition->animations[0]; u16 count=0;
    for (u16 i=0;i<animation->numFrame;i++)
    {
        bool unique=true;
        for (u16 j=0;j<i;j++) if (animation->frames[i]->tileset==animation->frames[j]->tileset) unique=false;
        if (unique) count+=animation->frames[i]->tileset->numTile;
    }
    return count;
}
static bool check_layouts(void)
{
    const Image *far[]={&far_1,&far_2,&far_3},*near[]={&near_1,&near_2,&near_3};
    const SpriteDefinition *boss[]={&scarab_image,&serpent_image,&lantern_image};
    u16 start=selected_panel_tile+1;
    if ((u32)boss_tile+frame_tiles(&button_image)>(u32)TILE_FONT_INDEX) FAIL("MENU ACTOR VRAM BUDGET");
    for (u16 i=0;i<3;i++)
    {
        if ((u32)start+far[i]->tileset->numTile+near[i]->tileset->numTile>(u32)TILE_SPRITE_INDEX)
            FAIL("STAGE SCENERY VRAM BUDGET");
        if ((u32)boss_tile+frame_tiles(boss[i])+BANNER_LINE_TILES*2>(u32)TILE_FONT_INDEX)
            FAIL("STAGE ACTOR VRAM BUDGET");
    }
    if ((u32)start+title_image.tileset->numTile+title_font.numTile>(u32)TILE_SPRITE_INDEX ||
        (u32)start+far_3.tileset->numTile+title_font.numTile>(u32)TILE_SPRITE_INDEX)
        FAIL("MENU VRAM BUDGET");
    return true;
}
bool assets_start(void)
{
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    if (VDP_WINDOW!=0xD000 || VDP_BG_A!=0xE000 || VDP_BG_B!=0xC000) FAIL("H40 PLANE TABLE LAYOUT");
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    SPR_initEx(FRAME_CACHE_TILES); VDP_loadFont(&gradient_font,DMA);
    PAL_setPalette(PAL2,hero_image.palette->data,CPU);
    PAL_setPalette(PAL3,sun_image.palette->data,CPU);
    VDP_setBackgroundColor(PAL3*16+1);
    panel_tile=TILE_USER_INDEX;
    const u32 panel[8]={0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111};
    VDP_loadTileData(panel,panel_tile,1,CPU);
    hud_tile=panel_tile+1; VDP_loadTileSet(&hud_tiles,hud_tile,DMA);
    health_tile=hud_tile+hud_tiles.numTile; VDP_loadTileSet(&health_tiles,health_tile,DMA);
    label_tile=health_tile+health_tiles.numTile; VDP_loadTileSet(&label_font,label_tile,DMA);
    selected_panel_tile=label_tile+label_font.numTile;
    const u32 selected_panel[8]={0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222};
    VDP_loadTileData(selected_panel,selected_panel_tile,1,CPU);
    u16 tile=TILE_FONT_INDEX-FRAME_CACHE_TILES;
    if (!cache(ART_HERO,&hero_image,PAL2,&tile) || !cache(ART_TEAPOT,&teapot_image,PAL2,&tile) ||
        !cache(ART_KITE,&kite_image,PAL2,&tile) || !cache(ART_BEETLE,&beetle_image,PAL2,&tile) ||
        !cache(ART_SUN,&sun_image,PAL3,&tile) || !cache(ART_MOON,&moon_image,PAL3,&tile) ||
        !cache(ART_STORM,&storm_image,PAL3,&tile) || !cache(ART_RIBBON,&ribbon_image,PAL3,&tile) || !cache(ART_EFFECT,&effects_image,PAL3,&tile)) return false;
    boss_tile=tile;
    return check_layouts();
}
bool assets_scene(const Game *game)
{
    bool world=game->state==STAGECARD || game->state==PLAY || game->state==BOSS || game->state==CLEAR;
    s16 scene=world?game->stage+1:game->state==CREDITS?1:0;
    if (scene==current_scene) return true;
    /* Finish queued writes before reusing their tile destinations. Loading is
     * a scene boundary, with the display blanked rather than active CRAM DMA. */
    SYS_doVBlankProcess(); VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A,TRUE); VDP_clearPlane(BG_B,TRUE); VDP_clearPlane(WINDOW,TRUE);
    VDP_setHorizontalScroll(BG_A,0); VDP_setHorizontalScroll(BG_B,0);
    VDP_setVerticalScroll(BG_A,0); VDP_setVerticalScroll(BG_B,0);
    VDP_setTextPalette(PAL3); VDP_setTextPriority(TRUE);
    sprites_invalidate_text();
    u16 tile=selected_panel_tile+1;
    if (world)
    {
        const Image *far=game->stage==1?&far_1:game->stage==2?&far_2:&far_3;
        const Image *near=game->stage==1?&near_1:game->stage==2?&near_2:&near_3;
        if ((u32)tile+far->tileset->numTile+near->tileset->numTile>(u32)TILE_SPRITE_INDEX) FAIL("SCENERY VRAM BUDGET");
        for (u16 i=0;i<2;i++) if (!VDP_drawImageEx(BG_B,far,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,tile),i*32,2,FALSE,TRUE)) FAIL("FAR SCENERY UPLOAD");
        tile+=far->tileset->numTile;
        for (u16 i=0;i<2;i++) if (!VDP_drawImageEx(BG_A,near,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,tile),i*32,14,FALSE,TRUE)) FAIL("NEAR SCENERY UPLOAD");
        PAL_setPalette(PAL0,far->palette->data,CPU); PAL_setPalette(PAL1,near->palette->data,CPU);
        u16 frames=boss_tile;
        if (!cache(ART_BOSS,game->stage==1?&scarab_image:game->stage==2?&serpent_image:&lantern_image,PAL2,&frames)) return false;
        banner_tile=frames;
        if ((u32)banner_tile+BANNER_LINE_TILES*2>(u32)TILE_FONT_INDEX) FAIL("BANNER VRAM RESERVATION");
        VDP_setScrollingMode(HSCROLL_TILE,VSCROLL_PLANE);
        VDP_setWindowOnBottom(2);
        VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,panel_tile),0,0,40,2);
        VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,panel_tile),0,26,40,2);
        VDP_setTextPlane(WINDOW);
    }
    else
    {
        u16 menu_frames=boss_tile;
        if (!cache(ART_BUTTON,&button_image,PAL3,&menu_frames)) return false;
        VDP_setWindowOff(); VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
        const Image *back=scene==1?&far_3:&title_image;
        if ((u32)tile+back->tileset->numTile+title_font.numTile>(u32)TILE_SPRITE_INDEX) FAIL("TITLE VRAM BUDGET");
        if (scene==1)
            for (u16 i=0;i<2;i++) VDP_drawImageEx(BG_B,back,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,tile),i*32,4,FALSE,TRUE);
        else VDP_drawImageEx(BG_B,back,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,tile),4,6,FALSE,TRUE);
        PAL_setPalette(PAL0,back->palette->data,CPU);
        big_font_tile=tile+back->tileset->numTile;
        VDP_loadTileSet(&title_font,big_font_tile,DMA);
        VDP_setTextPlane(BG_A);
    }
    current_scene=scene;
    return true;
}
