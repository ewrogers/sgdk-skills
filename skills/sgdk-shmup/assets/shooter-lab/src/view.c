#include <genesis.h>
#include "resources.h"
#include "view.h"
#include "sprite_list.h"

#define CACHE_TILES FRAME_CACHE_TILES
static Actor actors[78];
static u16 actor_count;
static Actor *ship,*boss,*enemies[ENEMY_COUNT],*shots[SHOT_COUNT],*bullets[BULLET_COUNT];
static Actor *pickups[PICKUP_COUNT],*effects[EFFECT_COUNT];
static Actor *barrier,*boss_guard,*payload,*blast[8],*twins[2];
static u16 twin_tiles[2];
static u16 old_guard=65535;
static u16 ship_tiles[4],enemy_tiles[6],boss_tiles[7],bullet_tiles[8],reaver_tiles[7],pickup_tiles[4],effect_tiles[4],utility_tiles[4];

static bool clear_banner;
static u16 title_tile,hud_gold,hud_blue;
static const char hud_glyphs[]=" 0123456789POWERBMSCHAIN";
static u16 frame,life_tile,bar_tile,old_bar=65535;
static char hud_lines[28][9];
static u16 hud_style[28];
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
    for (u16 i=0;i<length;i++)
    {
        if ((definition->animations[0]->frames[i]->numSprite&0x7F)!=1)
        { MEM_free(frames); return false; }
        indices[i]=frames[0][i];
    }
    MEM_free(frames); *tile+=count;
    return *tile<=TILE_FONT_INDEX;
}
static Actor *make_actor(const SpriteDefinition *definition,u16 palette)
{
    if (actor_count>=78) return NULL;
    Actor *actor=&actors[actor_count++];
    *actor=(Actor){definition,palette};
    return actor;
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
    VDP_loadFont(&sci_font,DMA);
    SPR_initEx(CACHE_TILES);
    if ((u32)TILE_USER_INDEX+stars_image.tileset->numTile+5+2*(sizeof hud_glyphs-1)+title_image.tileset->numTile>TILE_USER_MAX_INDEX) FAIL("BACKGROUND VRAM BUDGET");
    if (!VDP_drawImageEx(BG_B,&stars_image,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,TRUE)) FAIL("BACKGROUND UPLOAD");
    if (!VDP_drawImageEx(BG_B,&stars_image,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),32,0,FALSE,TRUE)) FAIL("TITLE STARFIELD UPLOAD");
    hud_gold=TILE_USER_INDEX+stars_image.tileset->numTile+5;
    hud_blue=hud_gold+sizeof hud_glyphs-1;
    title_tile=hud_blue+sizeof hud_glyphs-1;
    /* Recolor only the HUD's small alphabet, not another complete font. */
    for (u16 ink=0;ink<2;ink++)
        for (u16 glyph=0;glyph<sizeof hud_glyphs-1;glyph++)
        {
            u32 rows[8];
            for (u16 row=0;row<8;row++)
                rows[row]=(sci_font.tiles[(hud_glyphs[glyph]-32)*8+row]&0x11111111UL)*(ink?13:10);
            VDP_loadTileData(rows,(ink?hud_blue:hud_gold)+glyph,1,CPU);
        }
    const u32 solid[8]={0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111};
    u16 panel=TILE_USER_INDEX+stars_image.tileset->numTile;
    VDP_loadTileData(solid,panel,1,CPU);
    /* Resident HUD patterns use existing palettes and no hardware sprites. */
    const u32 icons[32]={
        0x00066000,0x00066000,0x006EE600,0x006EE600,
        0x06EEEE60,0x6EEEEEE6,0x66066066,0x00066000,
        0x11111111,0x10000001,0x10000001,0x10000001,
        0x10000001,0x10000001,0x10000001,0x11111111,
        0x11111111,0x18888881,0x18888881,0x18888881,
        0x18888881,0x18888881,0x18888881,0x11111111,
        0x11111111,0x1DDDDDD1,0x1DDDDDD1,0x1DDDDDD1,
        0x1DDDDDD1,0x1DDDDDD1,0x1DDDDDD1,0x11111111};
    life_tile=panel+1; bar_tile=panel+2;
    VDP_loadTileData(icons,life_tile,4,CPU);
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,panel),30,0,10,28);
    PAL_setPalette(PAL0,stars_image.palette->data,CPU);
    PAL_setPalette(PAL1,ship_image.palette->data,CPU);
    PAL_setPalette(PAL2,enemy_image.palette->data,CPU);
    PAL_setPalette(PAL3,bullet_image.palette->data,CPU);
    VDP_setBackgroundColor(1);
    u16 tile=TILE_FONT_INDEX-CACHE_TILES;
    if (!cache(&ship_image,ship_tiles,4,&tile) || !cache(&enemy_image,enemy_tiles,6,&tile) ||
        !cache(&boss_image,boss_tiles,7,&tile) || !cache(&reaver_image,reaver_tiles,7,&tile) || !cache(&bullet_image,bullet_tiles,8,&tile) ||
        !cache(&pickup_image,pickup_tiles,4,&tile) || !cache(&effect_image,effect_tiles,4,&tile) || !cache(&utility_image,utility_tiles,4,&tile) || !cache(&twin_image,twin_tiles,2,&tile)) FAIL("FRAME CACHE RESERVATION");
    ship=make_actor(&ship_image,PAL1);
    boss=make_actor(&boss_image,PAL2);
    if (!ship || !boss) FAIL("PLAYER/BOSS SPRITE ALLOCATION");
    for (u16 i=0;i<BULLET_COUNT;i++)
    { bullets[i]=make_actor(&bullet_image,PAL3); if (!bullets[i]) FAIL("HOSTILE SPRITE ALLOCATION"); }
    for (u16 i=0;i<SHOT_COUNT;i++)
    { shots[i]=make_actor(&bullet_image,PAL3); if (!shots[i]) FAIL("PLAYER SHOT ALLOCATION"); }
    for (u16 i=0;i<ENEMY_COUNT;i++)
    { enemies[i]=make_actor(&enemy_image,PAL2); if (!enemies[i]) FAIL("ENEMY SPRITE ALLOCATION"); }
    for (u16 i=0;i<PICKUP_COUNT;i++)
    { pickups[i]=make_actor(&pickup_image,PAL3); if (!pickups[i]) FAIL("PICKUP SPRITE ALLOCATION"); }
    for (u16 i=0;i<EFFECT_COUNT;i++)
    { effects[i]=make_actor(&effect_image,PAL3); if (!effects[i]) FAIL("EFFECT SPRITE ALLOCATION"); }
    for (u16 i=0;i<2;i++)
    { twins[i]=make_actor(&twin_image,PAL1); if (!twins[i]) FAIL("TWIN SPRITE ALLOCATION"); }
    boss_guard=make_actor(&utility_image,PAL3);
    barrier=make_actor(&utility_image,PAL3);
    payload=make_actor(&utility_image,PAL3);
    if (!boss_guard || !barrier || !payload) FAIL("UTILITY SPRITE ALLOCATION");
    for (u16 i=0;i<8;i++)
    { blast[i]=make_actor(&effect_image,PAL3); if (!blast[i]) FAIL("BLAST SPRITE ALLOCATION"); }
    return true;
}

