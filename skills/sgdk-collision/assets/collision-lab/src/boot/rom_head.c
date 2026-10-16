#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header={
    .console="SEGA MEGA DRIVE",.copyright="(C)26 SGDK LAB",
    .title_local="COLLISION LAB",.title_int="COLLISION LAB",
    .serial="GM 0000001-00",.IOSupport="J",.rom_start=0,.rom_end=0x0001FFFF,
    .ram_start=0x00FF0000,.ram_end=0x00FFFFFF,
    .notes="BOUNDED COLLISION QUERIES",.region="JUE"
};
