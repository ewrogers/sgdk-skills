#include <genesis.h>

__attribute__((externally_visible))
const ROMHeader rom_header = {
    "SEGA SSF        ",
    "(C)SGDK LAB 2026",
    "BANK LAB                                        ",
    "BANK LAB                                        ",
    "GM 00000000-00",
    0,
    "J               ",
    0x00000000, 0x005FFFFF,
    0xE0FF0000, 0xE0FFFFFF,
    "RA", 0xF820, 0x00200001, 0x00203FFF,
    "            ",
    "FIXED COMMON AND BANKED STAGE DATA      ",
    "JUE             "
};
