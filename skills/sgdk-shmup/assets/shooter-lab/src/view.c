#include <genesis.h>
#include "resources.h"
#include "view.h"

#define CACHE_TILES 160
static Sprite *ship,*boss,*enemies[ENEMY_COUNT],*shots[SHOT_COUNT],*bullets[BULLET_COUNT];
static Sprite *pickups[PICKUP_COUNT],*effects[EFFECT_COUNT];
static u16 ship_tiles[4],enemy_tiles[6],boss_tiles[3],bullet_tiles[3],pickup_tiles[2],effect_tiles[4];
static u16 frame;
static State old_state;
static bool old_pause,drawn;
static const char *error="UNKNOWN SETUP FAILURE";
const char *view_error(void) { return error; }
#define FAIL(reason) do { error=reason; return false; } while (0)

static bool cache(const SpriteDefinition *definition,u16 *indices,u16 length,u16 *tile)
{
    if (definition->numAnimation!=1 || definition->animations[0]->numFrame!=length) return false;
    u16 count;
    u16 **frames=SPR_loadAllFrames(definition,*tile,&count);
    if (!frames) return false;
    for (u16 i=0;i<length;i++) indices[i]=frames[0][i];
    MEM_free(frames); *tile+=count;
    return *tile<=TILE_FONT_INDEX;
}
static Sprite *make_sprite(const SpriteDefinition *definition,u16 palette,u16 tile,u16 depth)
{
    Sprite *sprite=SPR_addSpriteEx(definition,0,0,TILE_ATTR_FULL(palette,FALSE,FALSE,FALSE,tile),0);
    if (sprite)
    { SPR_setAutoAnimation(sprite,FALSE); SPR_setVisibility(sprite,HIDDEN); SPR_setDepth(sprite,depth); }
    return sprite;
}

bool view_start(void)
{
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    /* SGDK's 32x32 auto layout places Window at C800, which is not an
     * H40-aligned 4 KiB Window base. The 64x32 layout uses D000. */
    VDP_setPlaneSize(64,32,TRUE);
    if (!game_video_layout_valid(VDP_getPlaneWidth(),VDP_getPlaneHeight(),VDP_BG_A,VDP_BG_B,VDP_WINDOW))
        FAIL("VDP TABLES OVERLAP");
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    /* Horizontal Window positions are 16-pixel units. x=240 leaves a ten-tile
     * side HUD. A zero-height vertical window does not add a top/bottom strip. */
    VDP_setWindowHPos(TRUE,15); VDP_setWindowVPos(FALSE,0);
    VDP_setTextPlane(WINDOW); VDP_setTextPalette(PAL3); VDP_setTextPriority(TRUE);
    SPR_initEx(CACHE_TILES);
    if ((u32)TILE_USER_INDEX+stars_image.tileset->numTile+1>TILE_USER_MAX_INDEX) FAIL("BACKGROUND VRAM BUDGET");
    if (!VDP_drawImageEx(BG_B,&stars_image,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,TRUE)) FAIL("BACKGROUND UPLOAD");
    const u32 solid[8]={0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111};
    u16 panel=TILE_USER_INDEX+stars_image.tileset->numTile;
    VDP_loadTileData(solid,panel,1,CPU);
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,panel),30,0,10,28);
    PAL_setPalette(PAL0,stars_image.palette->data,CPU);
    PAL_setPalette(PAL1,ship_image.palette->data,CPU);
    PAL_setPalette(PAL2,enemy_image.palette->data,CPU);
    PAL_setPalette(PAL3,bullet_image.palette->data,CPU);
    VDP_setBackgroundColor(1);
    u16 tile=TILE_FONT_INDEX-CACHE_TILES;
    if (!cache(&ship_image,ship_tiles,4,&tile) || !cache(&enemy_image,enemy_tiles,6,&tile) ||
        !cache(&boss_image,boss_tiles,3,&tile) || !cache(&bullet_image,bullet_tiles,3,&tile) ||
        !cache(&pickup_image,pickup_tiles,2,&tile) || !cache(&effect_image,effect_tiles,4,&tile)) FAIL("FRAME CACHE RESERVATION");
    ship=make_sprite(&ship_image,PAL1,ship_tiles[0],0);
    boss=make_sprite(&boss_image,PAL2,boss_tiles[0],2);
    if (!ship || !boss) FAIL("PLAYER/BOSS SPRITE ALLOCATION");
    for (u16 i=0;i<BULLET_COUNT;i++)
    { bullets[i]=make_sprite(&bullet_image,PAL3,bullet_tiles[1],1); if (!bullets[i]) FAIL("HOSTILE SPRITE ALLOCATION"); }
    for (u16 i=0;i<SHOT_COUNT;i++)
    { shots[i]=make_sprite(&bullet_image,PAL3,bullet_tiles[0],3); if (!shots[i]) FAIL("PLAYER SHOT ALLOCATION"); }
    for (u16 i=0;i<ENEMY_COUNT;i++)
    { enemies[i]=make_sprite(&enemy_image,PAL2,enemy_tiles[0],2); if (!enemies[i]) FAIL("ENEMY SPRITE ALLOCATION"); }
    for (u16 i=0;i<PICKUP_COUNT;i++)
    { pickups[i]=make_sprite(&pickup_image,PAL3,pickup_tiles[0],3); if (!pickups[i]) FAIL("PICKUP SPRITE ALLOCATION"); }
    for (u16 i=0;i<EFFECT_COUNT;i++)
    { effects[i]=make_sprite(&effect_image,PAL3,effect_tiles[0],4); if (!effects[i]) FAIL("EFFECT SPRITE ALLOCATION"); }
    return true;
}

