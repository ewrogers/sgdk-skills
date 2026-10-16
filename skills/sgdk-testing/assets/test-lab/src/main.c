#include <genesis.h>
#include "lab.h"
#include "replay_data.h"
#include "expected_data.h"
#include "resources.h"

_Static_assert(LAB_A==BUTTON_A && LAB_B==BUTTON_B && LAB_C==BUTTON_C &&
               LAB_START==BUTTON_START && LAB_RIGHT==BUTTON_RIGHT,
               "Replay masks must match the SGDK snapshot");

enum { REFERENCE, RECORD, TAKE };
static LabState state;
static LabTape tape;
static LabClock clock;
static Sprite *courier, *motes[LAB_COINS], *bolts[LAB_BOLTS];
static u16 courier_tiles[2], mote_tiles[3], bolt_tile;
static u16 mode, previous, failed_tick, frames, take_hz;
static u32 take_digest, failed_expected, failed_actual;
static bool fault, finished, transport_paused, have_take, failed;
static const char *record_stop;

static void line(const char *text, u16 y, u16 palette)
{
    char buffer[41];
    u16 n=0;
    while (text[n] && n<40) { buffer[n]=text[n]; n++; }
    while (n<40) buffer[n++]=' ';
    buffer[40]=0;
    /* The SDK copies text tilemap entries to its queue buffer. */
    VDP_drawTextEx(BG_A,buffer,TILE_ATTR(palette,TRUE,FALSE,FALSE),0,y,DMA_QUEUE);
}

static void checkpoint(void)
{
    if (mode!=REFERENCE || state.tick%60) return;
    u16 index=state.tick/60;
    u32 actual=lab_digest(&state);
    if (index<EXPECTED_COUNT && actual!=expected_hashes[index] && !failed) {
        failed=true; failed_tick=state.tick;
        failed_expected=expected_hashes[index]; failed_actual=actual;
    }
}

static void begin(u16 next_mode, u16 hz)
{
    mode=next_mode; clock.phase=0; frames=0; failed=false;
    finished=false; transport_paused=false;
    lab_reset(&state,REPLAY_SEED);
    if (mode==RECORD) {
        tape.count=0; tape.ticks=0; have_take=false; take_hz=hz;
        record_stop="MANUAL STOP";
    }
    checkpoint();
}

static void end_record(void)
{
    take_digest=lab_digest(&state);
    have_take=tape.ticks!=0;
    finished=true;
}

static void tick(u16 held)
{
    u16 input=0;
    if (mode==RECORD) {
        input=held & (LAB_UP|LAB_DOWN|LAB_LEFT|LAB_RIGHT|LAB_A|LAB_START);
        if (!lab_record(&tape,input)) {
            record_stop="INPUT BUFFER FULL"; end_record(); return;
        }
    } else {
        const LabRun *runs=mode==TAKE ? tape.runs : reference_runs;
        u16 count=mode==TAKE ? tape.count : REPLAY_COUNT;
        if (!lab_input(runs,count,state.tick,&input)) { finished=true; return; }
    }
    lab_step(&state,input,fault && mode==REFERENCE);
    checkpoint();
    u16 limit=mode==TAKE ? tape.ticks : REPLAY_TICKS;
    if (state.tick==limit) {
        if (mode==RECORD) { record_stop="12 SECOND LIMIT"; end_record(); }
        else {
            finished=true;
            if (mode==TAKE && lab_digest(&state)!=take_digest) {
                failed=true; failed_tick=state.tick;
                failed_expected=take_digest; failed_actual=lab_digest(&state);
            }
        }
    }
}

static void draw_status(u16 hz)
{
    char buffer[64];
    sprintf(buffer," TEST LAB  %s %uHZ  LOGIC 60HZ",hz==50 ? "PAL" : "NTSC",hz);
    line(buffer,0,PAL0);
    const char *name=mode==REFERENCE ? "REFERENCE" : mode==RECORD ? "RECORDING" : "YOUR REPLAY";
    sprintf(buffer," %s  TICK %u/%u  SCORE %u",name,state.tick,
            mode==TAKE ? tape.ticks : REPLAY_TICKS,state.score);
    line(buffer,1,PAL0);
    if (failed) sprintf(buffer," DIVERGED AT TICK %u  UP FIXES BUG",failed_tick);
    else if (finished) sprintf(buffer," %s  HASH %08lX  FRAMES %u",
                              mode==RECORD ? "TAKE SAVED" : "PASS",(unsigned long)lab_digest(&state),frames);
    else sprintf(buffer," %s   %s",transport_paused ? "VIEW PAUSED" : state.paused ? "GAME PAUSED" : "RUNNING",
                 fault && mode==REFERENCE ? "MOVEMENT BUG ENABLED" : "SEEDED COURIER TEST");
    line(buffer,2,failed ? PAL3 : PAL0);
    if (mode==RECORD && !finished) {
        line(" D-PAD MOVE  A FIRE  START PAUSE",24,PAL0);
        line(" B FINISH TAKE   MAX 12 SECONDS",25,PAL0);
    } else {
        line(" A REFERENCE   B RECORD   C REPLAY",24,PAL0);
        line(" UP TOGGLE BUG   START PAUSE VIEW",25,PAL0);
    }
    if (failed) {
        sprintf(buffer," EXPECT %08lX   GOT %08lX",(unsigned long)failed_expected,(unsigned long)failed_actual);
    } else if (mode==RECORD && finished) {
        sprintf(buffer," %s   C REPLAYS YOUR TAKE",record_stop);
    } else if (have_take) {
        sprintf(buffer," TAKE %u TICKS  %u RUNS  %uHZ SOURCE",tape.ticks,tape.count,take_hz);
    } else strcpy(buffer," RECORD YOUR ROUTE THEN REPLAY IT");
    line(buffer,27,PAL0);
}

