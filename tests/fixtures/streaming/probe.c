#include <genesis.h>
#include "resources.h"
#include "checks.h"
#include "stream_io.h"
#include "palettes.h"

__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int error=check_core();if(error)return error;
    error=check_world();if(error)return error;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(StreamCache);*port=sizeof(ScSlot);*port=SC_SLOTS;*port=SC_CHUNK_TILES;
    for(u16 z=0;z<4;z++)for(u16 i=0;i<16;i++)*port=area_colors[z][i];
    for(u16 i=0;i<16;i++)*port=hero_colors[i];
    u16 scratch[128];
    for(u16 page=0;page<SC_PAGES;page++)for(u16 offset=0;offset<512;offset+=256) {
        if(!sc_rom_read(0,page,offset,scratch,256))return 1000;
        for(u16 i=0;i<256;i++)if(((u8 *)scratch)[i]!=stream_pages[page*512+offset+i])return 1001;
    }
    for(u16 i=0;i<sizeof(stream_pages);i+=2)*port=(u16)stream_pages[i]<<8 | stream_pages[i+1];
    for(u16 i=0;i<sizeof(stream_hero);i+=2)*port=(u16)stream_hero[i]<<8 | stream_hero[i+1];
    return 0;
}
