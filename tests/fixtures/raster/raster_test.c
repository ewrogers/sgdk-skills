#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "raster_model.h"
#include "water_split.h"

static bool enabled;
static u8 counter, regs[16];
static VoidCallback *callback;
static u16 dry_copy[32], writes[64], write_count, palette_calls, depth;
static u32 pairs[16], cram_command;
static u16 pair_count, status_reads, line_reads, current_line, status_value;
static bool frozen_clock, starve_after_data;

void VDP_setHInterrupt(bool value) { enabled = value; }
void VDP_setHIntCounter(u8 value) { counter = value; }
u8 VDP_getReg(u16 index) { assert(index < 16); return regs[index]; }
void SYS_setHIntCallback(VoidCallback *cb) { callback = cb; }
void SYS_disableInts(void) { ++depth; }
void SYS_enableInts(void) { assert(depth); --depth; }
void PAL_setColors(u16 index, const u16 *data, u16 count, TransferMethod method)
{
    assert(index == 0 && count == 32 && method == CPU);
    memcpy(dry_copy, data, sizeof dry_copy);
    ++palette_calls;
}
void raster_test_control(u16 value) { assert(write_count < 64); writes[write_count++] = value; }
void raster_test_command(u32 value) { cram_command = value; }
void raster_test_data(u32 value) { assert(pair_count < 16); pairs[pair_count++] = value; }
u16 raster_test_status(void)
{
    ++status_reads;
    if (starve_after_data && pair_count) return 0;
    /* Simulate fresh HBlank edges; VBlank and FIFO bits stay independent. */
    return (status_value & VDP_HBLANK_FLAG) && (status_reads & 1) ?
           status_value & (u16)~VDP_HBLANK_FLAG : status_value;
}
u16 raster_test_line(void)
{
    ++line_reads;
    return frozen_clock ? current_line : current_line + line_reads - 1;
}

static void reset_io(void)
{
    write_count = pair_count = status_reads = line_reads = palette_calls = 0;
    frozen_clock = FALSE;
    starve_after_data = FALSE;
    status_value = VDP_VBLANK_FLAG;
    regs[0] = 0x04; regs[1] = 0x74; regs[15] = 4;
}

static void model(void)
{
    RasterModel n, p;
    raster_model_init(&n); raster_model_init(&p);
    for (u16 i = 0; i < 60; ++i) assert(raster_model_step(&n, RASTER_RIGHT, 60));
    for (u16 i = 0; i < 50; ++i) assert(raster_model_step(&p, RASTER_RIGHT, 50));
    assert(n.phase == 60 && p.phase == 60 && n.camera_x == 120 && p.camera_x == 120);
    for (u16 i = 0; i < 1000; ++i) raster_model_step(&n, RASTER_UP | RASTER_LEFT, 60);
    assert(n.waterline == RASTER_MIN_LINE && n.camera_x == 0);
    for (u16 i = 0; i < 1000; ++i) raster_model_step(&n, RASTER_DOWN | RASTER_RIGHT, 60);
    assert(n.waterline == RASTER_MAX_LINE && n.camera_x == 704 && n.phase < 256);
    RasterModel before = n;
    assert(!raster_model_step(&n, 0, 0) && memcmp(&n, &before, sizeof n) == 0);
    raster_model_step(&n, RASTER_PAUSE, 60);
    u16 phase = n.phase;
    for (u16 i = 0; i < 100; ++i) raster_model_step(&n, RASTER_PAUSE | RASTER_UP, 60);
    assert(n.paused && n.phase == phase && n.waterline == RASTER_MAX_LINE);
    raster_model_step(&n, RASTER_TINT | RASTER_WAVES | RASTER_MUSIC, 60);
    assert(!n.tint && !n.waves && !n.music && n.paused);
    raster_model_step(&n, RASTER_TINT | RASTER_WAVES | RASTER_MUSIC, 60);
    assert(!n.tint && !n.waves && !n.music);
    raster_model_step(&n, 0, 60);
    raster_model_step(&n, RASTER_PAUSE, 60);
    assert(!n.paused && n.phase == (phase + 1) % 256);
}