static bool cache(const SpriteDefinition *definition, u16 *index, u16 *tiles, u16 frames_count)
{
    u16 count;
    u16 **loaded=SPR_loadAllFrames(definition,*index,&count);
    if (!loaded) return false;
    for (u16 i=0;i<frames_count;i++) tiles[i]=loaded[0][i];
    MEM_free(loaded);
    *index+=count;
    return *index<=TILE_FONT_INDEX;
}

static Sprite *sprite(const SpriteDefinition *definition, u16 tile, u16 palette)
{
    Sprite *result=SPR_addSpriteEx(definition,0,0,TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,tile),0);
    if (result) { SPR_setAutoAnimation(result,FALSE); SPR_setVisibility(result,VISIBLE); }
    return result;
}

static bool graphics(void)
{
    static const u16 palette[16]={0x0200,0x0420,0x0642,0x0864,0x0CA4,0x0ECA,0x04AE,0x026A,
                                0x0642,0x0864,0x08C6,0x0AEC,0x044A,0x066E,0x0AAE,0x0EEE};
    static const u32 grid[8]={0x11111111,0x10000000,0x10000000,0x10000000,
                              0x10000000,0x10000000,0x10000000,0x10000000};
    static const u32 wall[8]={0x33333333,0x32222223,0x32222223,0x32222223,
                              0x32222223,0x32222223,0x32222223,0x33333333};
    PAL_setPalette(PAL0,palette,CPU);
    PAL_setPalette(PAL1,courier_image.palette->data,CPU);
    PAL_setPalette(PAL2,mote_image.palette->data,CPU);
    PAL_setPalette(PAL3,bolt_image.palette->data,CPU);
    VDP_setTextPalette(PAL0);
    VDP_loadTileData(grid,TILE_USER_INDEX,1,CPU);
    VDP_loadTileData(wall,TILE_USER_INDEX+1,1,CPU);
    VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,4,40,19);
    VDP_fillTileMapRect(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX+1),18,10,4,8);
    SPR_initEx(32);
    u16 index=TILE_FONT_INDEX-32;
    if (!cache(&courier_image,&index,courier_tiles,2) ||
        !cache(&mote_image,&index,mote_tiles,3) ||
        !cache(&bolt_image,&index,&bolt_tile,1)) return false;
    courier=sprite(&courier_image,courier_tiles[0],PAL1);
    if (!courier) return false;
    for (u16 i=0;i<LAB_COINS;i++) {
        motes[i]=sprite(&mote_image,mote_tiles[0],PAL2);
        if (!motes[i]) return false;
    }
    for (u16 i=0;i<LAB_BOLTS;i++) {
        bolts[i]=sprite(&bolt_image,bolt_tile,PAL3);
        if (!bolts[i]) return false;
    }
    return true;
}

static void render(void)
{
    u16 frame=(state.active_ticks/10)&1;
    SPR_setPosition(courier,state.x-8,state.y-8);
    SPR_setFrame(courier,frame); SPR_setVRAMTileIndex(courier,courier_tiles[frame]);
    SPR_setHFlip(courier,(state.held & LAB_LEFT)!=0);
    for (u16 i=0;i<LAB_COINS;i++) {
        frame=(state.active_ticks/12+i)%3;
        SPR_setPosition(motes[i],state.coin_x[i]-4,state.coin_y[i]-4);
        SPR_setFrame(motes[i],frame); SPR_setVRAMTileIndex(motes[i],mote_tiles[frame]);
    }
    for (u16 i=0;i<LAB_BOLTS;i++) {
        SPR_setVisibility(bolts[i],state.bolts[i].life ? VISIBLE : HIDDEN);
        SPR_setPosition(bolts[i],state.bolts[i].x-4,state.bolts[i].y-4);
    }
    SPR_update();
}

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setBackgroundColor(0);
    if (!graphics()) {
        VDP_drawText("TEST LAB GRAPHICS INIT FAILED",4,12);
        VDP_setEnable(TRUE); SYS_enableInts();
        while (TRUE) SYS_doVBlankProcess();
    }
    u16 hz=SYS_isPAL() ? 50 : 60;
    begin(REFERENCE,hz);
    SYS_enableInts(); VDP_setEnable(TRUE);
    u16 redraw=0;
    while (TRUE) {
        u16 held=JOY_readJoypad(JOY_1);
        u16 pressed=held & (u16)~previous;
        previous=held;
        if (mode==RECORD && !finished) {
            if (pressed & LAB_B) end_record();
        } else if (pressed & LAB_B) begin(RECORD,hz);
        else if (pressed & LAB_A) begin(REFERENCE,hz);
        else if (pressed & LAB_C) begin(have_take ? TAKE : REFERENCE,hz);
        else if (pressed & LAB_UP) { fault=!fault; begin(REFERENCE,hz); }
        else if (pressed & LAB_START) transport_paused=!transport_paused;
        if (!finished && !transport_paused) {
            frames++;
            u16 count=lab_frame_ticks(&clock,hz);
            while (count-- && !finished) tick(held);
        }
        render();
        if (!redraw-- || finished) { draw_status(hz); redraw=5; }
        SYS_doVBlankProcess();
    }
}
