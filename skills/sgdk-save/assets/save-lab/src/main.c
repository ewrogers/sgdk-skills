#include <genesis.h>
#include "storage.h"

static SaveState state;
static u16 selected;
static bool password_mode;
static u16 cursor;
static char password[SAVE_PASSWORD_LENGTH + 1];
static const char *notice = "Ready";

static const char *result_text(SaveResult result)
{
    switch (result) {
        case SAVE_OK: return "OK";
        case SAVE_EMPTY: return "No save yet";
        case SAVE_CORRUPT: return "Damaged save - keeping working state";
        case SAVE_UNSUPPORTED: return "Newer save version - left untouched";
        case SAVE_CONFLICT: return "Conflicting saves - left untouched";
        case SAVE_IO_ERROR: return "SRAM read/write check failed";
        default: return "Invalid game values";
    }
}

static void number(u32 value, u16 x, u16 y)
{
    char text[12];
    uintToStr(value, text, 1);
    VDP_drawText(text, x, y);
}

static void draw(void)
{
    SaveState saved;
    u8 slot;
    SaveResult result = storage_load(&saved, &slot);
    VDP_clearPlane(BG_A, true);
    VDP_setTextPalette(PAL1);
    VDP_drawText("========== SGDK SAVE LAB ==========", 2, 1);
    VDP_setTextPalette(PAL0);
    VDP_drawText("Level", 5, 4); number(state.level, 20, 4);
    VDP_drawText("Score", 5, 6); number(state.score, 20, 6);
    VDP_drawText("Lives", 5, 8); number(state.lives, 20, 8);
    VDP_drawText("Music setting", 5, 10);
    VDP_drawText(state.music ? "On" : "Off", 20, 10);
    VDP_drawText("(Setting only; this lab is silent)", 5, 12);
    VDP_drawText("SRAM:", 3, 15);
    VDP_drawText(result_text(result), 3, 16);
    if (result == SAVE_OK) {
        VDP_drawText("Newest slot", 3, 17); number(slot + 1, 16, 17);
    }
    if (password_mode) {
        VDP_drawText("PASSWORD ENTRY", 3, 19);
        VDP_drawText(password, 12, 20);
        VDP_drawText("^", (u16)(12 + cursor), 21);
        VDP_drawText("Left/Right: cursor  Up/Down: letter", 2, 23);
        VDP_drawText("A: apply   B: cancel", 2, 24);
    } else {
        VDP_drawText(">", 3, (u16)(4 + selected * 2));
        save_password_encode(&state, password);
        VDP_drawText("Password:", 3, 19);
        VDP_drawText(password, 15, 19);
        VDP_drawText("D-pad: edit  A: save  B: load", 2, 21);
        VDP_drawText("C: password   Start: reset values", 2, 22);
        VDP_drawText("Hold A + press C: damage newest slot", 2, 23);
        VDP_drawText("Hold B + press C: erase BOTH slots", 2, 24);
    }
    VDP_setTextPalette(PAL1);
    VDP_drawText(notice, 2, 26);
    VDP_setTextPalette(PAL0);
}

static void edit_state(s16 direction)
{
    switch (selected) {
        case 0:
            if (direction > 0 && state.level < 63) state.level++;
            if (direction < 0 && state.level > 1) state.level--;
            break;
        case 1:
            if (direction > 0 && state.score <= SAVE_MAX_SCORE - 100) state.score += 100;
            if (direction < 0 && state.score >= 100) state.score -= 100;
            break;
        case 2:
            if (direction > 0 && state.lives < 9) state.lives++;
            if (direction < 0 && state.lives > 1) state.lives--;
            break;
        default: state.music = !state.music; break;
    }
    notice = "Working values changed; not saved";
}

static void edit_password(s16 direction)
{
    u16 i;
    for (i = 0; i < 32 && save_password_alphabet[i] != password[cursor]; i++) {}
    password[cursor] = save_password_alphabet[(i + direction + 32) % 32];
}

int main(bool hard_reset)
{
    u16 previous = 0;
    (void)hard_reset;
    JOY_init();
    VDP_setScreenWidth320();
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x101010));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xCCCCDD));
    PAL_setColor(31, RGB24_TO_VDPCOLOR(0x88BBFF));
    save_defaults(&state);
    notice = result_text(storage_load(&state, 0));
    draw();
    while (true) {
        u16 held = JOY_readJoypad(JOY_1);
        u16 pressed = held & (u16)~previous;
        previous = held;
        if (pressed) {
            if (password_mode) {
                if (pressed & BUTTON_LEFT) cursor = (u16)((cursor + 11) % 12);
                if (pressed & BUTTON_RIGHT) cursor = (u16)((cursor + 1) % 12);
                if (pressed & BUTTON_UP) edit_password(1);
                if (pressed & BUTTON_DOWN) edit_password(-1);
                if (pressed & BUTTON_A) {
                    if (save_password_decode(password, &state)) {
                        notice = "Password applied; A saves to SRAM";
                        password_mode = false;
                    } else notice = "Invalid password; check every letter";
                }
                if (pressed & BUTTON_B) { password_mode = false; notice = "Entry cancelled"; }
            } else if (pressed & BUTTON_C) {
                if (held & BUTTON_A) notice = result_text(storage_damage_newest());
                else if (held & BUTTON_B) notice = result_text(storage_erase());
                else { save_password_encode(&state, password); cursor = 0; password_mode = true; }
            } else if (pressed & BUTTON_START) {
                save_defaults(&state); notice = "Defaults restored in RAM only";
            } else if (pressed & BUTTON_A) {
                notice = result_text(storage_store(&state));
            } else if (pressed & BUTTON_B) {
                notice = result_text(storage_load(&state, 0));
            } else {
                if (pressed & BUTTON_UP) selected = (u16)((selected + 3) % 4);
                if (pressed & BUTTON_DOWN) selected = (u16)((selected + 1) % 4);
                if (pressed & BUTTON_LEFT) edit_state(-1);
                if (pressed & BUTTON_RIGHT) edit_state(1);
            }
            draw();
        }
        SYS_doVBlankProcess();
    }
    return 0;
}
