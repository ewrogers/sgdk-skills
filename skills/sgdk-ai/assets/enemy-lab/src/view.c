#include <genesis.h>
#include "view.h"
#include "world.h"
#include "text.h"
#include "resources.h"

static Sprite *players[2],*enemies[ENEMY_COUNT],*shots[SHOT_COUNT],*pulses[2],*markers[ROUTE_CAPACITY];
static Sprite *status[ENEMY_COUNT];
static Sprite *tries[3],*exit_sprite;
static u16 last_level=0xFFFF,last_phase=0xFFFF;
static u16 last_selected=0xFFFF,last_state=0xFFFF,last_overlay=0xFFFF,last_pause=0xFFFF;
static u16 last_expanded=0xFFFF;
static u16 last_charge[2]={0xFFFF,0xFFFF};
static const u32 floor_tile[8]={0x22222222,0x22222222,0x22222222,0x22222222,
                               0x22222222,0x22222222,0x22222222,0x22222223};
static const u32 wall_tile[8]={0x66666666,0x65555554,0x65555554,0x65555554,
                              0x65555554,0x65555554,0x65555554,0x44444444};
static const u32 hud_tiles[5][8]={
    {0x99999999,0x99999999,0x99999999,0x99999999,0x99999999,0x99999999,0x99999999,0x99999999},
    {0xAAAAAAAA,0xAAAAAAAA,0x99999999,0x99999999,0x99999999,0x99999999,0xBBBBBBBB,0xAAAAAAAA},
    {0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC},
    {0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB},
    {0xAAAAAAAA,0xA999999A,0xA999999A,0xA999999A,0xA999999A,0xA999999A,0xA999999A,0xAAAAAAAA}
};
static const u32 extra_tiles[3][8]={
    {0xBBBBBBBB,0xB222222B,0xB222222B,0xB222222B,0xB222222B,0xB222222B,0xB222222B,0xBBBBBBBB},
    {0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB,0xBBBBBBBB},
    {0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD,0xDDDDDDDD}
};
static void field(const char *value,u16 x,u16 y,u16 width)
{
    char buffer[41]; if (x+width<=40 && text_field(buffer,sizeof buffer,value,width)) VDP_drawText(buffer,x,y);
}
static void number(u16 value,u16 x,u16 y,u16 width)
{
    char buffer[11]; if (text_number(buffer,sizeof buffer,value,width)) VDP_drawText(buffer,x,y);
}
static Sprite *sprite(const SpriteDefinition *definition,u16 palette)
{
    Sprite *result=SPR_addSprite(definition,0,0,TILE_ATTR(palette,FALSE,FALSE,FALSE));
    if (result) { SPR_setAutoAnimation(result,FALSE); SPR_setVisibility(result,HIDDEN); }
    return result;
}
bool view_init(void)
{
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE); VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3); VDP_setTextPriority(TRUE);
    VDP_setBackgroundColor(1);
    u16 colors[64];
    for (u16 p=0;p<4;++p) for (u16 i=0;i<16;++i) colors[p*16+i]=hero_image.palette->data[i];
    colors[1]=RGB24_TO_VDPCOLOR(0x101018);
    colors[2]=colors[18]=colors[34]=colors[50]=RGB24_TO_VDPCOLOR(0x202028);
    colors[51]=RGB24_TO_VDPCOLOR(0x303038); colors[52]=RGB24_TO_VDPCOLOR(0x181820);
    colors[53]=RGB24_TO_VDPCOLOR(0x505058); colors[54]=RGB24_TO_VDPCOLOR(0x888890);
    colors[7]=RGB24_TO_VDPCOLOR(0x245580); colors[8]=RGB24_TO_VDPCOLOR(0x88CCFF);
    colors[23]=RGB24_TO_VDPCOLOR(0x805020); colors[24]=RGB24_TO_VDPCOLOR(0xFFCC66);
    colors[39]=RGB24_TO_VDPCOLOR(0x802424); colors[40]=RGB24_TO_VDPCOLOR(0xFF8877);
    colors[55]=RGB24_TO_VDPCOLOR(0x503080); colors[56]=RGB24_TO_VDPCOLOR(0xCC99FF);
    colors[57]=RGB24_TO_VDPCOLOR(0x101820); colors[58]=RGB24_TO_VDPCOLOR(0x304860);
    colors[59]=RGB24_TO_VDPCOLOR(0x6699CC); colors[60]=RGB24_TO_VDPCOLOR(0xFF6655);
    colors[61]=RGB24_TO_VDPCOLOR(0x66EE99); colors[62]=RGB24_TO_VDPCOLOR(0xFFCC66);
    for (u16 p=0;p<4;++p) colors[p*16+15]=RGB24_TO_VDPCOLOR(0xEEEEEE);
    PAL_setColors(0,colors,64,CPU);
    VDP_loadTileData(floor_tile,TILE_USER_INDEX,1,DMA);
    VDP_loadTileData(wall_tile,TILE_USER_INDEX+1,1,DMA);
    VDP_loadTileData(&hud_tiles[0][0],TILE_USER_INDEX+2,5,DMA);
    VDP_loadTileData(&extra_tiles[0][0],TILE_USER_INDEX+7,3,DMA);
    VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+2),0,0,40,8);
    VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+3),0,7,40,1);
    SPR_initEx(8*hero_image.maxNumTile+SHOT_COUNT*shot_image.maxNumTile+
               2*pulse_image.maxNumTile+ROUTE_CAPACITY*marker_image.maxNumTile+ENEMY_COUNT*status_image.maxNumTile+
               3*tries_image.maxNumTile+exit_image.maxNumTile);
    if (!SPR_isInitialized()) return false;
    for (u16 p=0;p<2;++p) {
        players[p]=sprite(&hero_image,PAL0); pulses[p]=sprite(&pulse_image,PAL0);
        if (!players[p] || !pulses[p]) return false;
    }
    for (u16 i=0;i<ENEMY_COUNT;++i) {
        enemies[i]=sprite(&hero_image,1+i/2); status[i]=sprite(&status_image,1+i/2);
        if (!enemies[i] || !status[i]) return false;
    }
    for (u16 i=0;i<SHOT_COUNT;++i) { shots[i]=sprite(&shot_image,PAL3); if (!shots[i]) return false; }
    for (u16 i=0;i<ROUTE_CAPACITY;++i) { markers[i]=sprite(&marker_image,PAL3); if (!markers[i]) return false; }
    for (u16 i=0;i<3;++i) { tries[i]=sprite(&tries_image,PAL3); if (!tries[i]) return false; }
    exit_sprite=sprite(&exit_image,PAL3); if (!exit_sprite) return false;
    field("ENEMY LAB",2,1,12); field("AREA",27,1,5);
    field("TRIES",2,3,5); field("ALERT",20,3,6);
    field("REACH EXIT   J STUN   START PAUSE",2,5,36);
    field("STUN",2,6,4);
    SPR_update(); SYS_doVBlankProcess(); VDP_setEnable(TRUE); return true;
}
static void robot(Sprite *sprite,Point point,u8 facing,u16 animation,bool moving,bool visible)
{
    SPR_setPosition(sprite,point.x,point.y);
    SPR_setAnimAndFrame(sprite,facing==3 ? 1 : facing,moving ? animation/8 : 4+animation/24);
    SPR_setHFlip(sprite,facing==3); SPR_setVisibility(sprite,visible ? VISIBLE : HIDDEN);
}
static u16 nearest(const Game *game)
{
    u16 result=0; u32 best=0xFFFFFFFFUL;
    for (u16 i=0;i<ENEMY_COUNT;++i) {
        s16 dx=game->enemies[i].pos.x-game->players[0].pos.x,dy=game->enemies[i].pos.y-game->players[0].pos.y;
        u32 distance=(long)dx*dx+(long)dy*dy;
        if (distance<best) { best=distance; result=i; }
    }
    return result;
}
static void overlay(const Game *game,u16 selected)
{
    const Enemy *enemy=&game->enemies[selected];
    static const s16 cx[16]={256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98,0,98,181,237};
    static const s16 cy[16]={0,98,181,237,256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98};
    for (u16 i=0;i<ROUTE_CAPACITY;++i) {
        Point point={0}; bool visible=false;
        if (game->overlay==1) {
            s16 dx=cx[i]*enemy_range(enemy->kind)/256,dy=cy[i]*enemy_range(enemy->kind)/256;
            point=(Point){enemy->pos.x+4+dx,enemy->pos.y+4+dy};
            visible=point.x>=16 && point.x<296 && point.y>=64 && point.y<200 &&
                (enemy->kind!=GUARD || enemy_cone(enemy->facing,dx,dy));
        } else if (game->overlay==2 && enemy->route_at+i<enemy->route_len) {
            point=world_point(enemy->route[enemy->route_at+i]); point.x+=4; point.y+=4; visible=true;
        }
        SPR_setPosition(markers[i],point.x,point.y); SPR_setPalette(markers[i],1+enemy->kind);
        SPR_setVisibility(markers[i],visible ? VISIBLE : HIDDEN);
    }
}
void view_draw(const Game *game)
{
    /* Text replaces whole tiles on A. Keep the opaque panel underneath on B,
       at high priority so low-priority actors cannot cover its lettering. */
    if (last_level!=game->level || (last_phase!=game->phase && game->phase==PLAYING)) {
        u16 border=TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+1);
        VDP_fillTileMapRect(BG_B,border,1,8,1,18);
        VDP_fillTileMapRect(BG_B,border,38,8,1,18);
        VDP_fillTileMapRect(BG_B,border,1,26,38,1);
        for (u16 y=0;y<GRID_H;++y) for (u16 x=0;x<GRID_W;++x)
            VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+
                               (world_entry_pad(game->level,world_point(y*GRID_W+x)) ? 7 : world_wall(game->level,x,y))),
                               2+x*2,8+y*2,2,2);
        number(game->level+1,32,1,1); field("OF 3",34,1,4); last_level=game->level;
    }
    if (last_phase!=game->phase) {
        VDP_clearTextArea(1,8,38,18);
        if (game->phase!=PLAYING) {
            VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_USER_INDEX+3),6,12,28,7);
            VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_USER_INDEX+2),7,13,26,5);
            static const char *const captions[]={"","CAUGHT!","AREA CLEAR","GAME OVER","MISSION COMPLETE"};
            field(captions[game->phase],11,14,23);
            field(game->phase==GAME_OVER || game->phase==COMPLETE ? "START FOR A NEW RUN" :
                  game->phase==CAUGHT ? "BACK TO THE ENTRY POINT" : "MOVING TO THE NEXT AREA",9,16,26);
        }
        last_phase=game->phase;
    }
    for (u16 i=0;i<3;++i) {
        SPR_setPosition(tries[i],64+i*20,24); SPR_setAnimAndFrame(tries[i],0,i<game->tries ? 0 : 1);
        SPR_setVisibility(tries[i],VISIBLE);
    }
    for (u16 p=0;p<2;++p) {
        const Player *player=&game->players[p];
        u16 fill=(STUN_COOLDOWN-player->cooldown)*10/STUN_COOLDOWN;
        u16 charge=player->active ? (player->cooldown ? fill : 20) : 30;
        if (last_charge[p]!=charge) {
            u16 x=p ? 27 : 8;
            VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+2),x,6,10,1);
            if (p) field(player->active ? "P2" : "",23,6,3);
            if (player->active && fill) VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,
                TILE_USER_INDEX+(player->cooldown ? 8 : 9)),x,6,fill,1);
            last_charge[p]=charge;
        }
    }
    Point exit=world_exit(game->level); SPR_setPosition(exit_sprite,exit.x,exit.y);
    SPR_setAnimAndFrame(exit_sprite,0,(game->tick/20)%2); SPR_setVisibility(exit_sprite,VISIBLE);
    for (u16 p=0;p<2;++p) {
        const Player *player=&game->players[p];
        robot(players[p],player->pos,player->facing,player->animation,player->moving,
              player->active && (!player->invulnerable || (game->tick/6)%2==0));
        SPR_setPosition(pulses[p],player->pos.x-24,player->pos.y-24);
        SPR_setAnimAndFrame(pulses[p],0,player->pulse ? (16-player->pulse)/4 : 0);
        SPR_setVisibility(pulses[p],player->pulse ? VISIBLE : HIDDEN);
    }
    for (u16 i=0;i<ENEMY_COUNT;++i) {
        const Enemy *enemy=&game->enemies[i];
        robot(enemies[i],enemy->pos,enemy->facing,enemy->animation,enemy->moving,
              !(enemy->state==STUNNED && enemy->timer<=60 && (game->tick/6)%2));
        SPR_setPosition(status[i],enemy->pos.x,enemy->pos.y-14);
        SPR_setAnimAndFrame(status[i],0,enemy->state==STUNNED ? 1+(game->tick/20)%2 : 0);
        SPR_setVisibility(status[i],enemy->state==STUNNED || enemy->alert || enemy->state==WINDUP ? VISIBLE : HIDDEN);
        VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,TILE_USER_INDEX+
                           (enemy->state==STUNNED ? 5 : enemy->state==CHASE || enemy->state==WINDUP ? 4 : 6)),
                           27+i*2,3,1,1);
    }
    static const u8 frames[8]={0,1,2,1,0,1,2,1};
    for (u16 i=0;i<SHOT_COUNT;++i) {
        const Shot *shot=&game->shots[i];
        SPR_setPosition(shots[i],shot->x/8,shot->y/8); SPR_setAnimAndFrame(shots[i],0,frames[shot->direction]);
        SPR_setHFlip(shots[i],shot->direction>=3 && shot->direction<=5); SPR_setVFlip(shots[i],shot->direction>=5);
        SPR_setVisibility(shots[i],shot->active ? VISIBLE : HIDDEN);
    }
    u16 selected=nearest(game); overlay(game,selected);
    static const char *const states[]={"IDLE","PATROL","CHASE","RETURN","WINDUP","RECOVER","STUNNED"};
    if (last_selected!=selected || last_state!=game->enemies[selected].state || last_overlay!=game->overlay) {
        static const char *const modes[]={"NORMAL","SENSE","PATH"};
        field(modes[game->overlay],2,27,7); field("NEAR",10,27,5); number(selected+1,15,27,1);
        field(states[game->enemies[selected].state],17,27,8); field("NAV",27,27,3); field("/16",33,27,3);
        last_selected=selected; last_state=game->enemies[selected].state; last_overlay=game->overlay;
    }
    if (last_expanded!=game->nav.expanded) { number(game->nav.expanded,31,27,2); last_expanded=game->nav.expanded; }
    if (last_pause!=game->paused) { field(game->paused ? "PAUSED" : "",17,1,7); last_pause=game->paused; }
    SPR_update();
}
