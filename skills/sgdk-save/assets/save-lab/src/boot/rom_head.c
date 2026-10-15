#include <genesis.h>

/* 32 KiB, odd-address SRAM. Only the first 64 logical bytes belong to this lab. */
__attribute__((externally_visible))
const ROMHeader rom_header = {
    .console = "SEGA MEGA DRIVE",
    .copyright = "(C)2026 SGDK",
    .title_local = "SGDK SAVE LAB",
    .title_int = "SGDK SAVE LAB",
    .serial = "GM 00000001-00",
    .IOSupport = "J",
    .rom_start = 0,
    .rom_end = 0x000FFFFF,
    .ram_start = 0x00FF0000,
    .ram_end = 0x00FFFFFF,
    .sram_sig = {'R', 'A'},
    .sram_type = 0xF820,
    .sram_start = 0x00200001,
    .sram_end = 0x0020FFFF,
    .notes = "TWO SLOT SAVE AND PASSWORD LAB",
    .region = "JUE"
};
