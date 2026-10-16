#include "hardware.h"
void hl_clock_init(HlClock *c,u32 now)
{
    *c=(HlClock){now,0,0,0,0};
}
u16 hl_clock_sample(HlClock *c,u32 now,u16 hz)
{
    u32 delta=now-c->last;
    if(hz!=50 && hz!=60)return 0;
    c->last=now;c->elapsed+=delta;
    if(delta>1)c->missed+=delta-1;
    if(delta>c->max_gap)c->max_gap=delta>65535 ? 65535 : (u16)delta;
    /* Bound work: report missed intervals rather than doing unbounded catch-up. */
    if(!delta)return 0;
    c->phase+=60;
    u16 ticks=c->phase/hz;c->phase%=hz;
    return ticks;
}
bool hl_row_fits(u16 pieces,u16 width,u16 screen_width)
{
    if(screen_width!=256 && screen_width!=320)return false;
    return width && width<=32 && !(width&7) && pieces<=screen_width/16 &&
        (u32)pieces*width<=screen_width;
}
bool hl_upload_fits(u16 address,u16 words)
{
    return !(address&1) && words && address>=HL_UPLOAD_START &&
        (u32)address+(u32)words*2<=HL_UPLOAD_END;
}
static u16 crc16(const u8 *data,u16 length)
{
    u16 crc=0xffff;
    for(u16 i=0;i<length;i++) {
        crc^=(u16)data[i]<<8;
        for(u16 b=0;b<8;b++)crc=(u16)((crc&0x8000) ? (crc<<1)^0x1021 : crc<<1);
    }
    return crc;
}
void hl_encode(u8 *p,u32 sequence)
{
    p[0]='H';p[1]='W';p[2]='L';p[3]=1;
    for(u16 i=0;i<4;i++)p[4+i]=(u8)(sequence>>(24-i*8));
    p[8]=0x55;p[9]=0xaa;
    u16 crc=crc16(p,10);p[10]=(u8)(crc>>8);p[11]=(u8)crc;
}
bool hl_decode(const u8 *p,u32 *sequence)
{
    if(!p || !sequence || p[0]!='H' || p[1]!='W' || p[2]!='L' || p[3]!=1 ||
        p[8]!=0x55 || p[9]!=0xaa || crc16(p,10)!=((u16)p[10]<<8|p[11]))return false;
    u32 result=0;for(u16 i=0;i<4;i++)result=result<<8|p[4+i];
    *sequence=result;return true;
}
