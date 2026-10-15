#include "water_split.h"
#include "raster_model.h"

/* Compile-time I/O seam for host behavior tests; no virtual calls on target. */
#ifdef RASTER_TEST
void raster_test_control(u16 value);
void raster_test_command(u32 value);
void raster_test_data(u32 value);
u16 raster_test_status(void);
u16 raster_test_line(void);
#define CONTROL(v) raster_test_control(v)
#define COMMAND(v) raster_test_command(v)
#define DATA(v) raster_test_data(v)
#define STATUS() raster_test_status()
#define LINE() raster_test_line()
#else
#define CONTROL(v) (*(vu16 *)VDP_CTRL_PORT = (v))
#define COMMAND(v) (*(vu32 *)VDP_CTRL_PORT = (v))
#define DATA(v) (*(vu32 *)VDP_DATA_PORT = (v))
#define STATUS() (*(vu16 *)VDP_CTRL_PORT)
#define LINE() GET_VCOUNTER
#endif

enum { POLL_LIMIT = 1024 };
_Static_assert(POLL_LIMIT == 1024, "Keep the assembly edge-wait bound and host protocol in sync");
static u16 dry_palette[32];
static u32 wet_pairs[16];
static volatile u16 armed_line, blank_command, display_command, increment_command, no_hint_command;
static volatile u16 late_count, timeout_count, last_line, setup_late;
static volatile bool armed, installed;

/* Preload command/colors BEFORE the edge wait. Only three register-to-port
   longword writes follow the fresh HBlank edge. Generic C may reload RAM too
   late; this is the only hand-timed assembly seam in the lab. */
static inline __attribute__((always_inline)) bool write_chunk(u32 command, u32 first, u32 second)
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
        "move.l %[first],(%[data])\n"
        "move.l %[second],(%[data])\n"
        "moveq #1,%[success]\n"
        "bra.s 6f\n"
        "5: moveq #0,%[success]\n"
        "6:\n"
        : [polls] "=&d" (polls), [success] "=&d" (success)
        : [command] "d" (command), [first] "d" (first), [second] "d" (second),
          [control] "a" (VDP_CTRL_PORT), [data] "a" (VDP_DATA_PORT)
        : "cc", "memory");
    return success != 0;
#else
    /* Host tests exercise the same bounded clear/rise protocol with fake I/O.
       Target disassembly checks cover the interrupt ABI and assembly seam. */
    bool clear = FALSE, rising = FALSE;
    for (u16 poll = 0; poll < POLL_LIMIT; ++poll)
        if (!(STATUS() & VDP_HBLANK_FLAG)) { clear = TRUE; break; }
    if (!clear) return FALSE;
    for (u16 poll = 0; poll < POLL_LIMIT; ++poll)
        if (STATUS() & VDP_HBLANK_FLAG) { rising = TRUE; break; }
    if (!rising) return FALSE;
    COMMAND(command);
    DATA(first); DATA(second);
    return TRUE;
#endif
}

void water_split_init(const u16 *dry, const u16 *wet)
{
    VDP_setHInterrupt(FALSE);
    for (u16 i = 0; i < 32; ++i) dry_palette[i] = dry[i];
    for (u16 i = 0; i < 16; ++i) wet_pairs[i] = ((u32)wet[i * 2] << 16) | wet[i * 2 + 1];
    late_count = timeout_count = last_line = setup_late = 0;
    armed = FALSE;
    installed = TRUE;
    SYS_setHIntCallback(water_split_interrupt);
}

void water_split_prepare(u16 line, bool enabled)
{
    VDP_setHInterrupt(FALSE);
    armed = FALSE;
    if (!installed) return;
    /* A late frame must not restore CRAM during active display. Skip its split. */
    if (!(STATUS() & VDP_VBLANK_FLAG)) { ++setup_late; ++late_count; return; }
    PAL_setColors(0, dry_palette, 32, CPU);
    if (!enabled || line < RASTER_MIN_LINE || line > RASTER_MAX_LINE) return;
    display_command = 0x8100 | VDP_getReg(1);
    blank_command = display_command & (u16)~0x40;
    increment_command = 0x8F00 | VDP_getReg(15);
    no_hint_command = 0x8000 | (VDP_getReg(0) & (u16)~0x10);
    armed_line = line;
    VDP_setHIntCounter(line - 1); /* Counter N schedules each N+1 lines. */
    armed = TRUE;
    VDP_setHInterrupt(TRUE);
}

HINTERRUPT_CALLBACK water_split_interrupt(void)
{
    /* SGDK JMPs here directly. GCC's interrupt ABI must emit RTE, not RTS. */
    /* A pending H interrupt can arrive immediately when re-enabled in VBlank.
       Do not consume the armed active-display split on that stale event. The
       status flag also handles PAL's V-counter wrap during blanking. */
    if (STATUS() & VDP_VBLANK_FLAG) return;
    CONTROL(no_hint_command); /* One palette split, not one callback per line. */
    if (!armed) return;
    armed = FALSE;
    u16 line = LINE();
    last_line = line;
    if (line + 1 < armed_line || line > armed_line + 1) { ++late_count; return; }
    CONTROL(blank_command);
    bool settled = FALSE;
    /* Let forced blank take effect before CRAM writes. Starting the burst
       immediately can still expose pixels from the VDP's rendering pipeline. */
    for (u16 poll = 0; poll < POLL_LIMIT; ++poll) {
        u16 status = STATUS();
        if (LINE() >= line + 1 &&
            (status & (VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG)) == (VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG)) {
            settled = TRUE;
            break;
        }
    }
    if (!settled) { ++timeout_count; goto restore; }
    CONTROL(0x8F02); /* CRAM autoincrement two; restore the saved R15 later. */
    /* Display blank alone does not hide CRAM dots. Transfer four colors per
       fresh HBlank across eight lines, rather than bursting during visible pixels. */
    for (u16 chunk = 0; chunk < 8; ++chunk)
        if (!write_chunk(VDP_WRITE_CRAM_ADDR(chunk * 8), wet_pairs[chunk * 2], wet_pairs[chunk * 2 + 1])) {
            ++timeout_count;
            goto restore;
        }
    settled = FALSE;
    /* Wait for a later HBlank and an empty FIFO. Bounds avoid a stuck ROM on
       an unexpected timing path; display is restored even if this times out. */
    for (u16 poll = 0; poll < POLL_LIMIT; ++poll) {
        u16 status = STATUS();
        if (LINE() >= line + 3 &&
            (status & (VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG)) == (VDP_FIFOEMPTY_FLAG | VDP_HBLANK_FLAG)) {
            settled = TRUE;
            break;
        }
    }
    if (!settled) ++timeout_count;
restore:
    CONTROL(increment_command);
    CONTROL(display_command);
}

void water_split_stop(void)
{
    SYS_disableInts();
    VDP_setHInterrupt(FALSE);
    armed = FALSE;
    installed = FALSE;
    SYS_setHIntCallback(NULL);
    SYS_enableInts();
}

u16 water_split_late(void) { return late_count; }
u16 water_split_timeouts(void) { return timeout_count; }
u16 water_split_last_line(void) { return last_line; }
u16 water_split_setup_late(void) { return setup_late; }
