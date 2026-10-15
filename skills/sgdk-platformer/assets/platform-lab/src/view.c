#include <genesis.h>
#include "resources.h"
#include "view.h"

#define CACHE_TILES 192
static Map *map;
static Game *world;
static Sprite *hero,*enemies[ENEMY_COUNT],*items[PICKUP_COUNT],*platform[2],*flags[3],*boots,*sparks;
static u16 hero_tiles[10],enemy_tiles[20],item_tiles[13];
static u16 revision,display_frame;
static bool drawn,old_pause,old_complete;

/* MAP patches streamed hardware 8x8 entries, not Tiled's logical 16x16 cells.
 * Source buffers belong to MAP and stay alive until its queued DMA drains. */
static void patch(Map *unused,u16 *buffer,u16 x,u16 y,MapUpdateType type,u16 size)
{
    (void)unused;
    for (u16 i=0;i<size;i++)
    {
        s16 column=x/2,row=y/2;
        Tile original=game_original_tile(column,row);
        if ((original==COVER && world->secret) ||
            (original==BREAKABLE && game_tile(world,column,row)==EMPTY)) buffer[i]=0;
        if (type==ROW_UPDATE) x++; else y++;
    }
}

static bool cache(const SpriteDefinition *definition,u16 *indices,u16 frames,u16 *index)
{
    if (definition->numAnimation!=1 || definition->animations[0]->numFrame!=frames) return false;
    u16 count;
    u16 **result=SPR_loadAllFrames(definition,*index,&count);
    if (!result) return false;
    for (u16 i=0;i<frames;i++) indices[i]=result[0][i];
    MEM_free(result);
    *index+=count;
    return *index<=TILE_FONT_INDEX;
}

static Sprite *sprite(const SpriteDefinition *definition,u16 palette,u16 tile)
{
    Sprite *result=SPR_addSpriteEx(definition,0,0,TILE_ATTR_FULL(palette,FALSE,FALSE,FALSE,tile),0);
    if (result)
    {
        SPR_setAutoAnimation(result,FALSE);
        SPR_setVisibility(result,HIDDEN);
    }
    return result;
}

bool view_start(Game *game)
{
    world=game;
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setWindowHPos(FALSE,0); VDP_setWindowVPos(FALSE,4);
    VDP_setTextPlane(WINDOW); VDP_setTextPalette(PAL3); VDP_setTextPriority(TRUE);
    VDP_setBackgroundColor(0);
    SPR_initEx(CACHE_TILES);
    if ((u32)TILE_USER_INDEX+course_map_tileset0.numTile>TILE_USER_MAX_INDEX) return false;
    if (!VDP_loadTileSet(&course_map_tileset0,TILE_USER_INDEX,DMA)) return false;
    PAL_setPalette(PAL0,terrain_palette.data,CPU);
    PAL_setPalette(PAL1,hero_image.palette->data,CPU);
    PAL_setPalette(PAL2,enemy_image.palette->data,CPU);
    PAL_setPalette(PAL3,palette_grey,CPU);
    PAL_setColor(0,RGB24_TO_VDPCOLOR(0x242424));
    PAL_setColor(63,RGB24_TO_VDPCOLOR(0x92DBFF));
    u16 index=TILE_FONT_INDEX-CACHE_TILES;
    if (!cache(&hero_image,hero_tiles,10,&index) ||
        !cache(&enemy_image,enemy_tiles,20,&index) || !cache(&item_image,item_tiles,13,&index)) return false;
    map=MAP_create(&course_map,BG_A,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX));
    if (!map) return false;
    MAP_setDataPatchCallback(map,patch);
    hero=sprite(&hero_image,PAL1,hero_tiles[0]); if (!hero) return false;
    for (u16 i=0;i<ENEMY_COUNT;i++)
    { enemies[i]=sprite(&enemy_image,PAL2,enemy_tiles[i*4]); if (!enemies[i]) return false; }
    for (u16 i=0;i<PICKUP_COUNT;i++)
    { items[i]=sprite(&item_image,PAL2,item_tiles[game->pickups[i].kind]); if (!items[i]) return false; }
    for (u16 i=0;i<2;i++)
    { platform[i]=sprite(&item_image,PAL2,item_tiles[6]); if (!platform[i]) return false; }
    for (u16 i=0;i<3;i++)
    { flags[i]=sprite(&item_image,PAL2,item_tiles[7]); if (!flags[i]) return false; }
    boots=sprite(&item_image,PAL2,item_tiles[9]);
    sparks=sprite(&item_image,PAL2,item_tiles[11]);
    if (boots) SPR_setDepth(boots,0);
    if (sparks) SPR_setDepth(sparks,0);
    return boots && sparks;
}

