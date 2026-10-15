#include <assert.h>
#include <string.h>
#include "checkpoint.h"

static u8 memory[CHECKPOINT_BYTES];
static int budget=-1;
static bool read_byte(u16 offset,u8 *value)
{
    if (offset>=sizeof memory) return false;
    *value=memory[offset]; return true;
}
static bool write_byte(u16 offset,u8 value)
{
    if (offset>=sizeof memory || !budget) return false;
    if (budget>0) --budget;
    memory[offset]=value; return true;
}
static const CheckpointIo io={read_byte,write_byte};
static Checkpoint example(void)
{
    Lab lab; lab_init(&lab); lab.run.joined=3; lab.run.character[0]=1; lab.run.character[1]=0;
    lab.run.room=1; lab.run.carried[0]=0x80; lab.run.score[0]=4; lab.run.score[1]=5; lab.run.coins=0x7F;
    return (Checkpoint){lab.run,false,true};
}
static void equal(const Checkpoint *a,const Checkpoint *b)
{
    assert(a->music==b->music && a->effects==b->effects);
    assert(a->run.joined==b->run.joined && a->run.room==b->run.room && a->run.coins==b->run.coins && a->run.boss_state==b->run.boss_state);
    for (u16 p=0;p<2;++p) assert(a->run.x[p]==b->run.x[p] && a->run.y[p]==b->run.y[p] &&
        a->run.score[p]==b->run.score[p] && a->run.character[p]==b->run.character[p] && a->run.carried[p]==b->run.carried[p]);
}
void test_checkpoint_format(void)
{
    Checkpoint value=example(),out; u32 seq=0;
    u8 bytes[48],damaged[48];
    assert(checkpoint_crc((const u8 *)"123456789",9)==0x29B1);
    assert(checkpoint_encode(&value,73,bytes)); assert(checkpoint_decode(bytes,&out,&seq));
    assert(seq==73); equal(&value,&out);
    for (u16 bit=0;bit<384;++bit) {
        memcpy(damaged,bytes,48); damaged[bit/8]^=1u<<(bit%8);
        Checkpoint before=out; u32 previous=seq;
        assert(!checkpoint_decode(damaged,&out,&seq)); equal(&before,&out); assert(seq==previous);
    }
    value.run.x[0]=500; assert(!checkpoint_encode(&value,1,bytes));
    value=example(); assert(!checkpoint_encode(&value,0,bytes));
    for (u16 field=4;field<44;++field) {
        memcpy(damaged,bytes,48); damaged[field]=field==17 ? 0 : 255;
        u16 crc=checkpoint_crc(damaged,44); damaged[44]=crc>>8; damaged[45]=crc;
        if (field>=8 && field<=11) continue; /* A larger sequence is valid. */
        assert(!checkpoint_decode(damaged,&out,&seq));
    }
}
void test_checkpoint_recovery(void)
{
    memset(memory,255,sizeof memory); Checkpoint first=example(),second=first,out;
    assert(!checkpoint_load(&io,&out)); assert(checkpoint_store(&io,&first));
    u8 baseline[96]; memcpy(baseline,memory,96); second.run.x[0]=200;
    for (int cutoff=0;cutoff<49;++cutoff) {
        memcpy(memory,baseline,96); budget=cutoff;
        assert(!checkpoint_store(&io,&second)); budget=-1;
        assert(checkpoint_load(&io,&out)); equal(&out,&first);
    }
    memcpy(memory,baseline,96); assert(checkpoint_store(&io,&second));
    assert(checkpoint_load(&io,&out)); equal(&out,&second);
    memory[48+16]^=1; assert(checkpoint_load(&io,&out)); equal(&out,&first);
    /* Equal versions with conflicting contents must not silently pick a winner. */
    assert(checkpoint_encode(&second,1,memory+48)); assert(!checkpoint_load(&io,&out));
    assert(!checkpoint_store(&io,&second));
    memset(memory,255,sizeof memory); assert(checkpoint_encode(&first,0xFFFFFFFFUL,memory));
    u8 before[96]; memcpy(before,memory,96);
    assert(!checkpoint_store(&io,&second) && !memcmp(before,memory,96));
}
