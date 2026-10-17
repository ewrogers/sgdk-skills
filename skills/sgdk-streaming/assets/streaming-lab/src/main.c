#include <genesis.h>
#include "resources.h"
#include "palettes.h"
#include "stream_io.h"
#include "stream_world.h"

enum { SHARED_BASE=96, PANEL_TILE=104, WORLD_WIDTH=1280, VIEW_WIDTH=320, ROWS=24 };
static StreamCache cache;
static ScIO io;
static u16 columns[4][ROWS],colors[2][16];
static u16 fade_colors[64],fade_destination;
static SwFade fade;
static u16 palette_owner[2]={0xFFFF,0xFFFF};
static const char *names[]={"MOSS GARDEN","AMBER WORKS","TIDAL QUAY","MOONLIT RUINS"};
static u16 camera,player=160,held_before,ticks,phase,build_column,load_camera;
static bool loading=true,paused,slow,face_left,warp_covered;
static u16 walk_waits;
#ifdef SC_AUTOPLAY
static u16 late;static u32 last_vblank;
#endif
static char old_status[40],old_area[40],old_fps[6];

static u16 clamp_camera(s16 value) { return value<0 ? 0 : value>960 ? 960 : (u16)value; }

static void text(const char *s,u16 x,u16 y)
{
    u16 length=(u16)strlen(s);
    if(io.used+length*2<=io.limit && DMA_getQueueSize()<DMA_getMaxQueueSize()) {
        VDP_drawTextEx(WINDOW,s,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),x,y,DMA_QUEUE);
        io.used+=length*2;
    }
}

static void digits(char *out,u16 value)
{
    if(value>99)value=99;
    char a='0';while(value>=10){a++;value-=10;}out[0]=a;out[1]=(char)('0'+value);
}

static void hud(u16 fps)
{
    u16 ready=0;for(u16 i=0;i<SC_SLOTS;i++)ready+=cache.slots[i].state==SC_READY;
    char line[]="CACHE 00/18  EVICT 00  CHUNK 256B";
    digits(line+6,ready);digits(line+19,(u16)(cache.evictions>99 ? 99 : cache.evictions));
    if(slow){line[29]='0';line[30]='6';line[31]='4';}
    if(strcmp(line,old_status)){text(line,1,2);memcpy(old_status,line,sizeof(line));}
    char area[40]="";const char *name=names[camera/320];u16 i=0;
    while(name[i]){area[i]=name[i];i++;}
    if(paused){const char *p=" / PAUSED";while(*p)area[i++]=*p++;}
    while(i<37)area[i++]=' ';
    area[i]=0;
    if(strcmp(area,old_area)){text(area,1,1);memcpy(old_area,area,sizeof(area));}
    char rate[]="00FPS";digits(rate,fps);
    if(strcmp(rate,old_fps)){text(rate,33,0);memcpy(old_fps,rate,sizeof(rate));}
}

static bool view_ready(u16 x)
{
    u16 ignored;
    for(u16 room=x/320;room<=(x+319)/320;room++)
        for(u16 p=0;p<6;p++)if(!sc_tile(&cache,room*6+p,0,&ignored))return false;
    return true;
}

static void pins_and_request(u16 target)
{
    u16 list[SC_SLOTS];
    u16 count=sw_wanted(clamp_camera((s16)target-128),clamp_camera((s16)target+128),false,list);
    sc_pin(&cache,list,count); /* Current view plus both margins fits at most three rooms. */
    for(u16 i=0;i<count;i++)if(sc_require(&cache,list[i])!=SC_AVAILABLE)break;
}

static bool column(u16 world_column,u16 buffer)
{
    for(u16 y=0;y<ROWS;y++) {
        u16 tile;
        if(!sc_tile(&cache,sw_page(world_column,y),(y&3)*4+(world_column&3),&tile))return false;
        columns[buffer][y]=TILE_ATTR_FULL(PAL1+(world_column/40&1),FALSE,FALSE,FALSE,tile);
    }
    return sc_queue_words(&io,DMA_VRAM,columns[buffer],VDP_getBGBAddress()+(world_column&63)*2,ROWS,128);
}

