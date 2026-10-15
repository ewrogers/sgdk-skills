#include <genesis.h>
#include "view.h"
#include "level.h"
#include "text.h"
#include "scene_video.h"
#include "resources.h"

static Sprite *actors[2], *coins[COIN_COUNT], *cursor, *boss, *smoke, *mega, *lasers[LASER_CAPACITY];
static u16 palette[64], heap_baseline;
static SceneId current;
static Lab previous;
static bool drawn;
static const u32 floor_tile[8]={0x22222222,0x23333333,0x23222222,0x23222222,
                                0x23222222,0x23222222,0x23222222,0x23222222};

static const u32 block_tile[8]={0x66666666,0x65555554,0x65555554,0x65555554,
                                0x65555554,0x65555554,0x65555554,0x44444444};

static void field(const char *text, u16 x, u16 y, u16 width)
{
    char row[41];
    if (width>40 || x>40 || x+width>40 || y>=28) return;
    if (text_field(row,sizeof row,text,width)) VDP_drawText(row,x,y);
}
static void tint_field(const char *text,u16 x,u16 y,u16 width,u16 color)
{
    VDP_setTextPalette(color); field(text,x,y,width); VDP_setTextPalette(PAL3);
}
static void centered_color(const char *text,u16 y,u16 color)
{
    u16 length=0; while (length<38 && text[length]) ++length;
    tint_field(text,(40-length)/2,y,length,color);
}
static void centered(const char *text, u16 y)
{
    u16 length=0; while (length<38 && text[length]) ++length;
    tint_field(text,(40-length)/2,y,length,y>=23 ? PAL1 : PAL3);
}
static void number(u32 value,u16 x,u16 y,u16 width)
{
    char out[11]; if (text_number(out,sizeof out,value,width)) field(out,x,y,width);
}
static void choices(const char *const *labels,u16 count,u16 selected,u16 y)
{
    for (u16 i=0; i<count; ++i) {
        tint_field(i==selected && current!=SCENE_TITLE ? ">" : "",9,y+i*2,1,PAL2);
        tint_field(labels[i],12,y+i*2,24,i==selected ? PAL2 : PAL3);
    }
}
static void colors(const Lab *lab)
{
    for (u16 i=0; i<64; ++i) palette[i]=0;
    for (u16 i=0; i<16; ++i) {
        palette[i]=hero_image.palette->data[i];
        palette[16+i]=hero_image.palette->data[i];
        palette[32+i]=coin_image.palette->data[i];
    }
    /* Two character palettes use the same resident robot frames. */
    palette[7]=RGB24_TO_VDPCOLOR(0x245580); palette[8]=RGB24_TO_VDPCOLOR(0x88CCFF);
    palette[23]=RGB24_TO_VDPCOLOR(0x802424); palette[24]=RGB24_TO_VDPCOLOR(0xFFAA88);
    palette[49]=RGB24_TO_VDPCOLOR(0x101018);
    static const u32 floors[]={0x282020,0x182820,0x202038,0x302018};
    static const u32 edges[]={0x403028,0x284030,0x403860,0x603020};
    palette[50]=RGB24_TO_VDPCOLOR(floors[lab->run.room]);
    palette[51]=RGB24_TO_VDPCOLOR(edges[lab->run.room]);
    palette[15]=RGB24_TO_VDPCOLOR(0x88BBFF);
    palette[31]=RGB24_TO_VDPCOLOR(0x889099);
    palette[47]=RGB24_TO_VDPCOLOR(0xFFCC66);
    palette[52]=RGB24_TO_VDPCOLOR(0x181818);
    palette[53]=RGB24_TO_VDPCOLOR(0x505050);
    palette[54]=RGB24_TO_VDPCOLOR(0x888888);
    palette[63]=RGB24_TO_VDPCOLOR(0xDDDDDD);
}
bool view_init(void)
{
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE); VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3); VDP_setBackgroundColor(49);
    VDP_setWindowHPos(FALSE,0); VDP_setWindowVPos(FALSE,0);
    SPR_initEx(2*hero_image.maxNumTile+COIN_COUNT*coin_image.maxNumTile+
               boss_image.maxNumTile+smoke_image.maxNumTile+mega_image.maxNumTile+
               LASER_CAPACITY*laser_image.maxNumTile);
    u16 black[64]={0}; PAL_setColors(0,black,64,CPU);
    heap_baseline=MEM_getFree(); return SPR_isInitialized();
}
void view_leave(void)
{
    scene_video_leave();
    for (u16 p=0; p<2; ++p) actors[p]=NULL;
    for (u16 i=0; i<COIN_COUNT; ++i) coins[i]=NULL;
    for (u16 i=0;i<LASER_CAPACITY;++i) lasers[i]=NULL;
    cursor=boss=smoke=mega=NULL; drawn=false;
}
bool view_clean(void)
{
    return SPR_getNumActiveSprite()==0 && MEM_getFree()==heap_baseline;
}
const u16 *view_palette(void) { return palette; }
static Sprite *actor(u16 character,s16 x,s16 y)
{
    Sprite *sprite=SPR_addSprite(&hero_image,x,y,TILE_ATTR(character,TRUE,FALSE,FALSE));
    if (sprite) SPR_setAutoAnimation(sprite,FALSE);
    return sprite;
}
static void header(const char *title)
{
    centered_color("======================================",0,PAL1);
    centered_color(title,2,PAL2);
    centered_color("--------------------------------------",4,PAL1);
}
bool view_enter(SceneId scene,const Lab *lab,const Checkpoint *saved)
{
    current=scene; drawn=false; colors(lab);
    VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3);
    if (scene==SCENE_TITLE) {
        header("TRANSITION LAB");
        for (u16 i=0;i<16;++i) palette[i]=title_art.palette->data[i];
        if ((u32)TILE_USER_INDEX+title_art.tileset->numTile>TILE_USER_MAX_INDEX ||
            !VDP_drawImageEx(BG_B,&title_art,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),
                             10,5,FALSE,TRUE)) return false;
        cursor=SPR_addSprite(&coin_image,72,112,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
        if (!cursor) return false;
        SPR_setAutoAnimation(cursor,FALSE);
        centered("1 OR 2 PLAYERS",12);
        centered("D-PAD CHOOSE   A / START SELECT",25);
    } else if (scene==SCENE_SELECT) {
        header("CHOOSE YOUR CHARACTER");
        for (u16 p=0;p<2;++p) { actors[p]=actor(lab->run.character[p],88+p*128,88); if (!actors[p]) return false; }
        field("PLAYER 1",6,8,12); field("PLAYER 2",23,8,12);
        centered("LEFT / RIGHT CHOOSE   START JOIN",23);
        centered("A SKIP 1 SECOND   B BACK",25);
    } else if (scene==SCENE_PLAY) {
        static const char *const titles[]={"ROOM 1 / EMBER COURT","ROOM 2 / MOSS GARDEN",
                                           "ROOM 3 / MOON TERRACE","ROOM 4 / THE COIN GUARDIAN"};
        header(titles[lab->run.room]);
        VDP_loadTileData(floor_tile,TILE_USER_INDEX,1,DMA);
        VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX),2,8,36,18);
        VDP_loadTileData(block_tile,TILE_USER_INDEX+1,1,DMA);
        for (u16 i=0;i<level_block_count(lab->run.room);++i) {
            const Block *block=level_block(lab->run.room,i);
            VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+1),
                               block->x/8,block->y/8,block->w/8,block->h/8);
        }
        for (u16 p=0;p<2;++p) if (lab->run.joined & (1u<<p)) {
            actors[p]=actor(lab->run.character[p],lab->run.x[p],lab->run.y[p]);
            if (!actors[p]) return false;
        }
        for (u16 i=0;i<COIN_COUNT;++i) {
            coins[i]=SPR_addSprite(&coin_image,lab_coin_x(lab->run.room,i)+4,lab_coin_y(lab->run.room,i)+4,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
            if (!coins[i]) return false;
            SPR_setAutoAnimation(coins[i],FALSE);
        }
        if (lab->run.room==ROOM_COUNT-1) {
            boss=SPR_addSprite(&boss_image,lab->boss_x,lab->boss_y,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
            smoke=SPR_addSprite(&smoke_image,lab->boss_x,lab->boss_y,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
            mega=SPR_addSprite(&mega_image,lab->mega_x,lab->mega_y,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
            if (!boss || !smoke || !mega) return false;
            SPR_setAutoAnimation(boss,FALSE); SPR_setAutoAnimation(smoke,FALSE); SPR_setAutoAnimation(mega,FALSE);
            for (u16 i=0;i<LASER_CAPACITY;++i) {
                lasers[i]=SPR_addSprite(&laser_image,0,0,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
                if (!lasers[i]) return false;
                SPR_setAutoAnimation(lasers[i],FALSE);
            }
        }
        field("P1",3,6,3); field("P2",24,6,3);
        centered(lab->run.room==ROOM_COUNT-1 ? "AVOID THE GUARDIAN   START PAUSE" : "COLLECT EVERY COIN   START PAUSE",26);
    } else if (scene==SCENE_OPTIONS) {
        header("OPTIONS"); centered("LEFT / RIGHT OR A TO CHANGE",23); centered("B BACK",25);
    } else if (scene==SCENE_CREDITS) {
        header("CREDITS"); centered("SGDK SKILLS / TRANSITION LAB",9);
        centered("ORIGINAL EXAMPLE AND PIXEL ART",12); centered("BUILT WITH SGDK",15);
        if (lab->run.boss_state==2) {
            centered_color("THE MEGA COIN IS YOURS!",6,PAL2);
            field("P1 COINS",8,20,10); number(lab->run.score[0],18,20,2);
            field("P2 COINS",23,20,10); number(lab->run.score[1],33,20,2);
        }
        centered("MIT LICENSE",18); centered("A / B / START BACK",25);
    } else if (scene==SCENE_CONTINUE) {
        header("CONTINUE YOUR ADVENTURE");
        if (lab->has_save) {
            field("SAVED ROOM",8,8,16); number(saved->run.room+1,27,8,2);
            field(saved->run.joined==3 ? "2 PLAYERS" : "1 PLAYER",8,10,24);
            field("COINS COLLECTED",8,12,17);
            number(saved->run.score[0]+saved->run.score[1],27,12,3);
        } else centered("NO VALID CHECKPOINT",10);
        centered("A / START SELECT   B BACK",25);

    }
    view_draw(lab); return true;
}
static void select_draw(const Lab *lab)
{
    static const char *const names[]={"SCOUT","SPARK"};
    for (u16 p=0;p<2;++p) {
        field(names[lab->run.character[p]],7+p*16,15,10);
        field(lab->run.joined & (1u<<p) ? "JOINED" : "PRESS START",6+p*17,17,12);
        SPR_setPalette(actors[p],lab->run.character[p]);
        SPR_setAnimAndFrame(actors[p],0,4);
    }
    if (!drawn || (previous.run.joined!=0)!=(lab->run.joined!=0) ||
        (previous.join_remaining+59)/60!=(lab->join_remaining+59)/60) {
        field("",2,20,36);
        if (lab->run.joined) {
            field("STARTING IN",12,20,12); number((lab->join_remaining+59)/60,25,20,2);
        } else centered("JOIN TO START THE COUNTDOWN",20);
    }
}
static void play_draw(const Lab *lab)
{
    if (drawn && previous.paused!=lab->paused) {
        VDP_clearTextArea(3,9,34,13);
    }
    if (!drawn || previous.run.score[0]!=lab->run.score[0]) number(lab->run.score[0],7,6,3);
    if (!drawn || previous.run.score[1]!=lab->run.score[1]) number(lab->run.score[1],28,6,3);
    for (u16 p=0;p<2;++p) if (actors[p]) {
        SPR_setPosition(actors[p],lab->run.x[p],lab->run.y[p]);
        SPR_setAnimAndFrame(actors[p],lab->facing[p],lab->moving[p] ? lab->animation_ticks[p]/8 : 4+lab->animation_ticks[p]/24);
        SPR_setHFlip(actors[p],lab->flip[p]);
        SPR_setVisibility(actors[p],lab->invulnerable[p] && (lab->phase/6)%2 ? HIDDEN : VISIBLE);
    }
    if (!lab->paused && ((!lab->run.coins && lab->run.room<ROOM_COUNT-1) || lab->run.boss_state==2) &&
        (!drawn || previous.run.coins || previous.paused || previous.run.boss_state!=lab->run.boss_state)) {
        VDP_clearTextArea(3,10,34,9);
        centered("==========================",12);
        centered_color(lab->run.boss_state==2 ? "GAME CLEAR!" : "ROOM CLEAR!",14,PAL2);
        centered(lab->run.boss_state==2 ? "THE VAULT IS YOURS!" : "ON TO THE NEXT ROOM",16);
        centered("==========================",18);
    }
    for (u16 i=0;i<COIN_COUNT;++i) if (coins[i]) {
        SPR_setPosition(coins[i],lab->loose[i].x/8+4,(lab->loose[i].y-lab->loose[i].height)/8+4);
        SPR_setVisibility(coins[i],lab->run.coins & (1u<<i) ? VISIBLE : HIDDEN);
        SPR_setAnimAndFrame(coins[i],0,(lab->phase/8)%4);
    }
    if (boss) {
        SPR_setPosition(boss,lab->boss_x,lab->boss_y);
        u8 direction=lab->boss_direction;
        u16 row=direction==2 ? 0 : direction==6 ? 2 : 1;
        SPR_setAnimAndFrame(boss,row,lab->boss_walking ? lab->boss_animation/8 : 0);
        SPR_setHFlip(boss,direction>=3 && direction<=5);
        SPR_setVisibility(boss,lab->run.boss_state==0 ? VISIBLE : HIDDEN);
        SPR_setPosition(smoke,lab->boss_x,lab->boss_y);
        SPR_setAnimAndFrame(smoke,0,lab->boss_defeat_ticks<48 ? lab->boss_defeat_ticks/12 : 3);
        SPR_setVisibility(smoke,lab->run.boss_state==1 && lab->boss_defeat_ticks<48 ? VISIBLE : HIDDEN);
        SPR_setPosition(mega,lab->mega_x,lab->mega_y);
        SPR_setAnimAndFrame(mega,0,(lab->phase/8)%4);
        SPR_setVisibility(mega,lab->run.boss_state==1 && lab->boss_defeat_ticks>=48 ? VISIBLE : HIDDEN);
        for (u16 i=0;i<LASER_CAPACITY;++i) {
            const Laser *shot=&lab->lasers[i];
            static const u8 frame[]={0,1,2,1,0,1,2,1};
            SPR_setPosition(lasers[i],shot->x/8,shot->y/8);
            SPR_setAnimAndFrame(lasers[i],0,frame[shot->direction]);
            SPR_setHFlip(lasers[i],shot->direction>=3 && shot->direction<=5);
            SPR_setVFlip(lasers[i],shot->direction>=5);
            SPR_setVisibility(lasers[i],shot->active ? VISIBLE : HIDDEN);
        }
        if (!drawn || previous.boss_windup!=lab->boss_windup || previous.run.boss_state!=lab->run.boss_state) {
            field("",2,24,36);
            if (lab->run.boss_state==1) centered_color("BOSS DOWN! TAKE THE MEGA COIN",24,PAL2);
            else if (lab->boss_windup) centered_color("LASER CHARGING!",24,PAL2);
        }
    }
    if (lab->paused && (!drawn || !previous.paused || previous.menu!=lab->menu)) {
        static const char *const pause[]={"RESUME","TITLE / KEEP CHECKPOINT"};
        centered("PAUSED",10); choices(pause,2,lab->menu,13);
    }
}
void view_draw(const Lab *lab)
{
    if (current==SCENE_PLAY && drawn && lab->paused && previous.paused && previous.menu==lab->menu) return;
    if (current==SCENE_TITLE) {
        const char *labels[]={"START",lab->has_save ? "CONTINUE" : "CONTINUE (NO SAVE)","OPTIONS","CREDITS"};
        if (!drawn || previous.menu!=lab->menu || previous.has_save!=lab->has_save) choices(labels,4,lab->menu,14);
        if (!lab->has_save && lab->menu!=1) tint_field("CONTINUE (NO SAVE)",12,16,24,PAL1);
        SPR_setPosition(cursor,72,112+lab->menu*16);
        SPR_setAnimAndFrame(cursor,0,(lab->phase/8)%4);
    } else if (current==SCENE_CONTINUE) {
        static const char *const labels[]={"RESUME CHECKPOINT","BACK"};
        if (!drawn || previous.menu!=lab->menu) choices(labels,2,lab->menu,17);
    } else if (current==SCENE_OPTIONS) {
        const char *labels[]={lab->music ? "MUSIC: ON" : "MUSIC: OFF",lab->effects ? "SOUND: ON" : "SOUND: OFF","BACK"};
        if (!drawn || previous.menu!=lab->menu || previous.music!=lab->music || previous.effects!=lab->effects)
            choices(labels,3,lab->menu,11);
    } else if (current==SCENE_SELECT) select_draw(lab);
    else if (current==SCENE_PLAY) play_draw(lab);
    previous=*lab; drawn=true;
    SPR_update();
}
void view_status(const char *message,u32 changes)
{
    if (current!=SCENE_TITLE) return;
    field(message,2,22,36);
    field("SWITCHES",2,27,9); number(changes,11,27,5);
    field("HEAP FREE",21,27,10); number(MEM_getFree(),31,27,7);
}

void view_notice(const char *message) { field(message,2,current==SCENE_PLAY ? 24 : 22,36); }
