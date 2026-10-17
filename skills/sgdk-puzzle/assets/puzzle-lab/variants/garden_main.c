#include <genesis.h>
#include "puzzle.h"
#include "resources.h"
#include "font.h"
#include "ui.h"
static PzBoard board;
static PzState from;
static Sprite *player,*boxes[PZ_CRATES];
static u16 window_map[28][40],board_map[18][24],atlas_base,blank_tile,frame_base,previous,phase,repeat,held_direction,motion,facing,clock_tick;
static u16 mode; /* 0 title, 1 playing, 2 room clear, 3 finished */
static bool paused,dirty;
static const char *const names[]={"THE LANTERN SEALS","THE GARDEN GATE","A BRIDGE OF LIGHT"};
static void label(const char *s,u16 y)
{
    u16 x=(40-strlen(s))/2;
    for(u16 i=0;x+i<40 && s[i];i++)window_map[y][x+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);
}
static void panel(u16 y,u16 height)
{for(u16 row=y;row<y+height;row++)for(u16 x=1;x<39;x++)window_map[row][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile);}
static void ui(void)
{
    VDP_setWindowVPos(FALSE,28);
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)window_map[y][x]=0;
    panel(0,5);panel(25,3);
    if(mode==0) {
        for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)window_map[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile);
        for(u16 y=2;y<=25;y++)for(u16 x=2;x<=37;x++) {
            u16 piece=99;
            if(y==2)piece=x==2 ? 0 : x==37 ? 2 : 1;
            else if(y==25)piece=x==2 ? 5 : x==37 ? 7 : 6;
            else if(x==2)piece=3;else if(x==37)piece=4;
            if(piece!=99)window_map[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,frame_base+piece);
        }
        label("CLOCKWORK GARDEN",5);label("PUZZLE LAB",7);
        label("PUSH LANTERNS ONTO THE SEALS",10);label("THEN REACH THE LIT EXIT",12);
        label("D-PAD: MOVE",15);label("A: UNDO     B: RESTART",17);label("START: PAUSE",19);
        label("A / START: BEGIN",23);
    } else {
        label(names[board.room],1);char text[40];sprintf(text,"ROOM %u OF 3     MOVES %u",board.room+1,board.state.moves);label(text,3);
        label("A UNDO   B RESTART   START PAUSE",26);
        if(paused || mode>=2) {
            panel(9,9);label(paused ? "PAUSED" : mode==2 ? "THE GARDEN GLOWS" : "ALL LANTERNS RESTORED",11);
            label(paused ? "START: RESUME" : mode==2 ? "A / START: NEXT ROOM" : "A / START: TITLE",15);
        }
    }
    for(u16 y=0;y<28;y++)DMA_queueDma(DMA_VRAM,window_map[y],VDP_getPlaneAddress(WINDOW,0,y),40,2);
}
static void sound(u16 kind)
{
    const u8 *data=NULL;u32 size=0;
#define CUE(n) do{data=snd_##n;size=sizeof(snd_##n);}while(0)
    switch(kind) {
        case PZ_BLOCKED:CUE(blocked);break;case PZ_WALK:CUE(step);break;
        case PZ_PUSH:CUE(push);break;case PZ_BRIDGE:CUE(bridge);break;
        case PZ_FINISH:CUE(win);break;case 6:CUE(undo);break;
    }
#undef CUE
    if(data)XGM2_playPCMEx(data,size,SOUND_PCM_CH3,6,FALSE,FALSE);
}
static void draw_board(void)
{
    bool gates=pz_gate_open(&board,&board.state),solved=pz_solved(&board,&board.state);
    for(u16 cell=0;cell<PZ_CELLS;cell++) {
        u16 t=board.terrain[cell];
        if(t==PZ_GATE)t=gates ? 9 : 4;
        else if(t==PZ_EXIT)t=solved ? 7 : 6;
        else if(t==PZ_PIT)t=(board.state.filled&(1u<<board.pit[cell])) ? 8 : 5;
        else if(t==PZ_GOAL || t==PZ_PLATE) {
            bool occupied=board.state.player==cell;
            for(u16 c=0;c<PZ_CRATES;c++)if(board.state.crate[c]==cell)occupied=TRUE;
            if(occupied)t=t==PZ_GOAL ? 10 : 11;
        }
        for(u16 y=0;y<2;y++)for(u16 x=0;x<2;x++)
            board_map[cell/PZ_W*2+y][cell%PZ_W*2+x]=atlas.tilemap->tilemap[y*atlas.tilemap->w+t*2+x]+atlas_base;
    }
    for(u16 y=0;y<18;y++)DMA_queueDma(DMA_VRAM,board_map[y],VDP_getPlaneAddress(BG_B,8,y+6),24,2);
}
static void place(Sprite *s,u16 cell,u16 old,s16 height,bool show)
{
    if(!show || cell==PZ_NONE){SPR_setVisibility(s,HIDDEN);return;}
    s16 x=64+(cell%PZ_W)*16,y=48+(cell/PZ_W)*16-height;
    if(motion && old!=PZ_NONE) {
        x+=((s16)(old%PZ_W)-(s16)(cell%PZ_W))*16*(s16)motion/6;
        y+=((s16)(old/PZ_W)-(s16)(cell/PZ_W))*16*(s16)motion/6;
    }
    SPR_setPosition(s,x,y);SPR_setVisibility(s,VISIBLE);SPR_setDepth(s,-y);
}
static void render(void)
{
    bool show=mode==1;
    SPR_setFrame(player,facing*2+(motion ? (clock_tick/3)&1 : 0));
    place(player,board.state.player,from.player,8,show);
    for(u16 i=0;i<PZ_CRATES;i++) {
        SPR_setFrame(boxes[i],(clock_tick/30)&1);
        place(boxes[i],board.state.crate[i],from.crate[i],0,show);
    }
}
static void start_room(u16 room)
{
    pz_load(&board,room);from=board.state;motion=0;repeat=0;held_direction=PZ_NONE;mode=1;dirty=TRUE;
}
static void tick(u16 held,u16 pressed)
{
    if(mode==0 || mode>=2) {
        if(pressed&(BUTTON_A|BUTTON_START)) {
            if(mode==3){mode=0;dirty=TRUE;}
            else start_room(mode==0 ? 0 : board.room+1);
        }
        return;
    }
    if(pressed&BUTTON_START) {
        paused=!paused;dirty=TRUE;
        if(paused){XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH3);}else XGM2_resume();
    }
    if(paused)return;
    clock_tick++;
    if(pressed&BUTTON_B){pz_restart(&board);from=board.state;motion=0;dirty=TRUE;sound(6);return;}
    if(pressed&BUTTON_A){if(pz_undo(&board)){from=board.state;motion=0;dirty=TRUE;sound(6);}return;}
    if(motion){motion--;if(!motion && pz_complete(&board)){mode=board.room==2 ? 3 : 2;dirty=TRUE;}return;}
    u16 direction=held&BUTTON_LEFT ? PZ_LEFT : held&BUTTON_RIGHT ? PZ_RIGHT : held&BUTTON_UP ? PZ_UP : held&BUTTON_DOWN ? PZ_DOWN : PZ_NONE;
    if(direction==PZ_NONE){repeat=0;held_direction=PZ_NONE;return;}
    bool initial=direction!=held_direction;
    if(!initial && repeat){repeat--;return;}
    held_direction=direction;repeat=initial ? 15 : 7;facing=direction;
    from=board.state;u16 result=pz_move(&board,direction);sound(result);
    if(result!=PZ_BLOCKED){motion=6;dirty=TRUE;}
}
int main(bool hard_reset)
{
    (void)hard_reset;pz_load(&board,0);from=board.state;held_direction=PZ_NONE;mode=0;dirty=TRUE;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);SPR_initEx(48);DMA_setMaxTransferSize(4096);
    atlas_base=TILE_USER_INDEX+garden.tileset->numTile;blank_tile=atlas_base+atlas.tileset->numTile;frame_base=blank_tile+1;
    if(frame_base+8>TILE_SPRITE_INDEX) {
        VDP_drawText("ART EXCEEDS VRAM RESERVATION",2,10);SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();
    }
    VDP_drawImageEx(BG_B,&garden,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,FALSE);
    VDP_loadTileSet(atlas.tileset,atlas_base,CPU);VDP_loadFontData(font_tiles,96,CPU);
    u32 blank[8];for(u16 i=0;i<8;i++)blank[i]=0x11111111;VDP_loadTileData(blank,blank_tile,1,CPU);VDP_loadTileData(frame_tiles,frame_base,8,CPU);
    PAL_setPalette(PAL0,garden.palette->data,CPU);PAL_setPalette(PAL1,hero.palette->data,CPU);PAL_setPalette(PAL3,ui_colors,CPU);VDP_setBackgroundColor(1);
    player=SPR_addSprite(&hero,0,0,TILE_ATTR(PAL1,FALSE,FALSE,FALSE));bool ok=player!=NULL;
    for(u16 i=0;i<PZ_CRATES;i++){boxes[i]=SPR_addSprite(&crate,0,0,TILE_ATTR(PAL0,FALSE,FALSE,FALSE));ok=ok && boxes[i]!=NULL;}
    if(!ok){VDP_drawText("SPRITE RESERVATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();}
    SPR_setAutoAnimation(player,FALSE);for(u16 i=0;i<PZ_CRATES;i++)SPR_setAutoAnimation(boxes[i],FALSE);
    render();SYS_enableInts();SPR_update();SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);
    XGM2_play(garden_music);Z80_setForceDelayDMA(TRUE);VDP_setEnable(TRUE);
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held&~previous;previous=held;
        phase+=60;u16 hz=SYS_isPAL() ? 50 : 60;
        while(phase>=hz){phase-=hz;tick(held,pressed);pressed=0;}
        if(dirty){draw_board();ui();dirty=FALSE;}render();
        SPR_update();SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