static void text_ink(const char *value,u16 row,u16 base,u16 palette)
{
    char line[10]; u16 i=0;
    while (value[i] && i<8) { line[i]=value[i]; i++; }
    while (i<8) line[i++]=' ';
    line[8]=0;
    u16 style=TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,base);
    if (!strcmp(hud_lines[row],line) && hud_style[row]==style) return;
    hud_style[row]=style;
    strcpy(hud_lines[row],line);
    u16 tiles[8];
    for (u16 n=0;n<8;n++)
    {
        u16 index=line[n]-32;
        if (base!=TILE_FONT_INDEX)
        {
            index=0;
            while (hud_glyphs[index] && hud_glyphs[index]!=line[n]) index++;
            if (!hud_glyphs[index]) index=0;
        }
        tiles[n]=TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,base+index);
    }
    VDP_setTileMapDataRowEx(WINDOW,tiles,0,row,31,8,DMA_QUEUE_COPY);
}
static void text(const char *value,u16 row) { text_ink(value,row,TILE_FONT_INDEX,PAL3); }

static void hud(const Game *game,u16 hz,u16 fps)
{
    char line[40];
    text("SHOOTER",1); sprintf(line,"STAGE %u",game->stage); text(line,2);
    text("SCORE",4); uintToStr(game->score,line,8); text(line,5);
    text("LIVES",7);
    u16 icons[8];
    for (u16 i=0;i<8;i++) icons[i]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,life_tile-1);
    for (u16 i=0;i<game->lives && i<3;i++) icons[i*3]=TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,life_tile);
    VDP_setTileMapDataRowEx(WINDOW,icons,0,8,31,8,DMA_QUEUE_COPY);
    sprintf(line,"POWER %u",game->power); text_ink(line,10,hud_gold,PAL3);
    sprintf(line,"BOMBS %u",game->bombs); text_ink(line,12,hud_blue,PAL3);
    sprintf(line,"CHAIN %u",game->combo); text_ink(line,14,hud_blue,PAL2);
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
        VDP_drawImageEx(BG_A,&title_image,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,title_tile),6,5,FALSE,TRUE);
        VDP_drawText("S H O O T E R   L A B",10,2);
        VDP_drawText("> START <",16,22);
        VDP_drawText("A / START TO LAUNCH",11,25);
    }
    else if (game->state==OVER)
    {
        VDP_drawText(game->state==CLEAR?"STAGE CLEAR!":" GAME OVER ",9,11);
        VDP_drawText("A / START: PLAY AGAIN",5,15);
    }
}
/* A plane bar leaves the sprite budget available to bullets. Upload only
 * changed tiles; temporary row storage is copied into the VBlank queue. */
