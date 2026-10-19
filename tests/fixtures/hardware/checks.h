#include "hardware.h"
#include "everdrive_link.h"
#define CHECK(x) do {if(!(x))return __LINE__;} while(0)
static bool feed_packet(HlEdLink *link,u16 kind,u32 nonce,u32 now,u8 *reply)
{
    u8 packet[HL_ED_PACKET_BYTES];hl_ed_packet(packet,kind,nonce);
    bool wrote=false;
    for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)wrote|=hl_ed_receive(link,packet[i],now,reply);
    return wrote;
}
static bool matches_packet(const u8 *actual,u16 kind,u32 nonce)
{
    u8 expected[HL_ED_PACKET_BYTES];hl_ed_packet(expected,kind,nonce);
    for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)if(actual[i]!=expected[i])return false;
    return true;
}
static int check_ed_link(void)
{
    CHECK(hl_ed_signature(0x55a0,0x55a8));
    CHECK(hl_ed_signature(0x55af,0x55a7));
    CHECK(!hl_ed_signature(0x55a0,0x55a0));
    CHECK(!hl_ed_signature(0xffff,0xffff) && !hl_ed_signature(0,8));
    CHECK(!hl_ed_signature(0x55a0,0x55b8));
    HlEdLink link;u8 reply[HL_ED_PACKET_BYTES]={0};
    hl_ed_init(&link,false,60);
    CHECK(!feed_packet(&link,HL_ED_REQUEST,0x12345678,1,reply));
    CHECK(!link.verified && !link.pending && !link.ever_verified);
    for(u16 hz=50;hz<=60;hz+=10) {
        const u32 start=0xfffffff0u;
        hl_ed_init(&link,true,hz);
        CHECK(!feed_packet(&link,HL_ED_ACK,0x12345678,start,reply) && !link.verified);
        CHECK(!feed_packet(&link,HL_ED_REQUEST,0,start,reply));
        CHECK(feed_packet(&link,HL_ED_REQUEST,0x12345678,start,reply));
        CHECK(matches_packet(reply,HL_ED_RESPONSE,0x12345678^HL_ED_XOR));
        CHECK(link.pending && !link.verified);
        CHECK(!feed_packet(&link,HL_ED_ACK,0x12345679,start+1,reply) && !link.verified);
        CHECK(feed_packet(&link,HL_ED_ACK,0x12345678,start+2,reply));
        CHECK(matches_packet(reply,HL_ED_DONE,0x12345678));
        CHECK(link.verified && link.ever_verified && !link.pending);
        hl_ed_tick(&link,start+2+hz*5-1);CHECK(link.verified);
        hl_ed_tick(&link,start+2+hz*5);CHECK(!link.verified);
        CHECK(link.ever_verified);
        CHECK(feed_packet(&link,HL_ED_REQUEST,456,400,reply));
        CHECK(!feed_packet(&link,HL_ED_ACK,456,400+hz,reply));
        CHECK(!link.verified && !link.pending);
        CHECK(feed_packet(&link,HL_ED_REQUEST,789,500,reply));
        CHECK(feed_packet(&link,HL_ED_REQUEST,321,501,reply));
        CHECK(!feed_packet(&link,HL_ED_ACK,789,502,reply));
        CHECK(feed_packet(&link,HL_ED_ACK,321,503,reply) && link.verified);
        hl_ed_init(&link,true,hz);
        CHECK(!link.verified && !link.ever_verified && !link.pending);
        u8 packet[HL_ED_PACKET_BYTES];hl_ed_packet(packet,HL_ED_REQUEST,123);
        packet[10]^=1;
        for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)CHECK(!hl_ed_receive(&link,packet[i],600,reply));
        CHECK(!link.pending);
        CHECK(!hl_ed_receive(&link,'H',600,reply));hl_ed_tick(&link,600+hz);CHECK(link.used==0);
        CHECK(!hl_ed_receive(&link,0xff,700,reply));
        CHECK(!hl_ed_receive(&link,'H',700,reply));
        CHECK(feed_packet(&link,HL_ED_REQUEST,0xffffffff,701,reply));
    }
    return 0;
}
static int check_fields(void)
{
    char line[HL_FIELD_WIDTH+1];
    CHECK(hl_format_field(line,"P1 HELD: ","0FFF"));
    const char *expected="P1 HELD: 0FFF";
    for(u16 i=0;i<13;i++)CHECK(line[i]==expected[i]);
    for(u16 i=13;i<HL_FIELD_WIDTH;i++)CHECK(line[i]==' ');
    CHECK(line[HL_FIELD_WIDTH]==0);
    CHECK(hl_format_field(line,"SEQUENCE: ","4294967295"));
    CHECK(hl_format_field(line,"SEQUENCE: ","1"));
    CHECK(line[10]=='1');for(u16 i=11;i<HL_FIELD_WIDTH;i++)CHECK(line[i]==' ');
    CHECK(hl_format_field(line,"012345678901234567890123456789012345",""));
    for(u16 i=0;i<HL_FIELD_WIDTH+1;i++)line[i]='?';
    CHECK(!hl_format_field(line,"012345678901234567890123456789012345","X"));
    for(u16 i=0;i<HL_FIELD_WIDTH+1;i++)CHECK(line[i]=='?');
    CHECK(!hl_format_field(0,"A","B") && !hl_format_field(line,0,"B") && !hl_format_field(line,"A",0));
    return 0;
}
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
