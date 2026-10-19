#include "everdrive_link.h"

bool hl_ed_signature(u16 first,u16 second)
{
    return (first & 0xfff0)==0x55a0 && (second & 0xfff0)==0x55a0 &&
           ((first ^ second) & 8)!=0;
}

void hl_ed_init(HlEdLink *link,bool present,u16 hz)
{
    link->pending_at=0;link->confirmed_at=0;link->received_at=0;link->nonce=0;link->used=0;
    for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)link->packet[i]=0;
    link->hz=hz==50 ? 50 : 60;
    link->present=present;link->pending=false;
    link->verified=false;link->ever_verified=false;
}

void hl_ed_tick(HlEdLink *link,u32 now)
{
    if(link->verified && (u32)(now-link->confirmed_at)>=link->hz*HL_ED_LEASE_SECONDS)
        link->verified=false;
    if(link->pending && (u32)(now-link->pending_at)>=link->hz)link->pending=false;
    if(link->used && (u32)(now-link->received_at)>=link->hz)link->used=0;
}

static u16 checksum(const u8 *packet)
{
    u16 crc=0xffff;
    for(u16 i=0;i<10;i++) {
        crc^=(u16)packet[i]<<8;
        for(u16 bit=0;bit<8;bit++)crc=(u16)((crc&0x8000) ? (crc<<1)^0x1021 : crc<<1);
    }
    return crc;
}

void hl_ed_packet(u8 *out,u16 kind,u32 nonce)
{
    out[0]='H';out[1]='L';out[2]='V';out[3]='1';
    out[4]=(u8)(kind>>8);out[5]=(u8)kind;
    for(u16 i=0;i<4;i++)out[6+i]=(u8)(nonce>>(24-8*i));
    const u16 crc=checksum(out);out[10]=(u8)(crc>>8);out[11]=(u8)crc;
}

bool hl_ed_receive(HlEdLink *link,u8 incoming,u32 now,u8 *reply)
{
    if(!link->present)return false;
    hl_ed_tick(link,now);
    static const u8 magic[4]={'H','L','V','1'};
    if(link->used<4 && incoming!=magic[link->used]) {
        link->used=incoming=='H' ? 1 : 0;link->packet[0]='H';return false;
    }
    link->packet[link->used++]=incoming;link->received_at=now;
    if(link->used<HL_ED_PACKET_BYTES)return false;
    link->used=0;
    const u8 *p=link->packet;
    if(checksum(p)!=((u16)p[10]<<8|p[11]))return false;
    const u16 kind=(u16)p[4]<<8|p[5];
    u32 nonce=0;for(u16 i=0;i<4;i++)nonce=nonce<<8|p[6+i];
    if(kind==HL_ED_REQUEST && nonce) {
        link->nonce=nonce;link->pending_at=now;link->pending=true;
        hl_ed_packet(reply,HL_ED_RESPONSE,nonce ^ HL_ED_XOR);return true;
    }
    if(link->pending && kind==HL_ED_ACK && nonce==link->nonce) {
        link->pending=false;link->verified=true;link->ever_verified=true;
        link->confirmed_at=now;hl_ed_packet(reply,HL_ED_DONE,nonce);return true;
    }
    return false;
}
