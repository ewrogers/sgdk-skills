#include <genesis.h>
/* The demo owns 80 logical SRAM bytes, not a serialized C struct. */
__attribute__((externally_visible))
const ROMHeader rom_header = {.console = "SEGA MEGA DRIVE",
                              .copyright = "(C)26 E.ROGERS",
                              .title_local = "THE LAST BELL - ADVENTURE LAB",
                              .title_int = "THE LAST BELL - ADVENTURE LAB",
                              .serial = "GM 0000001-00",
                              .IOSupport = "J",
                              .rom_start = 0,
                              .rom_end = 0x000FFFFF,
                              .ram_start = 0x00FF0000,
                              .ram_end = 0x00FFFFFF,
                              .sram_sig = {'R', 'A'},
                              .sram_type = 0xF820,
                              .sram_start = 0x00200001,
                              .sram_end = 0x0020FFFF,
                              .notes = "ORIGINAL RPG EXAMPLE - SGDK SKILLS",
                              .region = "JUE"};
