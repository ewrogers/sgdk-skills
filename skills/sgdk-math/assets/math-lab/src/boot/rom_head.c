#include <genesis.h>

__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console = "SEGA MEGA DRIVE", .copyright = "(C)2026 SGDK",
    .title_local = "SGDK MATH LAB", .title_int = "SGDK MATH LAB",
    .serial = {'G','M',' ','0','0','0','0','0','0','0','1','-','0','0'},
    .IOSupport = "J", .rom_start = 0, .rom_end = 0x0001FFFF,
    .ram_start = 0x00FF0000, .ram_end = 0x00FFFFFF,
    .notes = "FIXED POINT STARFIELD AND ORBITS", .region = "JUE"
};
