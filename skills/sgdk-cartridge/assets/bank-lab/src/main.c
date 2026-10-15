#include <genesis.h>
#include "bank_model.h"
#include "bank_io.h"
#include "common.h"
#include "stage_a.h"
#include "stage_b.h"
#include "music_a.h"
#include "music_b.h"
#include "stage_checks.inc"

#if !ENABLE_BANK_SWITCH
#error Bank Lab requires the rebuilt bank-enabled SDK; use tools/build.py
#endif

extern const u8 bank_edge_probe[32], __music_a_end[], __music_b_end[];
typedef struct {
    const char *name;
    const u8 *palette, *marker, *music, *music_end;
    const TileSet *floor;
    const SpriteDefinition *sprite;
    u16 crc, art_bank, music_bank;
} Stage;

static const Stage stages[BANK_STAGE_COUNT] = {
    {"EMBER FOUNDRY", stage_a_palette, stage_a_marker, stage_music_a, __music_a_end,
     &stage_a_floor, &stage_a_sprite, STAGE_A_CRC, 8, 9},
    {"TIDAL VAULT", stage_b_palette, stage_b_marker, stage_music_b, __music_b_end,
     &stage_b_floor, &stage_b_sprite, STAGE_B_CRC, 10, 11}
};
static BankModel game;
static Sprite *hero, *relic;
static bool data_ok, edge_ok, bank_ok;
static const char *failure = "STARTUP";
static const char *save_message = "NOT SAVED";

static bool palette_read(const Stage *stage, u16 colors[16])
{
    u8 bytes[48];
    if (!bank_copy(bytes, sizeof bytes, stage->palette, 32) ||
        !bank_copy(bytes + 32, 16, stage->marker, 16) || bank_crc16(bytes, sizeof bytes) != stage->crc) return FALSE;
    for (u16 i = 0; i < 16; ++i) colors[i] = (u16)bytes[i * 2] << 8 | bytes[i * 2 + 1];
    return TRUE;
}

static bool boundary_read(void)
{
    static const u8 expected[] = "BANK EIGHT END!!BANK NINE START!";
    u8 bytes[32];
    if (!bank_copy(bytes, sizeof bytes, bank_edge_probe, sizeof bytes)) return FALSE;
    for (u16 i = 0; i < sizeof bytes; ++i) if (bytes[i] != expected[i]) return FALSE;
    return TRUE;
}

static void text(const char *value, u16 x, u16 y)
{
    VDP_clearTextLine(y);
    VDP_drawText(value, x, y);
}

static void number(u16 value, u16 x, u16 y)
{
    char out[6]; uintToStr(value, out, 2); VDP_drawText(out, x, y);
}

static void hud(void)
{
    const Stage *stage = &stages[game.stage];
    text("BANK LAB / COMMON STAYS PINNED", 1, 0);
    text(stage->name, 1, 2);
    text("COMMON 00  ART 00  AUDIO 00", 1, 3);
    number(stage->art_bank, 16, 3); number(stage->music_bank, 26, 3);
    text("AUDIO WINDOW 3 / FAR WINDOWS 6+7", 1, 4);
    text(data_ok && edge_ok ? "FAR DATA OK / CROSS-BANK READ OK" : "BANK DATA CHECK FAILED", 1, 5);
    text("SRAM:", 1, 6); VDP_drawText(save_message, 7, 6);
    text(bank_ok ? "PINNED BANKS OK / MUSIC:" : "PINNED BANK CHECK FAILED", 1, 7);
    VDP_drawText(game.music ? "ON " : "OFF", 26, 7);
    text("DPAD MOVE / A NEXT STAGE", 1, 25);
    text("B SAVE / C LOAD / START MUSIC", 1, 26);
    text("SHARED HERO, UI AND CLICK: BANK 0", 1, 27);
}

static bool pins_valid(void)
{
    return SYS_getBank(0) == 0 && SYS_getBank(BANK_AUDIO_WINDOW) == stages[game.stage].music_bank;
}

static bool load_stage(void)
{
    const Stage *stage = &stages[game.stage];
    u16 colors[16];
    SYS_doVBlankProcess(); /* Drain any old queued ROM readers before changing content. */
    VDP_setEnable(FALSE);
    data_ok = palette_read(stage, colors);
    failure = "PALETTE CRC / FAR READ";
    if (!data_ok) return FALSE;
    failure = "FLOOR TILE TRANSFER";
    if (!VDP_loadTileSet(stage->floor, TILE_USER_INDEX, DMA)) return FALSE;
    VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 10, 40, 15);
    PAL_setPalette(PAL0, colors, DMA_QUEUE_COPY);
    /* Deliberately map both FAR windows after queuing the palette. Its copied
       RAM payload remains valid even when the temporary ROM views change. */
    edge_ok = boundary_read();
    failure = "CROSS-BANK PROBE";
    if (!edge_ok) return FALSE;
    failure = "SPRITE DEFINITION";
    if (!SPR_setDefinition(relic, stage->sprite)) return FALSE;
    u32 length = (u32)((unsigned long)stage->music_end - (unsigned long)stage->music);
    failure = "AUDIO BANK RANGE";
    if (!bank_audio_select(stage->music, length, game.music)) return FALSE;
    failure = "PINNED BANK REGISTERS";
    bank_ok = pins_valid();
    SPR_setPosition(hero, game.x, game.y);
    SPR_update(); /* Last FAR producer before the queued sprite tiles flush. */
    SYS_doVBlankProcess();
    hud();
    VDP_setEnable(TRUE);
    return bank_ok;
}

