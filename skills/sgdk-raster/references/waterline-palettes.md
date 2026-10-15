# Change colors below the surface

## What makes it Sonic-style

The world and sprite keep their tiles and color indexes. Above the waterline,
PAL0/PAL1 contain their dry colors. Below it, the handler changes those same
entries to precomputed blue-tinted colors. Water-effect sprites use PAL2 and the
HUD uses PAL3; both stay unchanged.
VBlank restores the dry colors before the next frame begins.

This does not allocate two more hardware palette banks: it reuses the same two
at different times. The lab occupies all four banks in each region
of the screen. Colors can differ across the frame, but a pixel still uses the
normal VDP palette/index rules.

Sonic 1's disassembly shows an H-interrupt handler writing all 64 CRAM colors.
It also documents shifting VBlank work when an early waterline would make the
split late. That is evidence of the technique, not proof that a generic C loop
can upload 64 colors invisibly inside a single HBlank.

## Why the lab has a transition band

CRAM writes can produce colored dots during visible output. Temporarily
disabling display does not by itself hide those dots and also hides sprites.
The lab keeps display enabled and transfers two colors per fresh HBlank,
skipping unchanged pairs. Black outlines and unused black entries stay black.
Its current art needs eight pairs; a guard rejects changes that exceed the
surface's coverage. An opaque animated foam strip on PAL2 conceals the rows
where only part of the world palette has changed. This is not translucency.

The small assembly routine preloads one packed color pair and its CRAM
command before waiting for HBlank to clear and rise again. It then emits two
register-to-port longword writes. Merely entering an H interrupt is not enough:
the interrupt request and HBlank flag have different timing.

Both edge waits are bounded. The C wrapper also bounds its settling/completion
waits and restores the saved autoincrement state on failure. Display remains
enabled throughout. A timeout
can leave a partial palette for the rest of that frame; next VBlank restores
the dry palette. The diagnostic makes this failure visible instead of hanging.

This trades visible surface space and main-CPU time for a more conservative
example. The ten surface sprites use the full H40 sprite-pixel budget on their
rows, intentionally occluding the robot during the crossing. Do not generalize
its timing to H32, interlace, another clock or
an arbitrary number of colors. If a game needs a tighter seam, profile a smaller
set of changed colors and inspect the actual target code before tuning it.

## Arm and restore once per frame

Use `HINTERRUPT_CALLBACK` for the function installed through
`SYS_setHIntCallback()`. SGDK's vector jumps directly to it, so the compiler
must preserve interrupt state and emit `RTE`. A normal `void` callback returning
with `RTS` is not interchangeable.

The lab installs an early V interrupt through `SYS_setVIntCallback()`. It restores
the dry palette, snapshots VDP register commands and sets counter `line - 1`
before the main loop flushes queued DMA. SGDK's later `SYS_setVBlankCallback()`
is used only for occasional HUD updates. Restoring colors there instead can be
too late if earlier transfer work uses the blanking budget: the previous frame's
underwater colors can carry into the whole next frame.

`water_split_publish()` publishes the next line and tint flag as one aligned
16-bit word. Publish it with the completed sprite/scroll queue before releasing
the main-loop critical section. The V interrupt latches that word for the frame;
later model changes cannot move an already armed split. Do not copy an entire
mutable game model from an interrupt and assume it describes the displayed frame.

An already pending interrupt during VBlank is ignored without consuming the
armed split. During active display, the first expected event disables further
H interrupts and performs the switch. An unexpectedly late event is skipped.

The handler owns PAL0/PAL1 and temporarily changes autoincrement.
Saved commands preserve the other register bits and restore the final state
expected by SGDK's cache. Do not run an independent fade or palette uploader
over these banks without composing both dry and wet variants deliberately.

For teardown, disable H interrupts and detach the H/V interrupt callbacks with
`water_split_stop()`. Also remove the ordinary HUD VBlank callback before releasing data.
Drain queued transfers while their owners
still exist. Restore normal colors while blanked or in VBlank. The standalone
lab owns these global callbacks; a larger game must coordinate their ownership.

## Sources

- SGDK 2.11 [interrupt ABI and callback contract](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
  [callback/queue order](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
  [early vertical interrupt entry](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/sega.s),
  [official H-interrupt sample](https://github.com/Stephane-D/SGDK/blob/v2.11/sample/fx/h-int/scaling/src/main.c).
- [VDP counter/display controls](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
  [cached register implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c),
  [generic palette transfer path](https://github.com/Stephane-D/SGDK/blob/v2.11/src/pal.c).
- [Sonic 1 HBlank and VBlank code](https://github.com/sonicretro/s1disasm/blob/a30c85abe29ad8f238630e0ed507e587ffea0e97/sonic.asm).
- Plutiedev [VDP commands](https://plutiedev.com/vdp-commands) and
  [VDP registers](https://plutiedev.com/vdp-registers).
- Ares's accurate VDP implementation: [CRAM write bus](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/md/vdp/memory.cpp),
  [HBlank/IRQ timing and dot emission](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/md/vdp/main.cpp)
  and [DAC output](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/md/vdp/dac.cpp).
  These explain the emulator behavior; they do not replace console validation.
