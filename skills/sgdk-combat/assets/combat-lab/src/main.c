#include <genesis.h>
#include "combat.h"
#include "resources.h"
#include "font.h"
#include "ui.h"
static CcGame game;
static Sprite *actor_sprites[CC_ACTORS],*shot_sprites[CC_SHOTS],*fx_sprites[CC_FX],*swing_sprites[CC_ACTORS];
static u16 hud_map[4][40],title_base,blank_tile,meter_base,last_layout,frame;
static s16 line_y[26];static u16 line_w[26],line_h[26],drawn;
static bool music_paused;
static CcInput snapshot(void)
{
    CcInput in={{0,0}};
    for(u16 p=0;p<2;p++) {
        u16 h=JOY_readJoypad(p);
        in.held[p]=(h&BUTTON_LEFT ? CC_LEFT : 0)|(h&BUTTON_RIGHT ? CC_RIGHT : 0)|
            (h&BUTTON_UP ? CC_UP : 0)|(h&BUTTON_DOWN ? CC_DOWN_KEY : 0)|
            (h&BUTTON_A ? CC_A : 0)|(h&BUTTON_B ? CC_B : 0)|(h&BUTTON_C ? CC_C : 0)|(h&BUTTON_START ? CC_START : 0);
    }
    return in;
}
static void label(const char *s,u16 y)
{
    VDP_drawTextEx(WINDOW,s,TILE_ATTR(PAL3,TRUE,FALSE,FALSE),(40-strlen(s))/2,y,DMA_QUEUE_COPY);
}
static void panel(u16 y,u16 h)
{
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile),1,y,38,h);
}
static void layout(void)
{
    u16 kind=game.mode;
    if(game.paused)kind=CC_COMPLETE+1;
    if(kind==last_layout)return;
    while(DMA_getQueueSize())SYS_doVBlankProcess();
    VDP_clearPlane(WINDOW,TRUE);VDP_setWindowVPos(FALSE,kind==CC_PLAY ? 4 : 28);
    const Image *image=kind==CC_TITLE ? &title : &court;
    VDP_setTileMapEx(BG_B,image->tilemap,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,kind==CC_TITLE ? title_base : TILE_USER_INDEX),0,0,0,0,40,28,CPU);
    if(kind==CC_TITLE) {
        panel(1,5);panel(14,13);label("C O P P E R   C O U R T",2);label("COMBAT LAB",4);
        label("START: ENTER THE PRACTICE COURT",15);
        label("A STRIKE   B GUARD   C BOLT",18);
        label("FACE ATTACKS TO BLOCK",20);label("RED WINDUP? DODGE THE HEAVY SWING",22);
        label("SECOND PAD START: JOIN ANY TIME",25);
    } else if(kind!=CC_PLAY) {
        panel(8,12);
        if(game.paused){label("PAUSED",10);label("ANY JOINED START: RESUME",14);label("SECOND PAD START CAN STILL JOIN",17);}
        else if(kind==CC_LOST){label("THE COURT GETS THE BETTER OF YOU",10);label("START: RETRY THIS ROUND",15);}
        else if(kind==CC_CLEAR){label("ROUND CLEAR",10);label("START: NEXT ROUND",15);}
        else {label("THE COPPER COURT SALUTES YOU",10);label("ALL THREE ROUNDS COMPLETE",13);label("START: TITLE",17);}
    }
    last_layout=kind;
}
static void put(const char *s,u16 x,u16 y)
{
    for(u16 i=0;s[i] && x+i<40;i++)hud_map[y][x+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);
}
static void hud(void)
{
    if(!DMA_canQueue(DMA_VRAM,160) || DMA_getQueueSize()+4>DMA_getMaxQueueSize())return;
    for(u16 y=0;y<4;y++)for(u16 x=0;x<40;x++)hud_map[y][x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,blank_tile);
    for(u16 p=0;p<2;p++) {
        u16 x=p*20;CcActor *a=&game.actor[p];char who[3]={'P',(char)('1'+p),0};put(who,x+1,0);
        if(!game.joined[p]){put("START TO JOIN",x+3,1);continue;}
        put("HP",x+4,0);put("C BOLT",x+1,1);
        u16 charge=a->bolt_cd>=75 ? 0 : (75-a->bolt_cd)*48/75;
        for(u16 t=0;t<6;t++) {
            u16 fill=charge>t*8 ? charge-t*8 : 0;if(fill>8)fill=8;
            hud_map[0][x+7+t]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,meter_base+p*9+(a->hp>t ? 8 : 0));
            hud_map[1][x+7+t]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,meter_base+18+fill);
        }
        if(a->state==CC_DOWN)put("RETURNING...",x+1,2);
        else put(a->state==CC_GUARD ? "GUARD: FRONT ONLY" : a->state==CC_HURT ? "HIT! RECOVERING" : "A STRIKE  B GUARD",x+1,2);
    }
    char round[]="ROUND 1/3   FOES 0   START PAUSE";round[6]=(char)('1'+game.wave);round[17]=(char)('0'+cc_enemies(&game));put(round,2,3);
    for(u16 y=0;y<4;y++)DMA_queueDma(DMA_VRAM,hud_map[y],VDP_getPlaneAddress(WINDOW,0,y),40,2);
}
static Sprite *add(const SpriteDefinition *def,u16 pal)
{
    Sprite *s=SPR_addSprite(def,0,0,TILE_ATTR(pal,FALSE,FALSE,FALSE));
    if(s){SPR_setAutoAnimation(s,FALSE);SPR_setVisibility(s,HIDDEN);}return s;
}
static void place(Sprite *s,s16 x,s16 y,u16 w,u16 h,bool visible,s16 depth)
{
    SPR_setPosition(s,x,y);SPR_setVisibility(s,visible ? VISIBLE : HIDDEN);SPR_setDepth(s,depth);
    if(visible && drawn<26){line_y[drawn]=y;line_w[drawn]=w;line_h[drawn++]=h;}
}
static bool effect_fits(s16 y)
{
    for(s16 line=y;line<y+16;line++)if(line>=32 && line<224) {
        u16 count=1,pixels=16;
        for(u16 n=0;n<drawn;n++)if(line>=line_y[n] && line<line_y[n]+(s16)line_h[n]){count++;pixels+=line_w[n];}
        if(count>20 || pixels>320)return FALSE;
    }
    return TRUE;
}
static void render(void)
{
    drawn=0;bool showing=game.mode==CC_PLAY;
    for(u16 i=0;i<CC_ACTORS;i++) {
        CcActor *a=&game.actor[i];bool show=showing && a->active && a->state!=CC_DOWN && (!a->invulnerable || (game.ticks&4));
        u16 anim=i<2 ? i : a->kind+1;
        SPR_setAnim(actor_sprites[i],anim);
        u16 pose=a->state==CC_HURT ? 3 : a->state==CC_CAST ? (a->time<8 ? 1 : 2) :
            a->state==CC_STRIKE ? (cc_active(a) ? 2 : a->time<(a->kind==CC_PLAYER ? 6 : a->kind==CC_SENTINEL ? 24 : 18) ? 1 : 3) : 0;
        SPR_setFrame(actor_sprites[i],a->state==CC_GUARD ? 16+a->facing : a->facing*4+pose);
        place(actor_sprites[i],a->x-12,a->y-29,24,32,show,-a->y);
    }
    for(u16 i=0;i<CC_SHOTS;i++) {
        CcShot *s=&game.shot[i];SPR_setAnim(shot_sprites[i],s->team ? 1 : 0);SPR_setFrame(shot_sprites[i],(game.ticks/3)%4);
        place(shot_sprites[i],s->x-4,s->y-4,8,8,showing && s->active,-250);
    }
    for(u16 i=0;i<CC_ACTORS;i++) {
        CcActor *a=&game.actor[i];CcBox origin=cc_swing_origin(a);
        SPR_setAnim(swing_sprites[i],a->facing+(a->kind==CC_SENTINEL ? 4 : 0));
        SPR_setFrame(swing_sprites[i],cc_swing_frame(a));
        place(swing_sprites[i],origin.x,origin.y,24,24,showing && cc_active(a),-251);
    }
    for(u16 i=0;i<CC_FX;i++) {
        CcFx *f=&game.fx[i];SPR_setAnim(fx_sprites[i],f->kind);SPR_setFrame(fx_sprites[i],f->life ? (18-f->life)/5 : 0);
        place(fx_sprites[i],f->x-8,f->y-8,16,16,showing && f->life && effect_fits(f->y-8),-260);
    }
}
static void sound(void)
{
    const u8 *data=NULL;u32 size=0;
#define CUE(name) do{data=snd_##name;size=sizeof(snd_##name);}while(0)
    if(game.events&CC_WIN)CUE(win);
    else if(game.events&CC_FALL)CUE(fall);
    else if(game.events&CC_BLOCK)CUE(block);
    else if(game.events&CC_IMPACT)CUE(impact);
    else if(game.events&CC_JOIN)CUE(join);
    else if(game.events&CC_FIRE)CUE(fire);
    else if(game.events&CC_SWING)CUE(swing);
#undef CUE
    if(data)XGM2_playPCMEx(data,size,SOUND_PCM_CH3,6,FALSE,FALSE);
}
int main(bool hard_reset)
{
    (void)hard_reset;cc_init(&game);last_layout=CC_NONE;frame=0;music_paused=FALSE;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);SPR_initEx(160);DMA_setMaxTransferSize(4096);
    title_base=TILE_USER_INDEX+court.tileset->numTile;blank_tile=title_base+title.tileset->numTile;meter_base=blank_tile+1;
    if(meter_base+27>TILE_SPRITE_INDEX) {
        VDP_drawText("ART EXCEEDS VRAM RESERVATION",2,10);SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();
    }
    VDP_drawImageEx(BG_B,&court,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,FALSE);
    VDP_loadTileSet(title.tileset,title_base,CPU);VDP_loadFontData(font_tiles,96,CPU);
    u32 blank[8];for(u16 y=0;y<8;y++)blank[y]=0x11111111;VDP_loadTileData(blank,blank_tile,1,CPU);
    u32 meters[27*8];
    for(u16 role=0;role<3;role++)for(u16 fill=0;fill<=8;fill++)for(u16 y=0;y<8;y++) {
        u32 word=0;for(u16 x=0;x<8;x++)word=word<<4|(y==1 || y==6 ? 6 : y>=2 && y<=5 ? (x<fill ? (role==2 ? 5 : role+2) : 9) : 1);
        meters[(role*9+fill)*8+y]=word;
    }
    VDP_loadTileData(meters,meter_base,27,CPU);
    PAL_setPalette(PAL0,court.palette->data,CPU);PAL_setPalette(PAL1,fighters.palette->data,CPU);PAL_setPalette(PAL2,bolts.palette->data,CPU);PAL_setPalette(PAL3,ui_colors,CPU);
    VDP_setBackgroundColor(1);bool ok=TRUE;
    for(u16 i=0;i<CC_ACTORS;i++){actor_sprites[i]=add(&fighters,PAL1);swing_sprites[i]=add(&swords,PAL2);ok=ok && actor_sprites[i] && swing_sprites[i];}
    for(u16 i=0;i<CC_SHOTS;i++){shot_sprites[i]=add(&bolts,PAL2);ok=ok && shot_sprites[i];}
    for(u16 i=0;i<CC_FX;i++){fx_sprites[i]=add(&sparks,PAL2);ok=ok && fx_sprites[i];}
    if(!ok){VDP_drawText("SPRITE RESERVATION FAILED",2,10);SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();}
    SYS_enableInts();for(u16 n=0;n<3;n++){SPR_update();SYS_doVBlankProcess();}
    CcInput first=snapshot();game.previous[0]=first.held[0];game.previous[1]=first.held[1];
    layout();SPR_update();SYS_doVBlankProcess();VDP_setEnable(TRUE);XGM2_play(court_music);Z80_setForceDelayDMA(TRUE);
    while(TRUE) {
        CcInput input=snapshot();cc_frame(&game,&input,SYS_isPAL() ? 50 : 60);
        if(game.paused!=music_paused){music_paused=game.paused;if(music_paused){XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH3);}else XGM2_resume();}
        sound();layout();render();if(game.mode==CC_PLAY && !game.paused && (!(frame%6) || game.events))hud();frame++;
        SPR_update();SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