static bool palettes(u16 x)
{
    for(u16 room=x/320;room<=(x+319)/320;room++) {
        u16 bank=room&1;
        if(palette_owner[bank]==room)continue;
        u16 saved[16];memcpy(saved,colors[bank],sizeof(saved));
        for(u16 i=0;i<16;i++)colors[bank][i]=area_colors[room][i];
        if(fade.state==SW_FADE_IDLE &&
           !sc_queue_words(&io,DMA_CRAM,colors[bank],(PAL1+bank)*32,16,2)) {
            memcpy(colors[bank],saved,sizeof(saved));return false;
        }
        palette_owner[bank]=room;
    }
    return true;
}

static void cover(u16 destination)
{
    loading=true;load_camera=destination;build_column=0;
    VDP_setWindowOnTop(28); /* Resident opaque Window covers unfinished art. */
    VDP_updateSprites(0,DMA_QUEUE);
    io.used+=8; /* Conservative allowance for the SDK's hidden SAT entry. */
}

static void frame(u16 held,u16 hz)
{
    u16 pressed=held & (u16)~held_before;held_before=held;
    if(pressed&BUTTON_START)paused=!paused;
    if(pressed&BUTTON_B)slow=!slow;
    if((pressed&BUTTON_A) && !loading && fade.state==SW_FADE_IDLE) {
        fade_destination=((camera/320+1)%4)*320;
        sw_fade_begin(&fade);warp_covered=false;
    }
    if((pressed&BUTTON_C) && fade.state==SW_FADE_IDLE) {
        if(sc_reset(&cache))cover(loading ? load_camera : camera);
    }
    if(fade.state==SW_FADE_LOAD && !warp_covered) {
        /* The previous frame's black palette has drained before covering/redrawing. */
        player=fade_destination+160;cover(fade_destination);warp_covered=true;
    }
    bool fading=fade.state!=SW_FADE_IDLE;
    SwFade fade_before=fade;
    sw_fade_tick(&fade,hz,!loading);
    s16 next_player=(s16)player;
    phase+=60;u16 steps=0;while(phase>=hz){phase-=hz;steps++;}
    if(!paused && !loading && !fading) {
        s16 direction=(held&BUTTON_RIGHT ? 1 : 0)-(held&BUTTON_LEFT ? 1 : 0);
        if(direction)face_left=direction<0;
        next_player+=direction*2*steps;
        if(next_player<16)next_player=16;
        if(next_player>1264)next_player=1264;
        ticks+=steps;
    }
    u16 next_camera=loading ? load_camera : sw_camera((u16)next_player);
    pins_and_request(next_camera);
    sc_step(&cache,slow ? 2 : SC_CHUNK_TILES,sc_rom_read,sc_dma_tiles,&io);
    if(loading) {
        if(view_ready(load_camera) && view_ready(clamp_camera((s16)load_camera-128)) &&
           view_ready(clamp_camera((s16)load_camera+128))) {
            bool ok=true;
            for(u16 i=0;i<4 && ok && build_column<sw_columns(load_camera);i++) {
                u16 world_column=load_camera/8+build_column;
                if(world_column>=160) { build_column=sw_columns(load_camera);break; }
                ok=column(world_column,i);if(ok)build_column++;
            }
            if(ok)ok=palettes(load_camera);
            if(ok && build_column>=sw_columns(load_camera)) {
                camera=load_camera;loading=false;
                /* Expose only after the queued columns have actually flushed. */
            }
        }
    } else if(view_ready(next_camera)) {
        bool ok=true;
        u16 entering;
        if(ok && sw_entering(camera,next_camera,&entering)) {
            ok=column(entering,0);
        }
        if(ok)ok=palettes(next_camera);
        if(ok) {camera=next_camera;player=(u16)next_player;}
        else cover(camera);
    } else if(!loading && next_camera!=camera && walk_waits<99)walk_waits++;
    VDP_setHorizontalScrollVSync(BG_B,-(s16)camera);
    VDP_setVerticalScrollVSync(BG_B,-32);
    if(!loading) {
        u16 animation=(!paused && (held&(BUTTON_LEFT|BUTTON_RIGHT))) ? ((ticks/8)&1) : 0;
        VDP_setSpriteFull(0,(s16)player-(s16)camera-8,188,SPRITE_SIZE(2,2),
            TILE_ATTR_FULL(PAL3,TRUE,FALSE,face_left,SHARED_BASE+animation*4),0);
        VDP_updateSprites(1,DMA_QUEUE);io.used+=8;
    }
    if(fading) {
        /* Own every palette during the transition; never reveal the loading panel. */
        for(u16 i=0;i<16;i++) {
            u16 ui=i==0 || i==1 ? 0x0202 : i==15 ? 0x0EEE : 0;
            fade_colors[i]=sw_fade_color(ui,fade.level);
            fade_colors[16+i]=sw_fade_color(colors[0][i],fade.level);
            fade_colors[32+i]=sw_fade_color(colors[1][i],fade.level);
            fade_colors[48+i]=sw_fade_color(hero_colors[i],fade.level);
        }
        if(!sc_queue_words(&io,DMA_CRAM,fade_colors,0,64,2))fade=fade_before;
    }
}

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);
    VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);
    u32 panel[8];for(u16 i=0;i<8;i++)panel[i]=0x11111111UL;
    VDP_loadTileData(panel,PANEL_TILE,1,CPU);
    VDP_loadTileData((const u32 *)stream_hero,SHARED_BASE,8,CPU);
    PAL_setColor(0,0x0202);PAL_setColor(1,0x0202);PAL_setColor(15,0x0EEE);
    PAL_setPalette(PAL3,hero_colors,CPU);
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,PANEL_TILE),0,0,40,28);
    VDP_setTextPlane(WINDOW);VDP_setTextPalette(PAL0);
    VDP_drawText("STREAMING LAB",1,0);