static void fatal(void)
{
    Z80_unloadDriver();
    VDP_clearPlane(BG_A, TRUE);
    VDP_setEnable(TRUE);
    VDP_drawText("BANK LAB CHECK FAILED", 4, 12);
    VDP_drawText(failure, 2, 14);
    while (TRUE) SYS_doVBlankProcess();
}

static u16 input(void)
{
    u16 held = JOY_readJoypad(JOY_1), out = 0;
    if (held & BUTTON_LEFT) out |= BANK_LEFT;
    if (held & BUTTON_RIGHT) out |= BANK_RIGHT;
    if (held & BUTTON_UP) out |= BANK_UP;
    if (held & BUTTON_DOWN) out |= BANK_DOWN;
    if (held & BUTTON_A) out |= BANK_STAGE;
    if (held & BUTTON_B) out |= BANK_SAVE;
    if (held & BUTTON_C) out |= BANK_LOAD;
    if (held & BUTTON_START) out |= BANK_MUSIC;
    return out;
}

static bool stored_banks_unchanged(const u16 before[8])
{
    for (u16 i = 0; i < 8; ++i) if (SYS_getBank(i) != before[i]) return FALSE;
    return TRUE;
}

int main(bool hard_reset)
{
    (void)hard_reset;
    u16 hz = SYS_isPAL() ? 50 : 60, refresh = 0;
    bank_model_init(&game); JOY_init();
    VDP_setEnable(FALSE); VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_clearPlane(BG_A, TRUE); VDP_clearPlane(BG_B, TRUE);
    VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3);
    PAL_setPalette(PAL3, palette_grey, CPU);
    PAL_setPalette(PAL1, hero_image.palette->data, CPU);
    u16 relic_tiles = stage_a_sprite.maxNumTile > stage_b_sprite.maxNumTile ?
                      stage_a_sprite.maxNumTile : stage_b_sprite.maxNumTile;
    SPR_initEx(hero_image.maxNumTile + relic_tiles);
    if (TILE_USER_INDEX + stage_a_floor.numTile > TILE_USER_MAX_INDEX ||
        TILE_USER_INDEX + stage_b_floor.numTile > TILE_USER_MAX_INDEX) fatal();
    hero = SPR_addSprite(&hero_image, game.x, game.y, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    relic = SPR_addSprite(&stage_a_sprite, 232, 124, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    if (!hero || !relic) fatal();
    SPR_setAutoAnimation(hero, FALSE);
    if (!load_stage()) fatal();
    while (TRUE) {
        u16 action = bank_model_step(&game, input(), hz);
        if (action & BANK_STAGE) {
            game.stage = (game.stage + 1) % BANK_STAGE_COUNT;
            if (!load_stage()) fatal();
        } else if (action & (BANK_SAVE | BANK_LOAD)) {
            u16 banks[8]; for (u16 i = 0; i < 8; ++i) banks[i] = SYS_getBank(i);
            bool ok = (action & BANK_SAVE) ? bank_store_write(&game) : bank_store_read(&game);
            bool preserved = stored_banks_unchanged(banks);
            save_message = !preserved ? "MAPPER CHANGED" : !ok ? "NO VALID SAVE / WRITE FAILED" :
                           (action & BANK_SAVE) ? "SAVED / BANKS UNCHANGED" : "LOADED / BANKS UNCHANGED";
            if (ok && preserved && (action & BANK_LOAD) && !load_stage()) fatal();
            bank_ok = preserved && pins_valid();
        }
        if (action & BANK_MUSIC) {
            game.music = !game.music;
            if (game.music) XGM2_playTrack(0); else XGM2_stop();
        }
        if (action & (BANK_STAGE | BANK_SAVE | BANK_LOAD))
            XGM2_playPCM(click_sound, sizeof click_sound, SOUND_PCM_CH2);
        /* Make an unrelated cross-bank read repeatedly during music playback.
           All such accesses finish BEFORE SPR_update queues its own FAR views. */
        if (++refresh >= hz) {
            u16 ignored[16]; refresh = 0;
            data_ok = palette_read(&stages[game.stage], ignored);
            edge_ok = boundary_read(); bank_ok = bank_ok && pins_valid();
        }
        if (action || refresh == 0) hud();
        SPR_setPosition(hero, game.x, game.y);
        SPR_setAnimAndFrame(hero, 0, 4 + (game.phase / 12) % 2);
        SPR_update();
        /* No more FAR access until these borrowed ROM tile pointers flush. */
        SYS_doVBlankProcess();
    }
}