static void boss_bar(const Game *game)
{
    if (game->state!=BOSS) { old_bar=65535; old_guard=65535; return; }
    u16 age=game->boss.age;
    if (age<BOSS_WARNING_TICKS+BOSS_ENTRY_TICKS) return;
    u16 reveal=age-(BOSS_WARNING_TICKS+BOSS_ENTRY_TICKS);
    u16 width=reveal<BOSS_BAR_TICKS?reveal*20/BOSS_BAR_TICKS:20;
    u16 fill=(game->boss.hp*20+BOSS_HEALTH-1)/BOSS_HEALTH;
    if (game->stage==2 && age>=BOSS_READY_TICKS && old_guard!=(u16)game_boss_guarded(game))
    {
        old_guard=game_boss_guarded(game);
        VDP_drawTextEx(BG_A,old_guard?"SHIELD UP":" EXPOSED ",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),11,3,DMA_QUEUE);
    }
    u16 key=width*32+fill;
    if (key==old_bar) return;
    if (old_bar==65534)
        VDP_drawTextEx(BG_A,game->stage==2?"AEGIS REAVER":"VOID WARDEN",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),10,1,DMA_QUEUE);
    u16 tiles[20];
    for (u16 i=0;i<20;i++)
        tiles[i]=(i<(20-width)/2 || i>=(20+width)/2)?0:
            TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,bar_tile+(i<fill?(game->boss.hp<=BOSS_HEALTH/2?2:1):0));
    VDP_setTileMapDataRowEx(BG_A,tiles,0,2,5,20,DMA_QUEUE_COPY);
    old_bar=key;
}

