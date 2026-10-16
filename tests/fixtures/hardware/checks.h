#include "hardware.h"
#define CHECK(x) do {if(!(x))return __LINE__;} while(0)
static int check_clock(void)
{
    for(u16 hz=50;hz<=60;hz+=10) {
        HlClock c;hl_clock_init(&c,0xfffffff0u);u32 ticks=0;
        for(u16 i=1;i<=hz*10;i++)ticks+=hl_clock_sample(&c,0xfffffff0u+i,hz);
        CHECK(ticks==600 && c.elapsed==hz*10u && c.missed==0 && c.max_gap==1 && c.phase==0);
        CHECK(hl_clock_sample(&c,c.last,hz)==0);
        u32 last=c.last;CHECK(hl_clock_sample(&c,last+3,0)==0 && c.last==last);
        CHECK(hl_clock_sample(&c,last+3,hz)>=1 && c.missed==2 && c.max_gap==3);
        CHECK(hl_clock_sample(&c,c.last+100000, hz)<=2 && c.max_gap==65535 && c.missed==100001);
    }
    return 0;
}
static int check_budgets(void)
{
    CHECK(hl_row_fits(20,8,320) && !hl_row_fits(21,8,320));
    CHECK(hl_row_fits(10,32,320) && !hl_row_fits(11,32,320));
    CHECK(hl_row_fits(16,8,256) && !hl_row_fits(17,8,256));
    CHECK(hl_row_fits(8,32,256) && !hl_row_fits(9,32,256));
    CHECK(!hl_row_fits(1,0,320) && !hl_row_fits(65535,32,320));
    CHECK(!hl_row_fits(1,9,320) && !hl_row_fits(1,40,320) && !hl_row_fits(1,8,224));
    CHECK(hl_upload_fits(0x8000,1024) && hl_upload_fits(0x87fe,1));
    CHECK(!hl_upload_fits(0x8000,1025) && !hl_upload_fits(0x8001,1));
    CHECK(!hl_upload_fits(0x7ffe,1) && !hl_upload_fits(0x8800,1));
    CHECK(!hl_upload_fits(0xfffe,65535) && !hl_upload_fits(0x8000,0));
    return 0;
}
static int check_records(void)
{
    u8 record[HL_RECORD_BYTES];u32 out;
    const u32 values[]={0,1,0x12345678,0xffffffff};
    for(u16 v=0;v<4;v++) {
        hl_encode(record,values[v]);out=9;CHECK(hl_decode(record,&out) && out==values[v]);
        for(u16 bit=0;bit<HL_RECORD_BYTES*8;bit++) {
            record[bit/8]^=1u<<(bit%8);out=0xabcdef01;
            CHECK(!hl_decode(record,&out) && out==0xabcdef01);
            record[bit/8]^=1u<<(bit%8);
        }
    }
    for(u16 i=0;i<HL_RECORD_BYTES;i++)record[i]=0xff;
    CHECK(!hl_decode(record,&out) && !hl_decode(0,&out) && !hl_decode(record,0));
    return 0;
}
