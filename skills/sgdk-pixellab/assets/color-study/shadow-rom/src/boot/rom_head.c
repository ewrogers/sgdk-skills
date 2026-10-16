#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header={
    .console="SEGA MEGA DRIVE",.copyright="(C)26 E.ROGERS",
    .title_local="PIXELLAB COLOR STUDY",.title_int="PIXELLAB COLOR STUDY",
    .serial="GM 0000000-00",.IOSupport="J",
    .rom_start=0,.rom_end=0x0003FFFF,
    .ram_start=0x00FF0000,.ram_end=0x00FFFFFF,
    .notes="FOUR PALETTE LINES - NO RASTER",.region="JUE"
};