static void tables(void)
{
    RasterModel s; raster_model_init(&s);
    struct { s16 before, data[224], after; } table;
    table.before = 1234; table.after = -1234;
    for (u16 camera = 0; camera <= 704; camera += 176) {
        s.camera_x = camera;
        for (u16 phase = 0; phase < 256; ++phase) {
            s.phase = phase;
            for (u16 line = 64; line <= 192; line += 64) {
                s.waterline = line;
                assert(raster_scroll(&s, table.data, 224));
                for (u16 y = 0; y < 224; ++y) {
                    s16 delta = table.data[y] + camera;
                    assert(y >= line ? delta >= -4 && delta <= 4 : delta == 0);
                }
                assert(table.before == 1234 && table.after == -1234);
            }
        }
    }
    s.waves = FALSE;
    assert(raster_scroll(&s, table.data, 224));
    for (u16 y = 0; y < 224; ++y) assert(table.data[y] == -(s16)s.camera_x);
    s16 snapshot[224]; memcpy(snapshot, table.data, sizeof snapshot);
    assert(!raster_scroll(&s, table.data, 225));
    assert(memcmp(snapshot, table.data, sizeof snapshot) == 0);
    s.waterline = 10;
    assert(!raster_scroll(&s, table.data, 224));
    assert(!raster_scroll(&s, NULL, 224));
    u16 source[32], target[34];
    target[0] = 1234; target[33] = 4321;
    for (u16 c = 0; c < 512; ++c) {
        u16 v = ((c & 7) << 1) | (((c >> 3) & 7) << 5) | (((c >> 6) & 7) << 9);
        for (u16 i = 0; i < 32; ++i) source[i] = v;
        assert(raster_tint(source, target + 1, 32));
        assert(target[1] == 0 && target[17] == 0);
        for (u16 i = 1; i <= 32; ++i) assert((target[i] & ~0x0EEE) == 0);
        assert(target[0] == 1234 && target[33] == 4321);
    }
    assert(!raster_tint(source, target, 33));
    assert(!raster_tint(NULL, target, 32));
}

static void split(void)
{
    u16 dry[32], wet[32];
    for (u16 i = 0; i < 32; ++i) { dry[i] = i * 2; wet[i] = 0x600 | (i * 2); }
    reset_io(); water_split_init(dry, wet);
    assert(callback == water_split_interrupt && !enabled);
    water_split_prepare(128, TRUE);
    assert(enabled && counter == 127 && palette_calls == 1);
    assert(memcmp(dry_copy, dry, sizeof dry) == 0);
    current_line = 236;
    callback(); /* A pending VBlank H-int must not consume the active split. */
    assert(!pair_count && water_split_late() == 0);
    current_line = 0; callback(); /* PAL V-counter wrap is still VBlank. */
    assert(!pair_count && water_split_late() == 0);
    memset(wet, 0, sizeof wet); /* Handler must own its prepacked copy. */
    current_line = 127;
    status_value = VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG;
    callback();
    assert(pair_count == 16 && cram_command == VDP_WRITE_CRAM_ADDR(56));
    for (u16 i = 0; i < 16; ++i)
        assert(pairs[i] == ((u32)(0x600 | (i * 4)) << 16 | (0x600 | (i * 4 + 2))));
    assert(writes[0] == 0x8004 && writes[1] == 0x8134 && writes[2] == 0x8F02);
    assert(writes[write_count - 2] == 0x8F04 && writes[write_count - 1] == 0x8174);
    assert(water_split_late() == 0 && water_split_timeouts() == 0);
    u16 written = pair_count;
    callback(); assert(pair_count == written); /* Repeated interrupt is a no-op. */
    reset_io(); water_split_prepare(64, TRUE); assert(counter == 63);
    water_split_prepare(192, TRUE); assert(counter == 191);
    water_split_prepare(63, TRUE); assert(!enabled);
    water_split_prepare(193, TRUE); assert(!enabled);
    water_split_prepare(128, FALSE); assert(!enabled);
    assert(palette_calls == 5); /* Dry palette restored even with tint disabled. */
    water_split_stop(); assert(callback == NULL && !enabled && !depth);
    u16 calls = palette_calls;
    water_split_prepare(128, TRUE); /* A remaining frame callback cannot rearm. */
    assert(!enabled && palette_calls == calls);
}

static void failure(void)
{
    u16 palette[32] = {0};
    reset_io(); water_split_init(palette, palette);
    status_value = 0;
    water_split_prepare(128, TRUE);
    assert(!enabled && !palette_calls && water_split_late() == 1);
    reset_io(); water_split_prepare(128, TRUE);
    current_line = 134; status_value = 0; callback();
    assert(!pair_count && water_split_late() == 2);
    reset_io(); water_split_prepare(128, TRUE);
    current_line = 127; frozen_clock = TRUE; status_value = 0;
    callback();
    assert(pair_count == 0 && status_reads <= 1026);
    assert(water_split_timeouts() == 1);
    assert(writes[write_count - 1] == 0x8174); /* Always restore display. */
    reset_io(); water_split_prepare(128, TRUE);
    current_line = 127; status_value = VDP_HBLANK_FLAG; /* FIFO never drains. */
    callback();
    assert(water_split_timeouts() == 2 && writes[write_count - 1] == 0x8174);
    reset_io(); water_split_prepare(128, TRUE);
    current_line = 127; status_value = VDP_HBLANK_FLAG | VDP_FIFOEMPTY_FLAG;
    starve_after_data = TRUE; /* Completion timeout after the palette burst. */
    callback();
    assert(pair_count == 2 && water_split_timeouts() == 3);
    assert(writes[write_count - 1] == 0x8174);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "model")) model();
    else if (!strcmp(argv[1], "tables")) tables();
    else if (!strcmp(argv[1], "split")) split();
    else if (!strcmp(argv[1], "failure")) failure();
    else return 2;
    puts(argv[1]);
    return 0;
}
