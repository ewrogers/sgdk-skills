#include <genesis.h>
#include "action.h"
#include "resources.h"
#include "ui.h"

static AlGame game;
static Sprite *actor_sprite,*target,*slash,*attack_wire,*body_wire,*target_wire;
static u16 screen[28][40],counts[6],previous,frames;
static bool paused,slow,boxes=true;
static u16 tone_ticks;

static void put(const char *text,u16 x,u16 y)
{
    if(x>=40 || y>=28)return;
    u16 length=(u16)strlen(text);if(length>40-x)length=40-x;
    for(u16 i=0;i<length;i++)screen[y][x+i]=TILE_FONT_INDEX+(u8)text[i]-32;
}
static void number(u16 value,u16 x,u16 y)
{
    if(value>9999)value=9999;
    char text[5];text[4]=0;
    for(s16 i=3;i>=0;i--){text[i]=(char)('0'+value%10);value/=10;}
    put(text,x,y);
}
static void hud(u16 hz)
{
    if(DMA_getQueueSize()+28>DMA_getMaxQueueSize() || !DMA_canQueue(DMA_VRAM,1120))return;
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)screen[y][x]=TILE_FONT_INDEX;
    put("ANIMATION LAB",1,1);put(hz==50 ? "PAL / 50 HZ" : "NTSC / 60 HZ",26,1);
    const char *states[]={"IDLE","WALK","ATTACK","HURT","DEAD"};
    put("ACTION",1,3);put(states[game.actor.clock.clip],9,3);
    if(game.actor.clock.clip==AL_ATTACK)put(game.actor.hit.w ? "ACTIVE" : game.actor.pose==6 ? "STARTUP" : "RECOVERY",24,3);
    put("HP",1,5);number(game.actor.hp,4,5);put(paused ? "PAUSED" : "RUNNING",24,5);
    put("SWINGS",1,7);number(counts[1],9,7);put("HITS",22,7);number(game.actor.hits,29,7);
    put("STEPS",1,9);number(counts[0],9,9);put("ENDS",22,9);number(counts[5],29,9);
    put(slow ? "UP SLOW: ON" : "UP SLOW: OFF",1,11);put(boxes ? "C BOXES: ON" : "C BOXES: OFF",22,11);
    put("RECOVERY CANCEL OPENS AT TICK 18",1,13);
    put("ONE HIT PER ATTACK - NO AUTO REPEAT",1,15);
    put("LEFT/RIGHT MOVE   A ATTACK",1,24);put("B TAKE HIT   DOWN+B KNOCKOUT",1,25);
    put("START PAUSE   UP SLOW   C BOXES",1,26);put("A REVIVES AFTER DEATH FINISHES",1,27);
    for(u16 y=0;y<28;y++)DMA_queueDma(DMA_VRAM,screen[y],VDP_getPlaneAddress(BG_A,0,y),40,2);
}

static Sprite *add(const SpriteDefinition *definition,u16 palette)
{
    Sprite *sprite=SPR_addSprite(definition,0,0,TILE_ATTR(palette,TRUE,FALSE,FALSE));
    if(sprite)SPR_setAutoAnimation(sprite,FALSE);
    return sprite;
}

static void render(void)
{
    AlActor *a=&game.actor;AlBox body=al_body(a),dummy_box=al_dummy();
    SPR_setPosition(actor_sprite,a->x-12,154);SPR_setHFlip(actor_sprite,a->left);
    SPR_setFrame(actor_sprite,a->pose);
    SPR_setPosition(target,208,146);SPR_setFrame(target,a->flash ? 1 : 0);
    SPR_setPosition(body_wire,body.x-2,body.y-2);SPR_setVisibility(body_wire,boxes ? VISIBLE : HIDDEN);
    SPR_setPosition(target_wire,dummy_box.x-2,dummy_box.y-3);SPR_setVisibility(target_wire,boxes ? VISIBLE : HIDDEN);
    SPR_setVisibility(slash,a->hit.w ? VISIBLE : HIDDEN);
    SPR_setVisibility(attack_wire,boxes && a->hit.w ? VISIBLE : HIDDEN);
    if(a->hit.w) {
        SPR_setPosition(slash,a->hit.x-2,a->hit.y-2);SPR_setHFlip(slash,a->left);
        SPR_setFrame(slash,a->pose==8 ? 1 : 0);
        SPR_setPosition(attack_wire,a->hit.x-2,a->hit.y-2);
    }
}

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
    PAL_setPalette(PAL0,ui_colors,CPU);PAL_setPalette(PAL1,hero.palette->data,CPU);
    PAL_setPalette(PAL2,dummy.palette->data,CPU);PAL_setPalette(PAL3,arc.palette->data,CPU);
    u32 floor[8]={0x22222222,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x22222222};
    VDP_loadTileData(floor,96,1,CPU);VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,96),0,22,40,6);
    SPR_initEx(72);
    actor_sprite=add(&hero,PAL1);target=add(&dummy,PAL2);slash=add(&arc,PAL3);
    attack_wire=add(&attack_box,PAL3);body_wire=add(&body_box,PAL3);target_wire=add(&target_box,PAL3);
    if(!actor_sprite || !target || !slash || !attack_wire || !body_wire || !target_wire) {
        VDP_drawText("SPRITE ALLOCATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);
        while(TRUE)SYS_doVBlankProcess();
    }
    DMA_setMaxTransferSize(4096);al_init(&game);render();SPR_update();
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    u16 hz=SYS_isPAL() ? 50 : 60;
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held & (u16)~previous;previous=held;
        if(pressed&BUTTON_START)paused=!paused;
        if(pressed&BUTTON_UP)slow=!slow;
        if(pressed&BUTTON_C)boxes=!boxes;
        u16 input=(held&BUTTON_LEFT ? AL_LEFT : 0)|(held&BUTTON_RIGHT ? AL_RIGHT : 0)|
            (held&BUTTON_A ? AL_A : 0)|(held&BUTTON_B ? AL_B : 0)|
            ((held&(BUTTON_DOWN|BUTTON_B))==(BUTTON_DOWN|BUTTON_B) ? AL_KO : 0);
        AlBatch batch;al_frame(&game,input,hz,paused,slow,&batch);u16 events=0;
        for(u16 i=0;i<batch.count;i++) {
            events|=batch.events[i];
            for(u16 bit=0;bit<6;bit++)if((batch.events[i]&(1<<bit)) && counts[bit]<9999)counts[bit]++;
            if(tone_ticks)tone_ticks--;
        }
        if(paused)tone_ticks=0;
        else if(events&(AL_HIT|AL_PAIN|AL_FALL|AL_SWING|AL_FOOT)) {
            u16 frequency=events&AL_FALL ? 100 : events&AL_PAIN ? 180 : events&AL_HIT ? 360 : events&AL_SWING ? 800 : 1100;
            tone_ticks=events&(AL_FALL|AL_PAIN|AL_HIT|AL_SWING) ? 6 : 2;PSG_setFrequency(0,frequency);
        }
        PSG_setEnvelope(0,tone_ticks ? 6 : PSG_ENVELOPE_MIN);
        render();if(!(frames++%6) || pressed)hud(hz);SPR_update();
        SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
