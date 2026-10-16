#include <genesis.h>
#include "iso.h"
#include "resources.h"
#include "ui.h"
#include "font.h"

static IlGame game;
static IlCache cache;
static Sprite *sprites[IL_ITEMS],*foot;
static IlItem items[IL_MAX_ITEMS];static IlLink supports[IL_MAX_LINKS];
static u16 order[IL_MAX_ITEMS],hud_map[6][40],bar_tile,sort_status,previous,tour,frame;
static bool paused,cutaway,info;
static const s16 anchor_x[]={16,8,16,16,16,16,16,12};
static const s16 anchor_y[]={12,35,36,36,55,40,24,29};
static void put(const char *s,u16 x,u16 row)
{
    if(x>=40 || row>=6)return;
    u16 n=(u16)strlen(s);if(n>40-x)n=40-x;
    for(u16 i=0;i<n;i++)hud_map[row][x+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);
}
static void number(u16 n,u16 x,u16 row)
{
    if(n>999)n=999;
    char text[4];text[3]=0;
    for(s16 i=2;i>=0;i--){text[i]=(char)('0'+n%10);n/=10;}
    put(text,x,row);
}
static void hud(void)
{
    if(!DMA_canQueue(DMA_VRAM,240) || DMA_getQueueSize()+6>DMA_getMaxQueueSize())return;
    for(u16 r=0;r<6;r++)for(u16 x=0;x<40;x++)hud_map[r][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,bar_tile);
    put("LANTERN AQUEDUCT",2,1);put(paused ? "PAUSED" : game.layer ? "UPPER WALK" : "GARDEN WALK",27,1);
    put("A TOUR  B CUTAWAY  C INFO  START PAUSE",2,4);
    if(sort_status!=IL_SORT_OK)put("DRAW ORDER NEEDS ATTENTION",2,3);
    else if(info) {
        put("HEIGHT",2,3);number((u16)game.z/16,9,3);
        put("SORT",15,3);put(sort_status==IL_SORT_OK ? "OK" : "CHECK",20,3);
        put(SYS_isPAL() ? "PAL 50" : "NTSC 60",29,3);
    } else put(cutaway ? "THE BRIDGE IS CUT AWAY" : "FOLLOW THE STAIRS, OR WALK BENEATH",2,3);
    for(u16 r=0;r<6;r++)DMA_queueDma(DMA_VRAM,hud_map[r],VDP_getPlaneAddress(BG_A,0,r<3 ? r : r+22),40,2);
}
static Sprite *add(const SpriteDefinition *def,u16 pal)
{
    Sprite *s=SPR_addSprite(def,0,0,TILE_ATTR(pal,FALSE,FALSE,FALSE));
    if(s)SPR_setAutoAnimation(s,FALSE);
    return s;
}
static void render(void)
{
    u16 links=0,n=il_scene(&game,items,supports,&links);sort_status=il_sort_cached(&cache,&items[IL_STATIC],1,supports,links,order);
    if(sort_status!=IL_SORT_INVALID)for(u16 rank=0;rank<n;rank++)SPR_setDepth(sprites[order[rank]],-(s16)rank);
    for(u16 i=0;i<n;i++) {
        bool hero=i==IL_STATIC;u16 kind=hero ? IL_HERO : il_props[i].kind;
        s16 x=hero ? game.x : il_props[i].x,y=hero ? game.y : il_props[i].y,z=hero ? game.z : il_props[i].z;
        IlPoint p=il_project(x,y,z);
        SPR_setPosition(sprites[i],256+p.x-game.camera_x-anchor_x[kind],64+p.y-game.camera_y-anchor_y[kind]);
        bool show=!cutaway || hero || (kind!=IL_DECK && kind!=IL_RAMP_L && kind!=IL_RAMP_R);
        SPR_setVisibility(sprites[i],show ? VISIBLE : HIDDEN);
        if(kind==IL_HERO)SPR_setFrame(sprites[i],game.facing*4+(game.moving ? game.walk/8 : 0));
        if(kind==IL_FOUNTAIN)SPR_setFrame(sprites[i],(game.ticks/16)%3);
    }
    IlPoint p=il_project(game.x,game.y,game.z);
    SPR_setPosition(foot,256+p.x-game.camera_x-8,64+p.y-game.camera_y-4);
    SPR_setDepth(foot,-1000);SPR_setVisibility(foot,info ? VISIBLE : HIDDEN);
    VDP_setHorizontalScroll(BG_B,-game.camera_x);VDP_setVerticalScroll(BG_B,game.camera_y);
}
int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);SPR_initEx(320);DMA_setMaxTransferSize(4096);
    if(ground.tileset->numTile+TILE_USER_INDEX+1>TILE_SPRITE_INDEX) {
        VDP_drawText("BACKGROUND EXCEEDS VRAM BUDGET",1,10);SYS_enableInts();VDP_setEnable(TRUE);
        while(TRUE)SYS_doVBlankProcess();
    }
    VDP_drawImageEx(BG_B,&ground,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,FALSE);
    bar_tile=TILE_USER_INDEX+ground.tileset->numTile;
    u32 solid[8]={0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111};
    VDP_loadTileData(solid,bar_tile,1,CPU);
    VDP_loadFontData(font_tiles,96,CPU);
    PAL_setPalette(PAL0,ground.palette->data,CPU);PAL_setPalette(PAL1,deck.palette->data,CPU);
    PAL_setPalette(PAL2,hero.palette->data,CPU);PAL_setPalette(PAL3,ui_colors,CPU);
    const SpriteDefinition *defs[]={&deck,&post,&ramp_l,&ramp_r,&tower,&tree,&fountain};bool ok=true;
    for(u16 i=0;i<IL_STATIC;i++){sprites[i]=add(defs[il_props[i].kind],PAL1);ok=ok && sprites[i];}
    sprites[IL_STATIC]=add(&hero,PAL2);foot=add(&marker,PAL2);ok=ok && sprites[IL_STATIC] && foot;
    if(!ok) {
        VDP_drawText("SPRITE ALLOCATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);
        while(TRUE)SYS_doVBlankProcess();
    }
    il_init(&game);u16 support_count=0;il_scene(&game,items,supports,&support_count);il_cache_init(&cache,items,IL_STATIC);
    render();SYS_enableInts();
    for(u16 warm=0;warm<4;warm++){SPR_update();if(!warm)hud();SYS_doVBlankProcess();}
    VDP_setEnable(TRUE);XGM2_play(garden_music);
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held & (u16)~previous;previous=held;
        if(pressed&BUTTON_START){paused=!paused;if(paused)XGM2_pause();else XGM2_resume();}
        if(pressed&BUTTON_A){tour=(tour+1)%4;il_tour(&game,tour);}
        if(pressed&BUTTON_B)cutaway=!cutaway;
        if(pressed&BUTTON_C)info=!info;
        u16 input=(held&BUTTON_LEFT ? IL_LEFT : 0)|(held&BUTTON_RIGHT ? IL_RIGHT : 0)|
            (held&BUTTON_UP ? IL_UP : 0)|(held&BUTTON_DOWN ? IL_DOWN : 0);
        il_frame(&game,input,SYS_isPAL() ? 50 : 60,paused);render();if(!(frame++%6) || pressed)hud();
        SPR_update();SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