void view_frame(const Game *game,u16 held,u16 hz,u16 fps)
{
    sprite_list_begin(game->state==TITLE?320:FIELD_W);
    boss->definition=game->stage==2?&reaver_image:&boss_image;
    const u16 *current_boss_tiles=game->stage==2?reaver_tiles:boss_tiles;
    bool changed=!drawn || game->state!=old_state;
    if (changed)
    {
        clear_banner=false;
        for (u16 i=0;i<BULLET_COUNT;i++) bullets[i]->palette=game->state==TITLE?PAL1:PAL3;
        /* BG_A is a stationary message layer; stars scroll on BG_B. */
        VDP_setTextPlane(BG_A); VDP_clearTextArea(0,0,40,28);
        VDP_setWindowHPos(game->state!=TITLE,game->state==TITLE?0:15);
        banner(game); VDP_setTextPlane(WINDOW);
    }
    if (game->state==BOSS)
    {
        if (game->boss.age<BOSS_WARNING_TICKS)
        {
            if (changed) VDP_drawTextEx(BG_A,"BOSS INCOMING",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),9,9,DMA_QUEUE);
        }
        else if (game->boss.age>=BOSS_WARNING_TICKS && old_bar==65535)
        {
            VDP_drawTextEx(BG_A,"             ",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),9,9,DMA_QUEUE);
            old_bar=65534;
        }
    }
    boss_bar(game);
    if (game->state==CLEAR && game->phase_time>=BOSS_DEATH_TICKS && !clear_banner)
    {
        VDP_drawTextEx(BG_A,"STAGE CLEAR!",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),9,11,DMA_QUEUE);
        VDP_drawTextEx(BG_A,game->stage==1?"A / START: NEXT STAGE":"A / START: PLAY AGAIN",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),5,15,DMA_QUEUE);
        clear_banner=true;
    }
    if (game->state==TITLE && frame%30==0)
        VDP_drawTextEx(BG_A,(frame/30)&1?"  START  ":"> START <",TILE_ATTR(PAL3,TRUE,FALSE,FALSE),16,22,DMA_QUEUE);
    VDP_setVerticalScroll(BG_B,-(s16)((game->state==TITLE?frame/3:game->ticks/2)&255));
    bool running=game->state==PLAY || game->state==BOSS;
    u16 phase=(game->ticks/5)&1;
    sprite_list_place(ship,game->x,game->y,16,running && (!game->invulnerable || !(game->ticks&4)),phase+((held&C)?2:0),ship_tiles);
    /* Player first, then damaging bullets, before decorative blast effects. */
    for (u16 i=0;i<BULLET_COUNT;i++)
        if (game->state==TITLE)
        {
            s16 x=(s16)((i*73+frame/(1+i%3))%336)-8;
            s16 y=(s16)((i*47+frame/(1+i%2))%240)-8;
            sprite_list_place(bullets[i],x*UNIT,y*UNIT,8,true,3+(i&1),bullet_tiles);
        }
        else sprite_list_place(bullets[i],game->bullets[i].x,game->bullets[i].y,8,running && game->bullets[i].active,game->bullets[i].kind?4+game->bullets[i].kind:1,bullet_tiles);
    if (game->state==CLEAR)
    {
        u16 time=game->phase_time;
        u16 dying=time<30?(time/6)&1:time<60?3:time<90?4:5+((time/12)&1);
        sprite_list_place(boss,game->boss.x,game->boss.y+(time>=90?((s16)((time/8)%8)<4?(s16)((time/8)%8):(s16)(7-(time/8)%8))*UNIT:0),32,time>=30 || (time&3)!=0,dying,current_boss_tiles);
    }
    else sprite_list_place(boss,game->boss.x,game->boss.y,32,running && game->boss.active && game->boss.age>=BOSS_WARNING_TICKS,
          game->boss.hp<=BOSS_HEALTH/2?2:(game->ticks/12)&1,current_boss_tiles);
    sprite_list_place(boss_guard,game->boss.x,game->boss.y,32,running && game_boss_guarded(game),phase,utility_tiles);
    for (u16 i=0;i<2;i++)
    {
        s16 x=0,y=0;
        if (i<game->twins) game_twin_point(game,i,&x,&y);
        sprite_list_place(twins[i],x,y,16,running && i<game->twins,phase,twin_tiles);
    }
    sprite_list_place(barrier,game->x,game->y,32,running && game->shield &&
        (game->shield_time>120 || !(game->ticks&8)),phase,utility_tiles);
    sprite_list_place(payload,game->payload.x,game->payload.y,32,running && game->payload.active &&
        game->payload.age<BOMB_ARM_TICKS,2+((game->payload.age/5)&1),utility_tiles);
    for (u16 i=0;i<8;i++)
    {
        bool expanding=running && game->payload.active && game->payload.radius>=20;
        s16 x=0,y=0;
        if (expanding) game_blast_point(game,i,&x,&y);
        sprite_list_place(blast[i],x,y,16,expanding,1+((game->payload.age/3)&1),effect_tiles);
    }
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        const Enemy *enemy=&game->enemies[i];
        sprite_list_place(enemies[i],enemy->x,enemy->y,16,running && enemy->active,enemy->kind*2+phase,enemy_tiles);
    }
    for (u16 i=0;i<SHOT_COUNT;i++) sprite_list_place(shots[i],game->shots[i].x,game->shots[i].y,8,running && game->shots[i].active,0,bullet_tiles);
    for (u16 i=0;i<PICKUP_COUNT;i++)
    {
        pickups[i]->palette=game->pickups[i].kind==3?PAL2:PAL3;
        sprite_list_place(pickups[i],game->pickups[i].x,game->pickups[i].y,16,running && game->pickups[i].active,game->pickups[i].kind,pickup_tiles);
    }
    for (u16 i=0;i<EFFECT_COUNT;i++)
    {
        const Effect *effect=&game->effects[i];
        if (game->state==CLEAR)
        {
            if (game->phase_time<BOSS_DEATH_TICKS)
            {
                u16 t=(game->phase_time+i*11)%24;
                sprite_list_place(effects[i],game->boss.x+((s16)(i%3)-1)*12*UNIT,
                    game->boss.y+((s16)(i/3)*2-1)*10*UNIT,16,true,t/6,effect_tiles);
                continue;
            }
            u16 t=(game->phase_time+i*13)%90;
            sprite_list_place(effects[i],(120+((s16)i-3)*(s16)t/2)*UNIT,(180-3*(s16)t+t*t/30)*UNIT,16,t<75,t/24,effect_tiles);
        }
        else sprite_list_place(effects[i],effect->x,effect->y,16,running && effect->life,(24-effect->life)/6,effect_tiles);
    }
    /* Bounded flash changes one background color through queued palette work. */
    if (!drawn || changed || (frame&3)==0)
    {
        u16 color=game->bomb_flash && (game->bomb_flash&4)?RGB24_TO_VDPCOLOR(0x49496D):game->stage==2?RGB24_TO_VDPCOLOR(0x240024):RGB24_TO_VDPCOLOR(0x000024);
        PAL_setColors(1,&color,1,DMA_QUEUE_COPY);
    }
    if (game->state!=TITLE && (!(frame%10) || changed || game->paused!=old_pause)) hud(game,hz,fps);
    frame++;
    sprite_list_end();
    old_state=game->state; old_pause=game->paused; drawn=true;
    VDP_setEnable(TRUE);
}
