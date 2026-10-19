#include <genesis.h>
#include "readiness.h"
#include "lab_ui.h"

void hl_ready_open(bool hard_reset, u16 entry_data, u16 entry_bss, bool pal, const HlEdLink *link)
{
    const bool data_ok = entry_data == 0x1234;
    const bool bss_ok = entry_bss == 0;
    const bool fresh = hard_reset && data_ok && bss_ok;
    hl_ui_begin(0);

    hl_ui_card(10, 9);
    hl_ui_text("BOOT / RAM CHECKS", 4, 11, PAL2);
    hl_ui_text(fresh ? "PASS" : "REVIEW", 29, 11, fresh ? PAL3 : PAL1);
    hl_ui_text("BOOT ENTRY", 4, 12, PAL2);
    hl_ui_text(hard_reset ? "HARD" : "SOFT", 29, 12, hard_reset ? PAL3 : PAL1);
    hl_ui_text("INIT DATA", 4, 13, PAL2);
    hl_ui_hex(entry_data, 4, 23, 13, PAL1);
    hl_ui_text(data_ok ? "PASS" : "CHECK", 29, 13, data_ok ? PAL3 : PAL1);
    hl_ui_text("ZERO / BSS", 4, 14, PAL2);
    hl_ui_hex(entry_bss, 4, 23, 14, PAL1);
    hl_ui_text(bss_ok ? "PASS" : "CHECK", 29, 14, bss_ok ? PAL3 : PAL1);
    hl_ui_text("EVERDRIVE EDIO", 4, 16, PAL2);
    hl_ui_text(link->present ? "DETECTED" : "UNKNOWN", 25, 16, link->present ? PAL3 : PAL1);
    hl_ui_text("USB HOST CHECK", 4, 17, PAL2);

    hl_ui_card(20, 4);
    hl_ui_text("RUNNING", 4, 21, PAL1);
    hl_ui_text("V", 24, 21, PAL2);
    hl_ui_text("LOGIC", 4, 22, PAL2);
    hl_ui_text("MISS", 20, 22, PAL2);
    hl_ui_text(pal ? "VIDEO PAL  / NOMINAL 50" : "VIDEO NTSC / NOMINAL 60", 2, 25, PAL0);
    hl_ui_text("L/R PAGES  A RESET  B INIT  C ZERO", 2, 26, PAL0);
}

void hl_ready_update(u32 vints, u32 logic, u32 missed, const HlEdLink *link)
{
    const u16 head = (vints / 10) % 8;
    hl_ui_meter(head,15,21);
    hl_ui_hex(vints, 8, 26, 21, PAL1);
    hl_ui_hex(logic, 8, 10, 22, PAL1);
    hl_ui_hex(missed, 8, 25, 22, PAL1);
    hl_ui_text(!link->present ? "UNAVAILABLE" : link->verified ? "VERIFIED   " :
               link->ever_verified ? "EXPIRED    " : "WAITING    ",
               25, 17, link->verified ? PAL3 : PAL1);
}
