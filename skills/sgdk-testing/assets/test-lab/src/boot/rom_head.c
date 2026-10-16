#include <genesis.h>

__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console = "SEGA MEGA DRIVE",
    .copyright = "(C)26 SGDK LAB",
    .title_local = "TEST LAB - SEEDED COURIER",
    .title_int = "TEST LAB - SEEDED COURIER",
    .serial = "GM 0000001-00",
    .IOSupport = "J",
    .rom_start = 0,
    .rom_end = 0x0001FFFF,
    .ram_start = 0x00FF0000,
    .ram_end = 0x00FFFFFF,
    .notes = "DETERMINISTIC REPLAY AND REGRESSION LAB",
    .region = "JUE"
};
