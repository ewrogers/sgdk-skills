#include "checked.h"
#include "pair.h"
#include "pair.h"
#include "upload.h"
#include "upload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static void numbers(void)
{
    CHECK(bp_example_checks() == 0);
    for (u32 value = 0; value <= 65535u; value++)
    {
        CHECK(bp_square((u16)value) == (uint64_t)value * value);
        for (u32 amount = 0; amount <= 65535u; amount += 257)
        {
            uint64_t sum = (uint64_t)value + amount;
            CHECK(bp_add_clamped((u16)value,(u16)amount) == (sum > 65535u ? 65535u : sum));
        }
    }
    for (u32 start = 0; start <= 65535u; start += 257)
        for (u32 elapsed = 0; elapsed <= 65535u; elapsed += 257)
            CHECK(bp_elapsed((u16)(start + elapsed),(u16)start) == elapsed);
    for (u16 i = 0; i < 16; i++) { u16 mask = 0; CHECK(bp_layer(i,&mask) && mask == (1u << i)); }
    u16 mask = 999; CHECK(!bp_layer(32,&mask) && !bp_layer(65535,&mask) && mask == 999);
}

static void packets(void)
{
    u32 seed = 1234567;
    for (u16 i = 0; i < 4000; i++)
    {
        seed = seed * 1664525UL + 1013904223UL;
        BpRecord input = {seed, (u16)(seed % 1001), (u8)(seed & 7)}, output = {0,0,0};
        u8 bytes[10]; memset(bytes,0xA5,sizeof bytes);
        CHECK(bp_encode(&input,bytes+1,8));
        CHECK(bytes[0] == 0xA5 && bytes[9] == 0xA5);
        CHECK(bytes[1] == 1 && bytes[2] == input.flags);
        CHECK(bytes[3] == input.health / 256 && bytes[4] == input.health % 256);
        CHECK(bytes[5] == seed / 16777216UL && bytes[8] == seed % 256);
        CHECK(bp_decode(bytes+1,8,&output));
        CHECK(output.score == input.score && output.health == input.health && output.flags == input.flags);
    }
    BpRecord output = {42,100,3};
    u8 valid[8] = {1,7,3,232,255,255,255,255};
    for (u16 length = 0; length < 10; length++)
    {
        if (length == 8) continue;
        CHECK(!bp_decode(valid,length,&output));
        CHECK(output.score == 42 && output.health == 100 && output.flags == 3);
    }
    for (u16 which = 0; which < 3; which++)
    {
        u8 broken[8]; memcpy(broken,valid,8);
        if (which == 0) broken[0] = 2;
        if (which == 1) broken[1] = 8;
        if (which == 2) broken[2] = 4;
        CHECK(!bp_decode(broken,8,&output) && output.score == 42 && output.health == 100 && output.flags == 3);
    }
    for (u16 capacity = 0; capacity < 8; capacity++)
    {
        u8 bytes[8]; memset(bytes,0xA5,8); CHECK(!bp_encode(&output,bytes,capacity));
        for (u16 i = 0; i < 8; i++) CHECK(bytes[i] == 0xA5);
    }
    u8 bytes[8]; memset(bytes,0xA5,8); output.health = 1001;
    CHECK(!bp_encode(&output,bytes,8)); output.health = 1; output.flags = 8;
    CHECK(!bp_encode(&output,bytes,8));
    for (u16 i = 0; i < 8; i++) CHECK(bytes[i] == 0xA5);
    CHECK(!bp_decode(NULL,8,&output) && !bp_decode(valid,8,NULL));
    CHECK(!bp_encode(NULL,bytes,8) && !bp_encode(&output,NULL,8));
}

static Sprite sprites[2];
static unsigned acquired, released, fail_at, adds;
static u16 release_order[2];
Sprite *SPR_addSprite(const SpriteDefinition *definition, s16 x, s16 y, u16 attr)
{
    CHECK(definition && x == (adds == 0 ? 32 : 64) && y == 32 && attr == 0);
    adds++;
    if (adds == fail_at) return NULL;
    CHECK(acquired < 2); sprites[acquired].id = (u16)acquired; return &sprites[acquired++];
}
void SPR_releaseSprite(Sprite *sprite)
{
    CHECK(sprite >= sprites && sprite < sprites+2 && released < acquired);
    release_order[released++] = sprite->id;
}
static void ownership(void)
{
    const SpriteDefinition definition = {1};
    CHECK(!bp_pair_is_open()); bp_pair_close();
    CHECK(!bp_pair_open(NULL,&definition) && adds == 0);
    CHECK(!bp_pair_open(&definition,NULL) && adds == 0);
    for (fail_at = 1; fail_at <= 2; fail_at++)
    {
        adds = acquired = released = 0;
        CHECK(!bp_pair_open(&definition,&definition));
        CHECK(!bp_pair_is_open() && acquired == released && acquired == fail_at-1);
        bp_pair_close(); CHECK(released == acquired);
    }
    fail_at = 0;
    for (u16 cycle = 0; cycle < 64; cycle++)
    {
        adds = acquired = released = 0;
        CHECK(bp_pair_open(&definition,&definition) && bp_pair_is_open());
        CHECK(!bp_pair_open(&definition,&definition) && adds == 2);
        bp_pair_close(); CHECK(!bp_pair_is_open() && released == 2);
        CHECK(release_order[0] == 1 && release_order[1] == 0);
        bp_pair_close(); CHECK(released == 2);
    }
}

static u16 mask = 2, depth, saved_mask, copies, copied[16], destination;
static bool queue_full;
void SYS_disableInts(void) { if (depth++ == 0) { saved_mask = mask; mask = 7; } }
void SYS_enableInts(void) { CHECK(depth > 0); if (--depth == 0) mask = saved_mask; }
bool DMA_copyAndQueueDma(u8 location, void *source, u16 to, u16 words, u16 step)
{
    CHECK(mask == 7 && depth && location == DMA_CRAM && words == 16 && step == 2);
    copies++; if (queue_full) return false;
    memcpy(copied,source,sizeof copied); destination = to; return true;
}
static void uploads(void)
{
    u16 colors[16]; for (u16 i = 0; i < 16; i++) colors[i] = i * 2;
    CHECK(!bp_queue_palette(4,colors) && !bp_queue_palette(0,NULL) && copies == 0 && mask == 2);
    for (u16 slot = 0; slot < 4; slot++)
    {
        CHECK(bp_queue_palette(slot,colors) && destination == slot * 32 && mask == 2 && depth == 0);
        for (u16 i = 0; i < 16; i++) CHECK(copied[i] == i * 2);
    }
    colors[0] = 42; CHECK(copied[0] == 0);
    queue_full = true; CHECK(!bp_queue_palette(0,colors) && mask == 2 && depth == 0);
    queue_full = false;
    SYS_disableInts(); CHECK(bp_queue_palette(0,colors) && mask == 7 && depth == 1);
    SYS_enableInts(); CHECK(mask == 2 && depth == 0 && copied[0] == 42);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (!strcmp(argv[1],"numbers")) numbers();
    else if (!strcmp(argv[1],"packets")) packets();
    else if (!strcmp(argv[1],"ownership")) ownership();
    else if (!strcmp(argv[1],"uploads")) uploads();
    else return 2;
    puts(argv[1]); return 0;
}
