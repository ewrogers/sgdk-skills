/* Fixed-width cartridge metadata intentionally has no trailing NUL. */
#if defined(__GNUC__) && __GNUC__ >= 16
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunterminated-string-initialization"
#endif
#include "genesis.h"

__attribute__((externally_visible))
const ROMHeader rom_header = {
#if (ENABLE_BANK_SWITCH != 0)
    "SEGA SSF        ",
#elif (MODULE_MEGAWIFI != 0)
    "SEGA MEGAWIFI   ",
#else
    "SEGA MEGA DRIVE ",
#endif
    "(C)DESK 2026    ",
    "LAST SERVICE                                    ",
    "LAST SERVICE                                    ",
    "GM 00000000-00",
    0x000,
    "J               ",
    0x00000000,
#if (ENABLE_BANK_SWITCH != 0)
    0x003FFFFF,
#else
    0x0001FFFF,
#endif
    0xE0FF0000,
    0xE0FFFFFF,
    "  ",
    0x0000,
    0x00000000,
    0x00000000,
    "            ",
    "NIGHT TRAM DISPATCH / 1998              ",
    "JUE             "
};

#if defined(__GNUC__) && __GNUC__ >= 16
#pragma GCC diagnostic pop
#endif
