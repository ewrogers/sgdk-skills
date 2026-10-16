#include <genesis.h>

__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console = "SEGA MEGA DRIVE", .copyright = "(C)26 E.ROGERS",
    .title_local = "SGDK C CONTRACTS", .title_int = "SGDK C CONTRACTS",
    .serial = "GM 0000000-00", .IOSupport = "J",
    .rom_start = 0, .rom_end = 0x0001FFFF,
    .ram_start = 0x00FF0000, .ram_end = 0x00FFFFFF,
    .notes = "NONINTERACTIVE CODE CHECKS", .region = "JUE"
};
