#include <genesis.h>
#include "checks.h"
#include "resources.h"
#include "font.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int e=check_clock();if(e)return e;e=check_budgets();if(e)return e;e=check_records();if(e)return e;e=check_fields();if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(HlClock);*port=sizeof(reference_music);*port=sizeof(reference_pcm);
    u8 record[HL_RECORD_BYTES];hl_encode(record,0x12345678);
    for(u16 i=0;i<HL_RECORD_BYTES;i++)*port=record[i];
    for(u16 i=0;i<768;i++){*port=(u16)(font_tiles[i]>>16);*port=(u16)font_tiles[i];}
    return 0;
}
