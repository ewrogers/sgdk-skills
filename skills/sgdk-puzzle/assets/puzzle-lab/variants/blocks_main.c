#include <genesis.h>
#include "blocks.h"
#include "resources.h"
#include "font.h"
#include "ui.h"
#include "block_tiles.h"
static BlGame game;
static u16 window_map[28][40],field[18][10],base_tile,blank_tile,frame_base,previous;
static bool title=TRUE,paused=FALSE,dirty=TRUE;
static void text(const char *s,u16 x,u16 y)
{for(u16 i=0;x+i<40 && s[i];i++)window_map[y][x+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);}
static void centered(const char *s,u16 y){text(s,(40-strlen(s))/2,y);}
static void frame(u16 x,u16 y,u16 w,u16 h)
{
    for(u16 row=0;row<h;row++)for(u16 col=0;col<w;col++) {
        u16 p=99;
        if(!row)p=!col ? 0 : col==w-1 ? 2 : 1;
        else if(row==h-1)p=!col ? 5 : col==w-1 ? 7 : 6;
        else if(!col)p=3;else if(col==w-1)p=4;
        if(p!=99)window_map[y+row][x+col]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,frame_base+p);
    }
}
static void draw(void)
{
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)window_map[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile);
    if(title) {
        frame(2,2,36,24);centered("FALLING LANTERNS",5);centered("BLOCK LAB",7);
        centered("BUILD COMPLETE ROWS TO CLEAR THEM",10);centered("THE OUTLINE SHOWS WHERE YOU LAND",12);
        centered("LEFT / RIGHT MOVE   DOWN FALL",15);centered("A ROTATE   B DROP",17);centered("START PAUSE",19);
        centered("A / START: BEGIN",23);
    } else {
        centered("FALLING LANTERNS",1);frame(9,4,12,20);
        text("NEXT",24,4);u16 mask=bl_shape(game.next,0);
        for(u16 y=0;y<4;y++)for(u16 x=0;x<4;x++)if(mask&(1u<<(y*4+x)))window_map[6+y][24+x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,base_tile+game.next+1);
        char n[16];text("SCORE",24,11);sprintf(n,"%lu",game.score);text(n,24,12);
        text("ROWS",24,15);sprintf(n,"%lu",game.lines);text(n,24,16);
        if(game.flash)text("ROW CLEAR!",24,19);
        text("A ROTATE   B DROP   START PAUSE",5,25);centered("DOWN: SOFT DROP",26);
        for(u16 y=0;y<18;y++)for(u16 x=0;x<10;x++){window_map[y+5][x+10]=0;field[y][x]=TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,base_tile+game.cells[y*10+x]);}
        if(!game.over) {
            s16 landing=game.y;while(bl_fits(&game,game.x,landing+1,game.rotation))landing++;
            mask=bl_shape(game.piece,game.rotation);
            for(s16 y=0;y<4;y++)for(s16 x=0;x<4;x++)if(mask&(1u<<(y*4+x))) {
                field[landing+y][game.x+x]=TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,base_tile+8);
                field[game.y+y][game.x+x]=TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,base_tile+game.piece+1);
            }
        }
        if(paused || game.over) {
            frame(3,9,34,9);
            for(u16 y=10;y<17;y++)for(u16 x=4;x<36;x++)window_map[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile);
            centered(paused ? "PAUSED" : "THE LANTERNS ARE STACKED",11);
            centered(paused ? "START: RESUME" : "A / START: TRY AGAIN",15);
        }
        for(u16 y=0;y<18;y++)DMA_queueDma(DMA_VRAM,field[y],VDP_getPlaneAddress(BG_B,10,y+5),10,2);
    }
    for(u16 y=0;y<28;y++)DMA_queueDma(DMA_VRAM,window_map[y],VDP_getPlaneAddress(WINDOW,0,y),40,2);
}
static void cue(u16 event)
{
    const u8 *p=event&4 ? snd_blocked : event&2 ? snd_win : snd_push;
    u32 length=event&4 ? sizeof(snd_blocked) : event&2 ? sizeof(snd_win) : sizeof(snd_push);
    XGM2_playPCMEx(p,length,SOUND_PCM_CH3,6,FALSE,FALSE);
}
int main(bool hard_reset)
{
    (void)hard_reset;bl_init(&game,147);SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);VDP_setWindowVPos(FALSE,28);DMA_setMaxTransferSize(4096);
    base_tile=TILE_USER_INDEX;blank_tile=base_tile;frame_base=base_tile+9;
    VDP_loadTileData(block_tiles,base_tile,9,CPU);VDP_loadTileData(frame_tiles,frame_base,8,CPU);VDP_loadFontData(font_tiles,96,CPU);
    PAL_setPalette(PAL3,ui_colors,CPU);VDP_setBackgroundColor(49);SYS_enableInts();SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);
    XGM2_play(garden_music);Z80_setForceDelayDMA(TRUE);VDP_setEnable(TRUE);
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held&~previous;previous=held;
        if(title || game.over) {
            if(pressed&(BUTTON_A|BUTTON_START)){title=FALSE;bl_init(&game,147);game.previous=BL_ROTATE|BL_DROP;dirty=TRUE;}
        } else {
            if(pressed&BUTTON_START){paused=!paused;dirty=TRUE;if(paused){XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH3);}else XGM2_resume();}
            u16 input=(held&BUTTON_LEFT ? BL_LEFT : 0)|(held&BUTTON_RIGHT ? BL_RIGHT : 0)|(held&BUTTON_DOWN ? BL_SOFT : 0)|(held&BUTTON_A ? BL_ROTATE : 0)|(held&BUTTON_B ? BL_DROP : 0);
            if(!paused) {
                s16 old_x=game.x,old_y=game.y;u16 old_rotation=game.rotation,old_piece=game.piece,flash=game.flash;u16 ticks=bl_clock(&game.phase,SYS_isPAL() ? 50 : 60);
                while(ticks--){u16 event=bl_tick(&game,input);if(event){cue(event);dirty=TRUE;}}
                if(old_x!=game.x || old_y!=game.y || old_rotation!=game.rotation || old_piece!=game.piece || (!!flash)!=(!!game.flash))dirty=TRUE;
            } else game.previous=input;
        }
        if(dirty){draw();dirty=FALSE;}SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
