#include <genesis.h>
/* Two 48-byte records use the first 96 logical SRAM bytes. */
__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console="SEGA MEGA DRIVE", .copyright="(C)2026 SGDK",
    .title_local="SGDK TRANSITION LAB", .title_int="SGDK TRANSITION LAB",
    .serial={'G','M',' ','0','0','0','0','0','0','0','1','-','0','0'}, .IOSupport="J",
    .rom_start=0, .rom_end=0x0003FFFF, .ram_start=0x00FF0000, .ram_end=0x00FFFFFF,
    .sram_sig={'R','A'}, .sram_type=0xF820, .sram_start=0x00200001, .sram_end=0x0020FFFF,
    .notes="SCENES AND RESOURCE OWNERSHIP LAB", .region="JUE"
};
