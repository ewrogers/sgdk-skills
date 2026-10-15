#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header={
    .console="SEGA MEGA DRIVE", .copyright="(C)2026 SGDK",
    .title_local="SGDK ENEMY LAB", .title_int="SGDK ENEMY LAB",
    .serial={'G','M',' ','0','0','0','0','0','0','0','1','-','0','0'}, .IOSupport="J",
    .rom_start=0,
    .rom_end=0x0003FFFF,
    .ram_start=0x00FF0000, .ram_end=0x00FFFFFF,
    .notes="ENEMY STATES AND BOUNDED NAVIGATION", .region="JUE"
};
