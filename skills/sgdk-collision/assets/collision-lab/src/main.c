#include <genesis.h>
#include "demo.h"
#include "resources.h"
#include "ui.h"

static CcDemo demo;
static Sprite *body,*shots[2],*walls[2],*dots[16],*markers[2];
static u16 screen[28][40],previous,frame,tone;
static bool paused;
static void put(const char *text,u16 x,u16 y)
{
    if(x>=40 || y>=28)return;
    u16 n=(u16)strlen(text);if(n>40-x)n=40-x;
    for(u16 i=0;i<n;i++)screen[y][x+i]=TILE_FONT_INDEX+(u8)text[i]-32;
}
static void number(u16 n,u16 x,u16 y)
{
    if(n>9999)n=9999;
    char text[5];text[4]=0;
    for(s16 i=3;i>=0;i--){text[i]=(char)('0'+n%10);n/=10;}
    put(text,x,y);
}
static void hud(u16 hz)
{
    if(DMA_getQueueSize()+28>DMA_getMaxQueueSize() || !DMA_canQueue(DMA_VRAM,1120))return;
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)screen[y][x]=TILE_FONT_INDEX;
    put("COLLISION LAB",1,1);put(hz==50 ? "PAL / 50 HZ" : "NTSC / 60 HZ",26,1);
    const char *pages[]={"1 FAST SHOTS","2 AIMING RAYS","3 WALL SLIDING"};put(pages[demo.page],1,3);
    put(paused ? "PAUSED" : "RUNNING",30,3);
    if(!demo.page) {
        put(demo.fast ? "40 PX/TICK" : "4 PX/TICK",1,5);put(demo.moving ? "MOVING WALL" : "STATIC WALL",24,5);
        put("ENDPOINT ONLY",2,9);put("MISSED",23,9);number(demo.misses,31,9);
        put("SWEPT BOX",2,16);put("HITS",23,16);number(demo.hits,31,16);
        put("A FIRE   B SPEED   UP MOVING WALL",1,25);
    } else {
        for(u16 i=0;i<8;i++) {
            CcBox w=cd_walls[i];
            for(u16 y=w.y/128;y<(w.y+w.h)/128;y++)
                for(u16 x=w.x/128;x<(w.x+w.w)/128;x++)screen[y][x]=96;
        }
        if(demo.page==1) {
            put(demo.ray.kind==CC_HIT ? "NEAREST HIT" : "NO HIT",1,5);
            put("TIME /4096",20,5);number(demo.ray.time,32,5);
            put("D-PAD AIM   A RESET",1,25);
        } else {
            put("CANDIDATES",1,5);number(demo.move.stats.visited,12,5);
            put("NARROW",22,5);number(demo.move.stats.narrow,30,5);
            put("D-PAD MOVE / SLIDE   A RESET",1,25);
        }
    }
    put("C NEXT VIEW   START PAUSE",1,27);
    for(u16 y=0;y<28;y++)DMA_queueDma(DMA_VRAM,screen[y],VDP_getPlaneAddress(BG_A,0,y),40,2);
}
static Sprite *add(const SpriteDefinition *def)
{
    Sprite *s=SPR_addSprite(def,0,0,TILE_ATTR(PAL1,TRUE,FALSE,FALSE));
    if(s)SPR_setAutoAnimation(s,FALSE);
    return s;
}
static void place(Sprite *s,s16 x,s16 y,bool visible)
{
    SPR_setPosition(s,x,y);SPR_setVisibility(s,visible ? VISIBLE : HIDDEN);
}
static void render(void)
{
    for(u16 i=0;i<16;i++)SPR_setVisibility(dots[i],HIDDEN);
    for(u16 i=0;i<2;i++){SPR_setVisibility(shots[i],HIDDEN);SPR_setVisibility(walls[i],HIDDEN);SPR_setVisibility(markers[i],HIDDEN);}
    SPR_setVisibility(body,HIDDEN);
    if(!demo.page) {
        for(u16 i=0;i<2;i++) {
            place(walls[i],demo.target_x/16,80+56*i,true);
            place(shots[i],demo.shot[i].x/16,demo.shot[i].y/16,demo.alive[i] || demo.flash[i]);
            if(demo.flash[i])place(markers[i],demo.shot[i].x/16-2,demo.shot[i].y/16-2,true);
        }
    } else if(demo.page==1) {
        CcVec start={48*16,112*16},delta={demo.aim.x-start.x,demo.aim.y-start.y};
        CcVec end=demo.aim;
        if(demo.ray.kind==CC_HIT) {
            end.x=start.x+(s16)((s32)delta.x*demo.ray.num/demo.ray.den);
            end.y=start.y+(s16)((s32)delta.y*demo.ray.num/demo.ray.den);
            place(markers[1],end.x/16-4+demo.ray.nx*8,end.y/16-4+demo.ray.ny*8,true);
        }
        for(u16 i=0;i<16;i++)place(dots[i],(start.x+(s32)(end.x-start.x)*i/15)/16,(start.y+(s32)(end.y-start.y)*i/15)/16,true);
        place(markers[0],demo.aim.x/16-4,demo.aim.y/16-4,true);
        place(body,46,110,true);
    } else place(body,demo.player.x/16-2,demo.player.y/16-2,true);
}
int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
    PAL_setPalette(PAL0,ui_colors,CPU);PAL_setPalette(PAL1,actor.palette->data,CPU);
    u32 stone[8]={0x44444444,0x42222224,0x42222224,0x42222224,0x42222224,0x42222224,0x42222224,0x44444444};
    VDP_loadTileData(stone,96,1,CPU);SPR_initEx(40);DMA_setMaxTransferSize(4096);
    body=add(&actor);bool ok=body!=NULL;
    for(u16 i=0;i<2;i++){shots[i]=add(&shot);walls[i]=add(&wall);markers[i]=add(&cursor);ok=ok && shots[i] && walls[i] && markers[i];}
    for(u16 i=0;i<16;i++){dots[i]=add(&dot);ok=ok && dots[i];}
    if(!ok) {
        VDP_drawText("SPRITE ALLOCATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);
        while(TRUE)SYS_doVBlankProcess();
    }
    cd_init(&demo,0);render();SPR_update();SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    u16 hz=SYS_isPAL() ? 50 : 60;
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held & (u16)~previous;previous=held;
        if(pressed&BUTTON_START)paused=!paused;
        if(pressed&BUTTON_C)cd_init(&demo,demo.page+1);
        if(pressed&BUTTON_A){if(!demo.page)cd_fire(&demo);else cd_init(&demo,demo.page);}
        if(!demo.page && (pressed&BUTTON_B))demo.fast=!demo.fast;
        if(!demo.page && (pressed&BUTTON_UP))demo.moving=!demo.moving;
        u16 input=(held&BUTTON_LEFT ? CD_LEFT : 0)|(held&BUTTON_RIGHT ? CD_RIGHT : 0)|
            (held&BUTTON_UP ? CD_UP : 0)|(held&BUTTON_DOWN ? CD_DOWN : 0);
        u16 before=demo.hits;u16 steps=cd_frame(&demo,input,hz,paused);
        tone=tone>steps ? tone-steps : 0;
        if(demo.hits!=before){tone=3;PSG_setFrequency(0,700);}
        if(paused)tone=0;
        PSG_setEnvelope(0,tone ? 7 : PSG_ENVELOPE_MIN);
        render();if(!(frame++%6) || pressed)hud(hz);SPR_update();SYS_doVBlankProcess();
        while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