static void place(Sprite *sprite,s16 x,s16 y,u16 width,u16 height,bool visible,
                  u16 frame,const u16 *tiles,bool flip)
{
    x-=world->camera_x; y-=world->camera_y;
    visible=visible && x+width>0 && x<320 && y+height>32 && y<224;
    SPR_setVisibility(sprite,visible?VISIBLE:HIDDEN);
    if (!visible) return;
    SPR_setPosition(sprite,x,y);
    SPR_setHFlip(sprite,flip);
    SPR_setFrame(sprite,frame);
    SPR_setVRAMTileIndex(sprite,tiles[frame]);
}

static void line(char *text,u16 row)
{
    u16 size=strlen(text);
    while (size<38) text[size++]=' ';
    text[38]=0;
    VDP_drawTextEx(WINDOW,text,TILE_ATTR(PAL3,TRUE,FALSE,FALSE),1,row,DMA_QUEUE);
}

static void hud(const Game *game,u16 hz,u16 fps)
{
    static const char *areas[]={"JUMP / STOMP","BOOTS / WALL JUMP","CHARGE / RIDE","SECRET / GOAL"};
    char text[80];
    sprintf(text,"PLATFORM LAB   HP %u  COINS %u",game->player.hearts,game->score); line(text,0);
    sprintf(text,"%s  %uFPS %uHZ",game->complete?"CLEAR! START:NEW RUN":game->paused?"PAUSED":areas[game->checkpoint],fps,hz); line(text,1);
    sprintf(text,"A:JUMP B:HOLD/RELEASE CHARGE C:RESET"); line(text,2);
    sprintf(text,"BOOTS:%s CHARGE:%u/36 LOAD:%u/100",game->player.boots?"ON ":"OFF",game->player.charge,SYS_getCPULoad()); line(text,3);
}

void view_draw(Game *game,u16 hz,u16 fps)
{
    bool loading=!drawn || game->teleport;
    if (old_complete!=game->complete)
    {
        VDP_setWindowVPos(FALSE,game->complete?12:4);
        char text[80];
        for (u16 row=4;row<12;row++) { text[0]=0; line(text,row); }
        if (game->complete)
        {
            strcpy(text,"       ========================"); line(text,5);
            strcpy(text,"          C O U R S E  C L E A R"); line(text,7);
            strcpy(text,"          START: PLAY AGAIN"); line(text,9);
            strcpy(text,"       ========================"); line(text,11);
        }
    }
    if (loading) VDP_setEnable(FALSE);
    /* A terrain edit refreshes the plane once. Normal camera movement streams
     * only new rows/columns. Queue work is consumed before the next edit. */
    MAP_scrollToEx(map,game->camera_x,game->camera_y,loading || revision!=game->terrain_revision);
    revision=game->terrain_revision;
    Player *p=&game->player;
    u16 frame=p->dashing?8+(game->ticks/3&1):!p->body.ground?(p->body.wall?7:6):
        p->body.vx?2+(game->ticks/6&3):(game->ticks/24&1);
    bool visible=!p->invulnerable || !(game->ticks&4);
    place(hero,p->body.x/UNIT-2,p->body.y/UNIT-3,16,24,visible,frame,hero_tiles,p->facing<0);
    s16 step=frame==2?-1:frame==4?1:0;
    place(boots,p->body.x/UNIT-2,p->body.y/UNIT+6,16,16,visible && p->boots,
          9+step,item_tiles,p->facing<0);
    place(sparks,p->body.x/UNIT-2,p->body.y/UNIT+2,16,16,
          visible && p->charge>0 && (game->ticks&2),p->charge==CHARGE_TICKS && p->air_dash?12:11,item_tiles,false);
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *e=&game->enemies[i];
        u16 phase=(game->ticks/10)&1;
        if (i==ARMOR && !e->guarded) phase+=2;
        place(enemies[i],e->body.x/UNIT-2,e->body.y/UNIT-2,16,16,e->active,
              i*4+phase,enemy_tiles,e->facing<0);
    }
    for (u16 i=0;i<PICKUP_COUNT;i++)
    {
        Pickup *p=&game->pickups[i];
        if (game->complete)
        {
            u16 phase=(game->celebration+i*17)%90;
            s16 x=game->camera_x+160+((s16)i-5)*(s16)phase/3;
            s16 y=game->camera_y+200-3*(s16)phase+(s16)(phase*phase/30);
            place(items[i],x,y,16,16,phase<75,3,item_tiles,false);
        }
        else place(items[i],p->x,p->y,16,16,!p->collected && (p->kind!=4 || game->secret),p->kind,item_tiles,false);
    }
    for (u16 i=0;i<2;i++)
        place(platform[i],game->platform.x/UNIT+i*16,game->platform.y/UNIT,16,16,true,6,item_tiles,false);
    const s16 xs[]={448,704,976},ys[]={192,240,240};
    for (u16 i=0;i<3;i++) place(flags[i],xs[i],ys[i],16,16,true,7,item_tiles,false);
    if (!(display_frame++%6) || loading || old_complete!=game->complete || old_pause!=game->paused) hud(game,hz,fps);
    old_pause=game->paused; old_complete=game->complete;
    SPR_update(); drawn=true;
    if (loading) { SYS_doVBlankProcess(); VDP_setEnable(TRUE); }
}
