#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "split_model.h"
#include "split_video.h"

static VoidCallback *callback;
static bool enabled, frozen;
static u16 status_value, status_reads, line_value, control, command_count, data_count;
static u16 vertical[2], window_y, window_down, counter, increment, depth;
static u32 command, data;
void VDP_setHInterrupt(bool value) { enabled = value; }
void VDP_setHIntCounter(u8 value) { counter = value; }
void VDP_setAutoInc(u8 value) { increment = value; }
void VDP_setVerticalScroll(VDPPlane plane, s16 value) { assert(plane < 2); vertical[plane] = value; }
void VDP_setWindowVPos(u16 down, u16 position) { window_down = down; window_y = position; }
u8 VDP_getReg(u16 index) { assert(index == 0); return 4; }
void SYS_setHIntCallback(VoidCallback *cb) { callback = cb; }
void SYS_disableInts(void) { ++depth; }
void SYS_enableInts(void) { assert(depth); --depth; }
void split_test_control(u16 value) { control = value; }
void split_test_command(u32 value) { command = value; ++command_count; }
void split_test_data(u32 value) { data = value; ++data_count; }
u16 split_test_line(void) { return line_value; }
u16 split_test_status(void)
{
    ++status_reads;
    return !frozen && (status_reads & 1) ? status_value & (u16)~VDP_HBLANK_FLAG : status_value;
}

static void model(void)
{
    SplitModel a, b; split_model_init(&a); split_model_init(&b);
    a.cpu = b.cpu = FALSE;
    for (u16 i = 0; i < 60; ++i) assert(split_model_step(&a, SPLIT_RIGHT, SPLIT_UP, 60));
    for (u16 i = 0; i < 50; ++i) assert(split_model_step(&b, SPLIT_RIGHT, SPLIT_UP, 50));
    assert(a.actor[0].x == 216 && a.actor[0].y == 96);
    assert(a.actor[1].x == 384 && a.actor[1].y == 136);
    for (u16 i = 0; i < 2; ++i) {
        assert(a.actor[i].x == b.actor[i].x && a.actor[i].y == b.actor[i].y);
        assert(a.actor[i].camera_x == b.actor[i].camera_x && a.actor[i].camera_y == b.actor[i].camera_y);
    }
    split_model_step(&a, SPLIT_PAUSE, 0, 60);
    s16 before = a.actor[0].x;
    for (u16 i = 0; i < 60; ++i) split_model_step(&a, SPLIT_PAUSE | SPLIT_RIGHT, SPLIT_UP, 60);
    assert(a.paused && a.actor[0].x == before);
    split_model_step(&a, 0, 0, 60); split_model_step(&a, SPLIT_PAUSE, 0, 60);
    for (u16 i = 0; i < 600; ++i) split_model_step(&a, SPLIT_LEFT | SPLIT_UP, SPLIT_DOWN | SPLIT_RIGHT, 60);
    assert(a.actor[0].x == 0 && a.actor[0].y == 0);
    assert(a.actor[1].x == 496 && a.actor[1].y == 496);
    assert(a.actor[0].camera_y == 0 && a.actor[1].camera_y == 420);
    s16 x, y;
    assert(split_actor_position(&a, 0, 0, &x, &y) && y == 16);
    assert(split_actor_position(&a, 1, 1, &x, &y) && y + 16 == 208);
    SplitModel old = a;
    assert(!split_model_step(&a, 0, 0, 0));
    assert(a.actor[0].x == old.actor[0].x && a.clock == old.clock);
    split_model_step(&a, SPLIT_CPU | SPLIT_RESET, 0, 60);
    assert(a.cpu && a.actor[0].x == 96);
    for (u16 i = 0; i < 2000; ++i) {
        split_model_step(&a, SPLIT_CPU, 0, 60); /* Held A cannot retrigger. */
        assert(a.cpu && a.phase < 768);
        assert(a.actor[1].x >= 128 && a.actor[1].x <= 384 && a.actor[1].y >= 128 && a.actor[1].y <= 256);
    }
}

static void views(void)
{
    SplitModel s; split_model_init(&s);
    s.actor[0].camera_x = 12; s.actor[1].camera_x = 190;
    struct { s16 before, values[224], after; } table;
    table.before = 123; table.after = 456;
    assert(split_scroll(&s, table.values, 224));
    for (u16 y = 0; y < 224; ++y) assert(table.values[y] == (y < 112 ? -12 : -190));
    assert(table.before == 123 && table.after == 456);
    assert(!split_scroll(&s, table.values, 225));
    assert(!split_scroll(&s, NULL, 224));
    for (u16 y = 0; y <= 420; ++y) {
        s.actor[0].camera_y = s.actor[1].camera_y = y;
        assert(((split_top_scroll(&s) + 16) & 511) == y);
        assert(((split_bottom_scroll(&s) + 116) & 511) == y);
    }
    s.actor[0] = (SplitActor){10, 10, 0, 0};
    s.actor[1] = (SplitActor){10, 105, 0, 100};
    s16 x, y;
    assert(split_actor_position(&s, 0, 0, &x, &y) && y == 26);
    assert(!split_actor_position(&s, 0, 1, &x, &y));
    assert(split_actor_position(&s, 1, 1, &x, &y) && y == 121);
    assert(!split_actor_position(&s, 1, 0, &x, &y));
    s.actor[1].x = 305;
    assert(!split_actor_position(&s, 1, 1, &x, &y));
    s.actor[1].x = 304;
    assert(split_actor_position(&s, 1, 1, &x, &y));
    assert(!split_actor_position(&s, 2, 0, &x, &y));
}

static void video(void)
{
    split_video_init(); assert(!enabled && callback == split_video_interrupt);
    status_value = VDP_VBLANK_FLAG;
    split_video_prepare(42, 400);
    assert(enabled && increment == 2 && counter == 109);
    assert(vertical[0] == 42 && vertical[1] == 0 && window_y == 2 && !window_down);
    line_value = 0; callback(); /* PAL-wrap stale event is still VBlank. */
    assert(!command_count && !split_video_late());
    status_value = VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG;
    line_value = 109; callback();
    assert(command_count == 1 && data_count == 1);
    assert(command == VDP_WRITE_VSRAM_ADDR(0) && data == (400u << 16));
    assert(control == 0x929A && !split_video_late() && !split_video_timeouts());
    callback(); assert(command_count == 1); /* One-shot. */
    status_value = 0; split_video_prepare(1, 2);
    assert(!enabled && split_video_late() == 1);
    status_value = VDP_VBLANK_FLAG; split_video_prepare(1, 2);
    status_value = 0; line_value = 120; callback();
    assert(command_count == 1 && split_video_late() == 2);
    status_value = VDP_VBLANK_FLAG; split_video_prepare(1, 2);
    status_value = VDP_HBLANK_FLAG; line_value = 109; frozen = TRUE; status_reads = 0;
    callback();
    assert(split_video_timeouts() == 1 && status_reads <= 1025 && command_count == 1);
    status_value = VDP_VBLANK_FLAG; split_video_prepare(1, 2);
    status_value = 0; line_value = 109; status_reads = 0; callback();
    assert(split_video_timeouts() == 2 && status_reads <= 1026 && command_count == 1);
    split_video_stop(); assert(!callback && !enabled && !depth);
    split_video_prepare(1, 2); assert(!enabled);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "split-model")) model();
    else if (!strcmp(argv[1], "split-views")) views();
    else if (!strcmp(argv[1], "split-video")) video();
    else return 2;
    puts(argv[1]); return 0;
}
