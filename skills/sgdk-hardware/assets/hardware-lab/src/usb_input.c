#include "usb_input.h"

static UiU16 get16(const UiU8 *p){return (UiU16)p[0]<<8|p[1];}
static UiU32 get32(const UiU8 *p){return (UiU32)get16(p)<<16|get16(p+2);}
static void put16(UiU8 *p,UiU16 n){p[0]=(UiU8)(n>>8);p[1]=(UiU8)n;}
static void put32(UiU8 *p,UiU32 n){put16(p,(UiU16)(n>>16));put16(p+2,(UiU16)n);}
static UiU16 crc(const UiU8 *p){
    UiU16 value=0xffff;
    for(UiU16 i=0;i<38;i++){
        value^=(UiU16)p[i]<<8;
        for(UiU16 b=0;b<8;b++)value=(UiU16)((value&0x8000)?(value<<1)^0x1021:value<<1);
    }
    return value;
}
void ui_init(UsbInput *u,UiU8 present,UiU16 hz){
    /* Also explicit on SGDK warm entry, whose globals may retain old bytes. */
    UiU8 *p=(UiU8 *)u;for(UiU16 i=0;i<sizeof(*u);i++)p[i]=0;
    u->present=present!=0;u->hz=hz==50?50:60;
}
static UiU8 zero(const UiU8 *p,UiU16 size){
    for(UiU16 i=0;i<size;i++)if(p[i])return 0;
    return 1;
}
static void clear_port(UiOverride *p){p->kind=0;p->held=0;p->dx=0;p->dy=0;p->ttl=0;}
static UiU8 command(UsbInput *u,UiU32 now){
    const UiU8 *p=u->wire;UiU8 kind=p[4],mask=p[5];
    UiU16 ttl=get16(p+6);UiU32 session=get32(p+8),seq=get32(p+12);
    if(!zero(p+32,6))return UI_BAD_ARGUMENT;
    if(kind==UI_OPEN){
        if(!session || seq || mask || ttl || !zero(p+16,16))return UI_BAD_ARGUMENT;
        u->session=session;u->sequence=0;
        for(UiU16 i=0;i<2;i++)clear_port(&u->override[i]);
        return UI_OK;
    }
    if(!session || session!=u->session)return UI_BAD_SESSION;
    if(!seq || seq<=u->sequence)return UI_BAD_SEQUENCE;
    if(kind!=UI_STATE && kind!=UI_RELEASE && kind!=UI_STATUS)return UI_BAD_COMMAND;
    if(kind==UI_STATUS){
        if(mask || ttl || !zero(p+16,16))return UI_BAD_ARGUMENT;
    }else{
        if(!mask || mask>3)return UI_BAD_ARGUMENT;
        if(kind==UI_RELEASE){
            if(ttl || !zero(p+16,16))return UI_BAD_ARGUMENT;
        }else{
            if(!ttl || ttl>UI_MAX_TICKS)return UI_BAD_ARGUMENT;
            /* Validate both ports completely before changing either. */
            for(UiU16 i=0;i<2;i++){
                const UiU8 *v=p+16+i*8;
                if(!(mask&(1u<<i))){if(!zero(v,8))return UI_BAD_ARGUMENT;continue;}
                UiU16 held=get16(v+2);UiS16 dx=(UiS16)get16(v+4),dy=(UiS16)get16(v+6);
                if(v[1] || (v[0]!=UI_PAD && v[0]!=UI_MOUSE))return UI_BAD_ARGUMENT;
                if(v[0]==UI_PAD){if((held&0xf000) || dx || dy)return UI_BAD_ARGUMENT;}
                else if((held&0xfff0) || dx < -2047 || dx>2047 || dy < -2047 || dy>2047)return UI_BAD_ARGUMENT;
            }
        }
    }
    u->sequence=seq;
    for(UiU16 i=0;i<2;i++)if(mask&(1u<<i)){
        UiOverride *v=&u->override[i];
        if(kind==UI_RELEASE)clear_port(v);
        else if(kind==UI_STATE){
            const UiU8 *p0=p+16+i*8;
            v->kind=p0[0];v->held=get16(p0+2);v->dx=(UiS16)get16(p0+4);v->dy=(UiS16)get16(p0+6);
            v->since=now;v->ttl=ttl;
        }
    }
    return UI_OK;
}
UiU8 ui_feed(UsbInput *u,UiU8 byte,UiU32 now){
    if(!u->present)return 0;
    if(u->used && (UiU32)(now-u->received_at)>=u->hz)u->used=0;
    u->received_at=now;
    u->wire[u->used++]=byte;
    if(u->used<4)return 0;
    UiU8 legacy=u->wire[0]=='H' && u->wire[1]=='L' && u->wire[2]=='V' && u->wire[3]=='1';
    UiU8 input=u->wire[0]=='U' && u->wire[1]=='I' && u->wire[2]=='V' && u->wire[3]=='1';
    if(!legacy && !input){
        for(UiU16 i=0;i<3;i++)u->wire[i]=u->wire[i+1];
        u->used=3;return 0;
    }
    if(u->used<(legacy?12:UI_BYTES))return 0;
    u->used=0;
    if(legacy)return 1;
    if(crc(u->wire)!=get16(u->wire+38))return 0;
    /* The caller's 32-byte budget allows at most one 40-byte command/frame. */
    if(u->ack_pending)return 0;
    u->ack_session=get32(u->wire+8);u->ack_sequence=get32(u->wire+12);
    u->ack_code=command(u,now);u->ack_pending=1;return 0;
}
void ui_step(UsbInput *u,UiU32 now,const UiPort physical[2]){
    u->tick=now;
    for(UiU16 i=0;i<2;i++){
        UiOverride *r=&u->override[i];UiPort next=physical[i],old=u->port[i];
        if(!u->present || (r->kind && (UiU32)(now-r->since)>=r->ttl))clear_port(r);
        next.remote=0;
        if(r->kind){next.kind=r->kind;next.held=r->held;next.dx=r->dx;next.dy=r->dy;next.remote=1;r->dx=0;r->dy=0;}
        next.pressed=next.kind==old.kind?(UiU16)(next.held&~old.held):next.held;
        next.released=next.kind==old.kind?(UiU16)(old.held&~next.held):old.held;
        u->port[i]=next;
    }
}
UiU8 ui_reply(UsbInput *u,UiU8 out[UI_BYTES]){
    if(!u->ack_pending)return 0;
    u->ack_pending=0;for(UiU16 i=0;i<UI_BYTES;i++)out[i]=0;
    out[0]='U';out[1]='I';out[2]='V';out[3]='1';out[4]=UI_ACK;
    put32(out+8,u->ack_session);put32(out+12,u->ack_sequence);
    UiU16 remaining=0;
    for(UiU16 i=0;i<2;i++){
        const UiPort *p=&u->port[i];UiU8 *v=out+16+i*8;
        v[0]=p->kind;v[1]=p->remote;put16(v+2,p->held);put16(v+4,(UiU16)p->dx);put16(v+6,(UiU16)p->dy);
        if(p->remote){
            out[5]|=(UiU8)(1u<<i);
            UiU16 ticks=(UiU16)(u->override[i].ttl-(UiU32)(u->tick-u->override[i].since));
            if(ticks>remaining)remaining=ticks;
        }
    }
    put16(out+6,remaining);put32(out+32,u->tick);out[36]=u->ack_code;out[37]=3;
    put16(out+38,crc(out));return 1;
}
