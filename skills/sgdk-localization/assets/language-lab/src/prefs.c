#include <genesis.h>
#include "loc.h"
#include "prefs.h"

u8 prefs_load(void)
{
    u8 bytes[6],id=0;
    SRAM_enableRO();
    for (u16 i=0;i<6;i++) bytes[i]=SRAM_readByte(i);
    SRAM_disable();
    return loc_pref_decode(bytes,&id) ? id : 0;
}

void prefs_save(u8 id)
{
    u8 bytes[6];
    if (!loc_pref_encode(id,bytes)) return;
    SRAM_enable();
    SRAM_writeByte(5,0); /* Invalidate first; commit only the complete record. */
    for (u16 i=0;i<5;i++) SRAM_writeByte(i,bytes[i]);
    SRAM_writeByte(5,bytes[5]);
    SRAM_disable();
}
