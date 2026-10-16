#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header={
    .console="SEGA MEGA DRIVE",.copyright="(C)26 SGDK LAB",
    .title_local="HARDWARE LAB",.title_int="HARDWARE LAB",
    .serial="GM 0000060-00",.IOSupport="J",.rom_start=0,.rom_end=0x0001FFFF,
    .ram_start=0x00FF0000,.ram_end=0x00FFFFFF,
    .sram_sig={'R','A'},.sram_type=0xF820,
    .sram_start=0x00200001,.sram_end=0x0020FFFF,
    .notes="OPT-IN SRAM TEST FIRST 12 BYTES",.region="JUE"
};
