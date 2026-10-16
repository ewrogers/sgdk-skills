#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header={
    .console="SEGA MEGA DRIVE",.copyright="(C)26 SGDK LAB",
    .title_local="LANGUAGE LAB - THE WORD FERRY",.title_int="LANGUAGE LAB - THE WORD FERRY",
    .serial="GM 0000001-00",.IOSupport="J",.rom_start=0,.rom_end=0x0001FFFF,
    .ram_start=0x00FF0000,.ram_end=0x00FFFFFF,
    .sram_sig={'R','A'},.sram_type=0xF820,.sram_start=0x00200001,.sram_end=0x0020FFFF,
    .notes="UTF8 AUTHORING - NATIVE GLYPH RENDERING",.region="JUE"
};