static void place(Sprite *sprite,s16 x,s16 y,u16 size,bool active,u16 frame,const u16 *tiles)
{
    x=x/UNIT-size/2; y=y/UNIT-size/2;
    /* Window's opaque, high-priority panel masks the visible part outside the
     * playfield. The simulation never admits player motion behind that panel. */
    active=active && x+size>0 && x<FIELD_W && y+size>0 && y<FIELD_H;
    SPR_setVisibility(sprite,active?VISIBLE:HIDDEN);
    if (!active) return;
    SPR_setPosition(sprite,x,y); SPR_setFrame(sprite,frame); SPR_setVRAMTileIndex(sprite,tiles[frame]);
}
static void text(const char *value,u16 row)
{
    char line[10]; u16 i=0;
    while (value[i] && i<8) { line[i]=value[i]; i++; }
    while (i<8) line[i++]=' ';
    line[8]=0;
    VDP_drawTextEx(WINDOW,line,TILE_ATTR(PAL3,TRUE,FALSE,FALSE),31,row,DMA_QUEUE);
}
static void hud(const Game *game,u16 hz,u16 fps)
{
    char line[40];
    text("SHOOTER",1); text("LAB",2);
    text("SCORE",4); uintToStr(game->score,line,8); text(line,5);
    text("TRIES",7);
    strcpy(line,"---"); for (u16 i=0;i<game->lives;i++) line[i]='*'; text(line,8);
    sprintf(line,"POWER %u",game->power); text(line,10);
    sprintf(line,"BOMBS %u",game->bombs); text(line,12);
    sprintf(line,"CHAIN %u",game->combo); text(line,14);
    if (game->state==BOSS)
    {
        text("BOSS",16); strcpy(line,"--------");
        u16 filled=(game->boss.hp*8+BOSS_HEALTH-1)/BOSS_HEALTH;
        for (u16 i=0;i<filled;i++) line[i]='=';
        text(line,17);
    }
    else { sprintf(line,"WAVE %u/%u",game->wave,WAVE_COUNT); text(line,16); text("",17); }
    text(game->paused?"PAUSED":game->state==CLEAR?"CLEAR!":game->state==OVER?"GAMEOVER":"",19);
    sprintf(line,"%uF %uHZ",fps,hz); text(line,21);
    sprintf(line,"LOAD %u",SYS_getCPULoad()); text(line,22);
    text("A FIRE",24); text("B BOMB",25); text("C FOCUS",26);
}
static void banner(const Game *game)
{
    if (game->state==TITLE)
    {
        VDP_drawText("S H O O T E R   L A B",4,10);
        VDP_drawText("A OR START TO LAUNCH",5,13);
        VDP_drawText("D-PAD MOVE  HOLD A FIRE",3,16);
        VDP_drawText("B BOMB     C SLOW / CORE",3,18);
    }
    else if (game->state==CLEAR || game->state==OVER)
    {
        VDP_drawText(game->state==CLEAR?"STAGE CLEAR!":" GAME OVER ",9,11);
        VDP_drawText("A / START: PLAY AGAIN",5,15);
    }
}
void view_frame(const Game *game,u16 held,u16 hz,u16 fps)
{
    bool changed=!drawn || game->state!=old_state;
    if (changed)
    {
        /* BG_A is a stationary message layer; stars scroll on BG_B. */
        VDP_setTextPlane(BG_A); VDP_clearTextArea(0,0,30,28); banner(game); VDP_setTextPlane(WINDOW);
    }
    VDP_setVerticalScroll(BG_B,-(s16)((game->ticks/2)&255));
    bool running=game->state==PLAY || game->state==BOSS;
    u16 phase=(game->ticks/5)&1;
    place(ship,game->x,game->y,16,running && (!game->invulnerable || !(game->ticks&4)),phase+((held&C)?2:0),ship_tiles);
    place(boss,game->boss.x,game->boss.y,32,running && game->boss.active,
          game->boss.hp<=BOSS_HEALTH/2?2:(game->ticks/12)&1,boss_tiles);
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        const Enemy *enemy=&game->enemies[i];
        place(enemies[i],enemy->x,enemy->y,16,running && enemy->active,enemy->kind*2+phase,enemy_tiles);
    }
    for (u16 i=0;i<SHOT_COUNT;i++) place(shots[i],game->shots[i].x,game->shots[i].y,8,running && game->shots[i].active,0,bullet_tiles);
    for (u16 i=0;i<BULLET_COUNT;i++) place(bullets[i],game->bullets[i].x,game->bullets[i].y,8,running && game->bullets[i].active,1,bullet_tiles);
    for (u16 i=0;i<PICKUP_COUNT;i++) place(pickups[i],game->pickups[i].x,game->pickups[i].y,16,running && game->pickups[i].active,game->pickups[i].kind,pickup_tiles);
    for (u16 i=0;i<EFFECT_COUNT;i++)
    {
        const Effect *effect=&game->effects[i];
        if (game->state==CLEAR)
        {
            u16 t=(game->phase_time+i*13)%90;
            place(effects[i],(120+((s16)i-3)*(s16)t/2)*UNIT,(180-3*(s16)t+t*t/30)*UNIT,16,t<75,t/24,effect_tiles);
        }
        else place(effects[i],effect->x,effect->y,16,running && effect->life,(24-effect->life)/6,effect_tiles);
    }
    /* Bounded flash changes one background color through queued palette work. */
    if (!drawn || changed || (frame&3)==0)
    {
        u16 color=game->bomb_flash && (game->bomb_flash&4)?RGB24_TO_VDPCOLOR(0x49496D):RGB24_TO_VDPCOLOR(0x000024);
        PAL_setColors(1,&color,1,DMA_QUEUE_COPY);
    }
    if (!(frame++%10) || changed || game->paused!=old_pause) hud(game,hz,fps);
    SPR_update(); old_state=game->state; old_pause=game->paused; drawn=true;
    VDP_setEnable(TRUE);
}
