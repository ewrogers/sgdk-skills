#include <genesis.h>
#include "coop.h"
#include "resources.h"
#include "font.h"
#include "ui.h"
static CpGame game;
static Sprite *keepers[4],*moths[4],*motes[8],*lamps[4];
static u16 panel[5][40],title_base,bar_tile,markers,frame,topology,last_layout;
static bool demo;static u32 demo_frame;
static u16 port_kind(u16 p)
{
    switch(JOY_getPortType(p)) {
        case PORT_TYPE_PAD:return CP_PORT_PAD;
        case PORT_TYPE_TEAMPLAYER:return CP_PORT_TEAM;
        case PORT_TYPE_EA4WAYPLAY:return CP_PORT_EA;
        default:return CP_PORT_OTHER;
    }
}
static void configure(void)
{
    CpPlan plan=cp_plan(port_kind(PORT_1),port_kind(PORT_2));
    const u16 supports[]={JOY_SUPPORT_OFF,JOY_SUPPORT_6BTN,JOY_SUPPORT_TEAMPLAYER,JOY_SUPPORT_EA4WAYPLAY};
    if(plan.reader[0]==CP_READER_EA)JOY_setSupport(PORT_2,JOY_SUPPORT_OFF);
    for(u16 p=0;p<2;p++)JOY_setSupport(p,supports[plan.reader[p]]);
    topology=plan.mask;
}
static CpInput snapshot(void)
{
    CpInput input={0};
    for(u16 d=0;d<CP_DEVICES;d++) {
        u16 h=JOY_readJoypad(d);u8 type=JOY_getJoypadType(d);
        if(!(topology&(1u<<d)) || (type!=JOY_TYPE_PAD3 && type!=JOY_TYPE_PAD6))continue;
        input.valid|=1u<<d;
        input.held[d]=(h&BUTTON_LEFT ? CP_LEFT : 0)|(h&BUTTON_RIGHT ? CP_RIGHT : 0)|
            (h&BUTTON_UP ? CP_UP : 0)|(h&BUTTON_DOWN ? CP_DOWN : 0)|(h&BUTTON_A ? CP_A : 0)|
            (h&BUTTON_B ? CP_B : 0)|(h&BUTTON_C ? CP_C : 0)|(h&BUTTON_START ? CP_START : 0);
    }
    return input;
}
static void put(const char *s,u16 x,u16 y)
{
    if(y>=5 || x>=40)return;
    for(u16 i=0;s[i] && x+i<40;i++)panel[y][x+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);
}
static void value(u16 n,u16 x,u16 y)
{
    if(n>99)n=99;
    char s[3]={(char)('0'+n/10),(char)('0'+n%10),0};put(s,x,y);
}
static void label(const char *s,u16 y)
{
    VDP_drawTextEx(WINDOW,s,TILE_ATTR(PAL3,TRUE,FALSE,FALSE),(40-strlen(s))/2,y,DMA_QUEUE_COPY);
}
static void text_panel(u16 y,u16 height)
{
    /* Opaque cells hide the artwork; transparent Window cells reveal BG_B. */
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,bar_tile),1,y,38,height);
}
static void layout(u16 kind)
{
    if(kind==last_layout)return;
    /* Drain old owners before replacing either the title plane or window map. */
    while(DMA_getQueueSize())SYS_doVBlankProcess();
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(WINDOW,TRUE);
    VDP_setWindowVPos(FALSE,kind==1 ? 5 : 28);
    if(kind==1) {
        VDP_setTileMapEx(BG_B,world.tilemap,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,0,0,64,48,CPU);
    } else {
        VDP_setTileMapEx(BG_B,title.tilemap,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,title_base),0,0,0,0,40,28,CPU);
        if(kind==0) {
            text_panel(2,5);text_panel(8,5);text_panel(16,11);
            label("B E L L K E E P E R S",3);label("CO-OP LAB",5);
            label("START ON ANY PAD TO JOIN",9);label("UP TO FOUR LANTERN KEEPERS",11);
            label("D-PAD MOVE   A STUN   B DASH",17);label("A+C+START LEAVES THE PARTY",19);
            label("B RESCAN PADS   C CPU TOUR",22);label("DIRECT PADS / SEGA TAP / EA TAP",25);
        } else {
            text_panel(3,6);text_panel(10,9);text_panel(22,3);
            label("THE BELLS RING AGAIN",4);label("ALL FOUR LAMPS RESTORED",7);
            for(u16 i=0;i<4;i++)if(game.player[i].device!=CP_NONE) {
                char text[32]="P1  MOTES 00  RETURNS 00";text[1]=(char)('1'+i);
                u16 n=game.player[i].gems; text[10]=(char)('0'+n/10);text[11]=(char)('0'+n%10);
                n=game.player[i].deaths;if(n>99)n=99;text[21]=(char)('0'+n/10);text[22]=(char)('0'+n%10);
                label(text,11+i*2);
            }
            label(demo ? "START: LEAVE CPU TOUR" : "START: ANOTHER ROUND",23);
        }
    }
    last_layout=kind;
}
static void hud(void)
{
    if(!DMA_canQueue(DMA_VRAM,200) || DMA_getQueueSize()+5>DMA_getMaxQueueSize())return;
    for(u16 y=0;y<5;y++)for(u16 x=0;x<40;x++)panel[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,bar_tile);
    for(u16 i=0;i<4;i++) {
        u16 x=i*10;CpPlayer *p=&game.player[i];
        panel[0][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,markers+i);
        char who[3]={'P',(char)('1'+i),0};put(who,x+1,0);
        if(p->device==CP_NONE){put("START JOIN",x,1);continue;}
        put("PAD",x+4,0);char slot[2]={(char)('1'+p->device),0};put(slot,x+7,0);
        if(p->respawn){put("BACK",x,1);value((p->respawn+59)/60,x+5,1);}
        else {put("HP",x,1);for(u16 h=0;h<p->hp;h++)put("*",x+3+h,1);}
        put("MOTES",x,2);value(p->gems,x+6,2);
        put(p->stun_cd ? "STUN ..." : "STUN OK",x,3);
    }
    if(game.paused)put("PAUSED - ANY JOINED START RESUMES",2,4);
    else if(demo)put("CPU TOUR - START RETURNS TO TITLE",2,4);
    else if(game.tethered)put("STAY TOGETHER - MOVE TOWARD THE PARTY",1,4);
    else {put("LAMPS",1,4);value(game.score/2,7,4);put("/04 A STUN B DASH START PAUSE",9,4);}
    for(u16 y=0;y<5;y++)DMA_queueDma(DMA_VRAM,panel[y],VDP_getPlaneAddress(WINDOW,0,y),40,2);
}
static Sprite *add(const SpriteDefinition *def,u16 palette)
{
    Sprite *s=SPR_addSprite(def,0,0,TILE_ATTR(palette,FALSE,FALSE,FALSE));
    if(s){SPR_setAutoAnimation(s,FALSE);SPR_setVisibility(s,HIDDEN);}return s;
}
static void place(Sprite *s,s16 x,s16 y,u16 w,u16 h,bool show)
{
    s16 xx=x-game.camera_x-(s16)w/2,yy=y-game.camera_y+40-(s16)h+3;
    show=show && game.running && !game.won && xx+(s16)w>0 && xx<320 && yy+(s16)h>40 && yy<224;
    SPR_setPosition(s,xx,yy);SPR_setVisibility(s,show ? VISIBLE : HIDDEN);SPR_setDepth(s,-y);
}
static void render(void)
{
    bool playing=game.running && !game.won;
    VDP_setHorizontalScrollVSync(BG_B,playing ? -game.camera_x : 0);VDP_setVerticalScrollVSync(BG_B,playing ? game.camera_y-40 : 0);
    for(u16 i=0;i<4;i++) {
        CpPlayer *p=&game.player[i];bool show=p->device!=CP_NONE && !p->respawn && (!p->invulnerable || (game.ticks&4));
        place(keepers[i],p->x,p->y,16,24,show);SPR_setAnim(keepers[i],i);
        u16 step=p->moving ? (p->walk/6)%4 : 0;step=step==3 ? 1 : step;
        SPR_setFrame(keepers[i],p->facing*3+step);
        CpPoint q=cp_enemy(&game,i);place(moths[i],q.x,q.y,16,16,!game.enemy_stun[i] || (game.ticks&8));SPR_setFrame(moths[i],(game.ticks/10)%4);
        place(lamps[i],cp_beacons[i].x,cp_beacons[i].y,16,24,TRUE);SPR_setFrame(lamps[i],game.score/2>i ? 1+(game.ticks/12)%3 : 0);
    }
    for(u16 i=0;i<8;i++){place(motes[i],cp_coins[i].x,cp_coins[i].y,8,8,!(game.coin_mask&(1u<<i)));SPR_setFrame(motes[i],(game.ticks/10+i)%4);}
}
static void sound(u16 events)
{
    const u8 *sample=NULL;u32 len=0;
    if(events&CP_CLEAR){sample=snd_clear;len=sizeof(snd_clear);}
    else if(events&CP_HIT){sample=snd_hurt;len=sizeof(snd_hurt);}
    else if(events&CP_JOIN){sample=snd_join;len=sizeof(snd_join);}
    else if(events&CP_RESPAWN){sample=snd_return;len=sizeof(snd_return);}
    else if(events&CP_PICKUP){sample=snd_coin;len=sizeof(snd_coin);}
    else if(events&CP_STUN){sample=snd_stun;len=sizeof(snd_stun);}
    if(sample)XGM2_playPCMEx(sample,len,SOUND_PCM_CH3,6,FALSE,FALSE);
}
int main(bool hard_reset)
{
    (void)hard_reset;demo=FALSE;demo_frame=0;frame=0;last_layout=CP_NONE;cp_init(&game);
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,64,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);SPR_initEx(128);DMA_setMaxTransferSize(4096);
    title_base=TILE_USER_INDEX+world.tileset->numTile;bar_tile=title_base+title.tileset->numTile;markers=bar_tile+1;
    if(markers+4>TILE_SPRITE_INDEX) {
        VDP_drawText("ART EXCEEDS VRAM RESERVATION",2,10);SYS_enableInts();VDP_setEnable(TRUE);
        while(TRUE)SYS_doVBlankProcess();
    }
    VDP_drawImageEx(BG_B,&world,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,FALSE);
    VDP_loadTileSet(title.tileset,title_base,CPU);VDP_loadFontData(font_tiles,96,CPU);
    u32 patterns[40];for(u16 t=0;t<5;t++)for(u16 y=0;y<8;y++) {
        u32 word=0;for(u16 x=0;x<8;x++)word=word<<4|(t && x>=1 && x<=6 && y>=1 && y<=6 ? t+1 : 1);
        patterns[t*8+y]=word;
    }
    VDP_loadTileData(patterns,bar_tile,5,CPU);
    PAL_setPalette(PAL0,world.palette->data,CPU);PAL_setPalette(PAL1,hero.palette->data,CPU);PAL_setPalette(PAL2,moth.palette->data,CPU);PAL_setPalette(PAL3,ui_colors,CPU);
    VDP_setBackgroundColor(1);bool ok=TRUE;
    for(u16 i=0;i<4;i++){keepers[i]=add(&hero,PAL1);moths[i]=add(&moth,PAL2);lamps[i]=add(&beacon,PAL2);ok=ok && keepers[i] && moths[i] && lamps[i];}
    for(u16 i=0;i<8;i++){motes[i]=add(&mote,PAL2);ok=ok && motes[i];}
    if(!ok) {
        VDP_drawText("SPRITE RESERVATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();
    }
    SYS_enableInts();configure();
    for(u16 warm=0;warm<3;warm++){SPR_update();SYS_doVBlankProcess();}
    CpInput first=snapshot();cp_seed_inputs(&game,&first);
    layout(0);SPR_update();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    XGM2_play(courtyard_music);Z80_setForceDelayDMA(TRUE);
    u16 old_real[8]={0};bool was_paused=FALSE;
    while(TRUE) {
        CpInput real=snapshot();u16 real_pressed[8];
        for(u16 d=0;d<8;d++){real_pressed[d]=real.held[d]&~old_real[d];old_real[d]=real.held[d];}
        if(demo) {
            bool exit=FALSE;for(u16 d=0;d<8;d++)if(real_pressed[d]&CP_START)exit=TRUE;
            if(exit){demo=FALSE;cp_init(&game);cp_seed_inputs(&game,&real);}
            else {CpInput synthetic;cp_demo_input(&game,demo_frame++,&synthetic);cp_frame(&game,&synthetic,SYS_isPAL() ? 50 : 60);}
        } else {
            if(!game.running) {
                bool scan=FALSE,tour=FALSE;for(u16 d=0;d<8;d++){if(real_pressed[d]&CP_B)scan=TRUE;if(real_pressed[d]&CP_C)tour=TRUE;}
                if(scan){JOY_reset();configure();for(u16 n=0;n<3;n++)SYS_doVBlankProcess();real=snapshot();cp_seed_inputs(&game,&real);}
                if(tour){demo=TRUE;demo_frame=0;cp_init(&game);}
            }
            if(!demo)cp_frame(&game,&real,SYS_isPAL() ? 50 : 60);
        }
        if(game.paused!=was_paused) {
            was_paused=game.paused;if(was_paused){XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH3);}else XGM2_resume();
        }
        sound(game.events);layout(!game.running ? 0 : game.won ? 2 : 1);render();
        if(game.running && !game.won && (!(frame%6) || game.events))hud();
        frame++;
        SPR_update();SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