#ifdef SC_AUTOPLAY
    slow=true;VDP_drawText("AUTO",26,0);
#endif
    VDP_drawText("A WARP B CHUNK C RELOAD START PAUSE",1,3);
    VDP_drawText("COPPERLINE COURIER",11,10);
    VDP_drawText("FILLING THE TILE CACHE...",8,13);
    VDP_drawText("LEFT / RIGHT TO WALK",10,17);
    VDP_drawText("SHARED HERO AND HUD STAY RESIDENT",4,20);
    VDP_setWindowOnTop(28);VDP_resetSprites();
    sc_init(&cache);DMA_setMaxTransferSize(SC_FRAME_BYTES);
    if(SC_CACHE_BASE+SC_SLOTS*SC_PAGE_TILES>TILE_USER_MAX_INDEX+1) {
        VDP_drawText("VRAM LAYOUT DOES NOT FIT",8,15);
        SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();
    }
    u16 hz=SYS_isPAL() ? 50 : 60,frames=0;
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    while(TRUE) {
        io=(ScIO){0,SC_FRAME_BYTES};
        u16 fps=SYS_getFPS();
#ifdef SC_AUTOPLAY
        static bool right=true;
        if(player>=1240)right=false;
        if(player<=40)right=true;
        u32 now=vtimer;
        if(last_vblank && now-last_vblank>1 && late<99)late++;
        last_vblank=now;
        frame(right ? BUTTON_RIGHT : BUTTON_LEFT,hz);
#else
        frame(JOY_readJoypad(JOY_1),hz);
#endif
        if(!(frames++%6)) {
            hud(fps);
#ifdef SC_AUTOPLAY
            char diagnostic[]="WAIT00 LATE00";digits(diagnostic+4,walk_waits);digits(diagnostic+11,late);
            text(diagnostic,24,1);
#endif
        }
        SYS_doVBlankProcess();
        /* No new producer may overwrite borrowed RAM while anything is queued. */
        while(DMA_getQueueSize())SYS_doVBlankProcess();
        sc_complete(&cache,true);
        VDP_setWindowOnTop(loading ? 28 : 4);
    }
}
