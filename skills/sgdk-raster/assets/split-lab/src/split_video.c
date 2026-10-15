#include "split_video.h"

#ifdef RASTER_TEST
void split_test_control(u16 value);
void split_test_command(u32 value);
void split_test_data(u32 value);
u16 split_test_status(void);
u16 split_test_line(void);
#define CONTROL(v) split_test_control(v)
#define COMMAND(v) split_test_command(v)
#define DATA(v) split_test_data(v)
#define STATUS() split_test_status()
#define LINE() split_test_line()
#else
#define CONTROL(v) (*(vu16 *)VDP_CTRL_PORT = (v))
#define STATUS() (*(vu16 *)VDP_CTRL_PORT)
#define LINE() GET_VCOUNTER
#endif

static volatile u16 no_hint, bottom_window, late, timeouts;
static volatile u32 bottom_values;
static volatile bool armed, installed;

static inline __attribute__((always_inline)) bool write_split(u32 values, u16 window)
{
#ifndef RASTER_TEST
    u16 polls, success;
    __asm__ volatile(
        "move.w #1023,%[polls]\n"
        "1: move.w (%[control]),%[success]\n"
        "btst #2,%[success]\n"
        "beq.s 2f\n"
        "dbra %[polls],1b\n"
        "bra.s 5f\n"
        "2: move.w #1023,%[polls]\n"
        "3: move.w (%[control]),%[success]\n"
        "btst #2,%[success]\n"
        "bne.s 4f\n"
        "dbra %[polls],3b\n"
        "bra.s 5f\n"
        "4: move.l %[command],(%[control])\n"
        "move.l %[values],(%[data])\n"
        "move.w %[window],(%[control])\n"
        "moveq #1,%[success]\n"
        "bra.s 6f\n"
        "5: moveq #0,%[success]\n"
        "6:\n"
        : [polls] "=&d" (polls), [success] "=&d" (success)
        : [command] "d" (VDP_WRITE_VSRAM_ADDR(0)), [values] "d" (values), [window] "d" (window),
          [control] "a" (VDP_CTRL_PORT), [data] "a" (VDP_DATA_PORT)
        : "cc", "memory");
    return success != 0;
#else
    bool clear = FALSE, rising = FALSE;
    for (u16 i = 0; i < 1024; ++i)
        if (!(STATUS() & VDP_HBLANK_FLAG)) { clear = TRUE; break; }
    if (!clear) return FALSE;
    for (u16 i = 0; i < 1024; ++i)
        if (STATUS() & VDP_HBLANK_FLAG) { rising = TRUE; break; }
    if (!rising) return FALSE;
    COMMAND(VDP_WRITE_VSRAM_ADDR(0)); DATA(values); CONTROL(window);
    return TRUE;
#endif
}

void split_video_init(void)
{
    VDP_setHInterrupt(FALSE);
    late = timeouts = 0;
    installed = TRUE; armed = FALSE;
    SYS_setHIntCallback(split_video_interrupt);
}

void split_video_prepare(u16 top_scroll, u16 bottom_scroll)
{
    VDP_setHInterrupt(FALSE); armed = FALSE;
    if (!installed) return;
    if (!(STATUS() & VDP_VBLANK_FLAG)) { ++late; return; }
    VDP_setAutoInc(2);
    VDP_setVerticalScroll(BG_A, top_scroll & 511);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_setWindowVPos(FALSE, 2);
    no_hint = 0x8000 | (VDP_getReg(0) & (u16)~0x10);
    bottom_values = (u32)(bottom_scroll & 511) << 16; /* A then blank B. */
    bottom_window = 0x9200 | 0x80 | 26; /* Bottom two HUD rows; keep SGDK's top cache. */
    /* Prepare slightly before midpoint; the opaque 108..115 divider hides
       the fetch/latch transition without disabling the whole display. */
    VDP_setHIntCounter(109);
    armed = TRUE;
    VDP_setHInterrupt(TRUE);
}

HINTERRUPT_CALLBACK split_video_interrupt(void)
{
    if (STATUS() & VDP_VBLANK_FLAG) return;
    CONTROL(no_hint);
    if (!armed) return;
    armed = FALSE;
    u16 line = LINE();
    if (line < 108 || line > 111) { ++late; return; }
    if (!write_split(bottom_values, bottom_window)) ++timeouts;
}

void split_video_stop(void)
{
    SYS_disableInts(); VDP_setHInterrupt(FALSE);
    installed = armed = FALSE; SYS_setHIntCallback(NULL);
    SYS_enableInts();
}
u16 split_video_late(void) { return late; }
u16 split_video_timeouts(void) { return timeouts; }
