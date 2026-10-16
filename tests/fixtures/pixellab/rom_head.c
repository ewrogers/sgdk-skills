#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console="SEGA MEGA DRIVE", .copyright="(C)26 E.ROGERS",
    .title_local="PIXELLAB ASSET PROBE", .title_int="PIXELLAB ASSET PROBE",
    .serial="GM 0000000-00", .IOSupport="J",
    .rom_start=0, .rom_end=0x0001FFFF,
    .ram_start=0x00FF0000, .ram_end=0x00FFFFFF,
    .notes="SGDK SKILLS ART RESOURCE PROBE", .region="JUE"
};
