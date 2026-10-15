#include "checkpoint.h"
u16 checkpoint_crc(const u8 *data, u16 size)
{
    u16 crc=0xFFFF;
    for (u16 i=0; i<size; ++i) {
        crc ^= (u16)data[i] << 8;
        for (u16 b=0; b<8; ++b) crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}
static void put16(u8 *data, u16 value) { data[0]=value >> 8; data[1]=value; }
static u16 get16(const u8 *data) { return (u16)data[0] << 8 | data[1]; }
bool checkpoint_encode(const Checkpoint *value, u32 sequence, u8 data[CHECKPOINT_SLOT_BYTES])
{
    if (!value || !data || !sequence || !lab_run_valid(&value->run)) return false;
    for (u16 i=0; i<CHECKPOINT_SLOT_BYTES; ++i) data[i]=0;
    data[0]='T'; data[1]='R'; data[2]='N'; data[3]='S'; data[4]=2;
    data[5]=value->run.joined; data[6]=value->run.room;
    data[7]=value->run.character[0] | value->run.character[1] << 1;
    put16(data+8, sequence >> 16); put16(data+10, sequence);
    for (u16 p=0; p<2; ++p) {
        put16(data+12+p*2, value->run.score[p]);
        put16(data+16+p*4, value->run.x[p]); put16(data+18+p*4, value->run.y[p]);
    }
    data[24]=value->run.coins; data[25]=value->music; data[26]=value->effects;
    data[27]=value->run.carried[0]; data[28]=value->run.carried[1]; data[29]=value->run.boss_state;
    put16(data+44, checkpoint_crc(data,44)); data[46]=0xA5; data[47]=0x5A;
    return true;
}
bool checkpoint_decode(const u8 data[CHECKPOINT_SLOT_BYTES], Checkpoint *out, u32 *sequence)
{
    if (!data || !out || !sequence || data[0]!='T' || data[1]!='R' || data[2]!='N' || data[3]!='S' ||
        data[4]!=2 || data[7]>3 || data[25]>1 || data[26]>1 ||
        data[46]!=0xA5 || data[47]!=0x5A || get16(data+44)!=checkpoint_crc(data,44)) return false;
    for (u16 i=30;i<44;++i) if (data[i]) return false;
    Checkpoint value={0};
    value.run.joined=data[5]; value.run.room=data[6]; value.run.coins=data[24];
    value.run.character[0]=data[7]&1; value.run.character[1]=(data[7]>>1)&1;
    value.run.carried[0]=data[27]; value.run.carried[1]=data[28]; value.run.boss_state=data[29];
    value.music=data[25]!=0; value.effects=data[26]!=0;
    for (u16 p=0; p<2; ++p) {
        value.run.score[p]=get16(data+12+p*2);
        value.run.x[p]=get16(data+16+p*4); value.run.y[p]=get16(data+18+p*4);
    }
    u32 serial=(u32)get16(data+8)<<16 | get16(data+10);
    if (!serial || !lab_run_valid(&value.run)) return false;
    *out=value; *sequence=serial; return true;
}
static bool read_slot(const CheckpointIo *io, u16 slot, u8 *bytes)
{
    for (u16 i=0; i<CHECKPOINT_SLOT_BYTES; ++i)
        if (!io->read(slot*CHECKPOINT_SLOT_BYTES+i,bytes+i)) return false;
    return true;
}
static bool same(const u8 *a, const u8 *b)
{
    for (u16 i=0; i<CHECKPOINT_SLOT_BYTES; ++i) if (a[i]!=b[i]) return false;
    return true;
}
/* Equal sequence numbers with different bytes are an ambiguous save, not a guess. */
static bool inspect(const CheckpointIo *io, Checkpoint values[2], u32 seq[2], bool valid[2], u16 *newest)
{
    u8 a[CHECKPOINT_SLOT_BYTES], b[CHECKPOINT_SLOT_BYTES];
    if (!io || !io->read || !read_slot(io,0,a) || !read_slot(io,1,b)) return false;
    valid[0]=checkpoint_decode(a,values,seq); valid[1]=checkpoint_decode(b,values+1,seq+1);
    if (valid[0] && valid[1] && seq[0]==seq[1] && !same(a,b)) return false;
    *newest=valid[1] && (!valid[0] || seq[1]>=seq[0]) ? 1 : 0;
    return true;
}
bool checkpoint_load(const CheckpointIo *io, Checkpoint *out)
{
    Checkpoint values[2]; u32 seq[2]={0}; bool valid[2]; u16 newest;
    if (!out || !inspect(io,values,seq,valid,&newest) || (!valid[0] && !valid[1])) return false;
    *out=values[newest]; return true;
}
bool checkpoint_store(const CheckpointIo *io, const Checkpoint *value)
{
    Checkpoint values[2], decoded; u32 seq[2]={0}, serial; bool valid[2]; u16 newest;
    u8 bytes[CHECKPOINT_SLOT_BYTES], check[CHECKPOINT_SLOT_BYTES];
    if (!io || !io->write || !inspect(io,values,seq,valid,&newest)) return false;
    serial=(valid[0] || valid[1]) ? seq[newest] : 0;
    if (serial==0xFFFFFFFFUL || !checkpoint_encode(value,serial+1,bytes)) return false;
    u16 slot=(valid[0] || valid[1]) ? newest^1 : 0, base=slot*CHECKPOINT_SLOT_BYTES;
    if (!io->write(base+CHECKPOINT_SLOT_BYTES-1,0)) return false;
    for (u16 i=0; i<CHECKPOINT_SLOT_BYTES; ++i) if (!io->write(base+i,bytes[i])) return false;
    if (!read_slot(io,slot,check) || !same(bytes,check)) return false;
    return checkpoint_decode(check,&decoded,&serial);
}
