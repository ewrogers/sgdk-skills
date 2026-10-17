#include <genesis.h>
#include "resources.h"
#include "model.h"
#include "controls.h"
static Courier game;
static Sprite *hero;
static u16 colors[32];
static void no_event(u16 joy, u16 changed, u16 held)
{ (void)joy; (void)changed; (void)held; }
static void present(void) { SPR_update(); SYS_doVBlankProcess(); }
static void room_draw(void)
{
    VDP_clearPlane(BG_A,TRUE);
    VDP_setTextPalette(PAL0);
    for (u16 y=4;y<24;y++) for (u16 x=0;x<40;x++) {
        u16 tile=courier_solid(&game,x*8+4,y*8+4) ? 1 : 0;
        if(!game.room && !game.parcel && x>=19 && x<21 && y>=11 && y<15)tile=2;
        if (!game.room && !game.parcel && x>=9 && x<11 && y>=7 && y<9) tile=3;
        if (game.room && x>=33 && x<35 && y>=18 && y<20) tile=4;
        VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX+tile),x,y);
    }
    VDP_drawText(game.room ? "DELIVERY COURT" : "PARCEL COURT",2,1);
    VDP_drawText(game.parcel ? "PARCEL: YES" : "PARCEL: NO ",24,1);
    VDP_drawText("D-PAD MOVE   START PAUSE",2,25);
    if(game.room) {
        VDP_drawText("[A]",33,16);VDP_drawText("DELIVER",31,17);
    } else {
        VDP_drawText("EXIT",35,10);VDP_drawText(">>",37,12);
    }
    VDP_drawText(game.room ? "FOLLOW THE LOWER PASSAGE TO SEAL" : "FIND PARCEL THEN EXIT RIGHT >>",2,26);
}
static void handoff(void)
{
    PSG_setEnvelope(0,15);
    PAL_fadeOut(0,31,12,TRUE);
    while (PAL_isDoingFade()) present();
    present(); /* Drain the final queued palette step while its source is alive. */
    SPR_releaseSprite(hero);hero=NULL;
    present(); /* Remove the old SAT entry before reusing its resources. */
    room_draw();
    hero=SPR_addSprite(&courier,game.x-8,game.y-8,TILE_ATTR(PAL1,TRUE,FALSE,FALSE));
    present();
    PAL_fadeIn(0,31,colors,12,TRUE);
    while (PAL_isDoingFade()) present();
    present();
    /* No other scene owner: consume buttons held during the transition. */
    game.previous=courier_controls(JOY_readJoypad(JOY_1));
}
int main(bool hardReset)
{
    (void)hardReset;
    JOY_init();JOY_setEventHandler(no_event);
    VDP_setScreenWidth320();SPR_initEx(16);
    PAL_setColors(0,(const u16[32]){0},32,CPU);
    VDP_loadTileSet(court.tileset,TILE_USER_INDEX,CPU);
    for (u16 i=0;i<16;i++) { colors[i]=court.palette->data[i];colors[16+i]=courier.palette->data[i]; }
    colors[15]=RGB24_TO_VDPCOLOR(0xFFFFFF);
    courier_init(&game,courier_controls(JOY_readJoypad(JOY_1)));room_draw();
    hero=SPR_addSprite(&courier,game.x-8,game.y-8,TILE_ATTR(PAL1,TRUE,FALSE,FALSE));
    PAL_setColors(0,colors,32,CPU);
    if (!hero) { VDP_drawText("SPRITE ALLOCATION FAILED",2,14);while(1)SYS_doVBlankProcess(); }
    u16 hz=SYS_isPAL() ? 50 : 60,last_cue=0,last_pause=2,last_near=2;
    while(TRUE) {
        u16 held=courier_controls(JOY_readJoypad(JOY_1));
        courier_frame(&game,held,hz);
        if (game.events&C_TRAVEL) {
            handoff();
            if(!hero) { VDP_drawText("SPRITE ALLOCATION FAILED",2,14);while(1)SYS_doVBlankProcess(); }
        }
        if (game.events&C_PICKUP) {
            for (u16 y=7;y<9;y++) for (u16 x=9;x<11;x++)
                VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),x,y);
            for (u16 y=11;y<15;y++) for (u16 x=19;x<21;x++)
                VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),x,y);
            VDP_drawText("PARCEL: YES",24,1);
            VDP_drawTextBGFill(BG_A,"GATE OPEN - EXIT AT RIGHT EDGE >>",2,26,36);
        }
        if (game.events&C_DELIVER) VDP_drawText("DELIVERED! THANK YOU",10,13);
        if(game.room && last_near!=courier_can_deliver(&game)) {
            last_near=courier_can_deliver(&game);
            VDP_drawTextBGFill(BG_A,game.won ? "DELIVERY COMPLETE" : last_near ? "PRESS A TO DELIVER THE PARCEL" : "FOLLOW THE LOWER PASSAGE TO SEAL",2,26,36);
        }
        if (game.paused!=last_pause) {
            VDP_drawText(game.paused ? "PAUSED" : "      ",32,2);
            last_pause=game.paused;
        }
        if (game.cue && !game.paused) {
            if(game.cue>last_cue)PSG_setFrequency(0,game.won ? 880 : 660);
            PSG_setEnvelope(0,game.cue>6 ? 5 : 10);
        } else PSG_setEnvelope(0,15);
        last_cue=game.cue;
        SPR_setPosition(hero,game.x-8,game.y-8);
        present();
    }
    return 0;
}
