# Optional USB input validation — 2026-10-19

The testing skill owns the reusable UIV1 module, installer, Python client and
protocol. The input skill retains physical-driver and gameplay snapshot
ownership; hardware supplies an independently buildable adapter; project retains
deployment/reset. Only cooperating development ROMs include this optional hook.

## Build and contract evidence

Pulled `ewrogers/sgdk:latest`: SGDK 2.11, GCC 16.2.0, binutils
2.47.20260726, base 68000. Manifest
`1d8dd3f289ef9ed1c7186b68b47a1977bc02b2bcbd72903e7682f6b19ce7d0df`.
Public commands continue using the mutable tag.

The final host suite passed: 555 tests, 49 opt-in skips, including seven new USB
tests. Distribution checks enforce portable, independently installed resources;
cross-skill references use optional web links.
O0/O3 C contracts use UBSan; transcript tests exercise fragmented legacy/extended
identity, noise, stale/bad-CRC ACKs, rejection and a fresh session nonce.
Installer checks use an independently copied skill and verify no overwrite.

The release/debug and requested PAL/NTSC hardware matrix passed, executing
protocol assertions on Unicorn's original-68000 model. Coverage includes two
atomic slots, mixed pad/mouse, one-shot deltas, four mouse buttons, edges,
release/expiry, malformed requests, CRC, stale sequence, HLV1 multiplexing,
partial timeout and VInt wrap. Absent-interface tests access no FIFO MMIO.
`UsbInput` is at most 128 bytes; `UiPort` is 12 bytes. These are storage/work
bounds, not measured cycle budgets. Telemetry-disabled CLI installation copied
all 38 skills; changed skills passed quick validation.

## Physical observation

Mac host: Python/pySerial 3.5, FFmpeg 9.0.2; EverDrive Pro extended protocol
05/device 18, VID:PID 38DF:0018. Firmware was updated by the user earlier in this
session. No base-console model is inferred.

The final 262144-byte release was uploaded with the default hard reset:

- Build: `HL-61054C38-RA`.
- ROM SHA-256: `d9b873f8e7faa48b3ec69c4ca2a5ddc6685b9b0ddfcfb03c150aa19b8257ea7c`.
- Evidence: ignored `build/usb-input-final2-ready` and
  `build/usb-input-final2-capture/report.json`, with two HD60 X frames per case.

Direct HD60 X captures showed HARD, DATA 1234, BSS 0000, startup PASS, detected
EDIO, and advancing readiness counters/heartbeat. USB navigated to Input Lab
and locked D-pad navigation. Captures showed pads 0411/0122, mouse on each slot,
both mice with all four button bits exercised, and simultaneous pad/mouse.
Held bits and pointer positions matched commands; zero-delta renewals did not
repeat movement. Explicit releases and an unrenewed 30-VInt pulse returned to
the current physical sampler (port 2 had no device). A legacy readiness exchange
also completed while a UIV1 override remained active. Arrow tiles now queue
after their row clears, preserving visible D-pad arrows.

This tests logical USB input through a cooperating ROM. It does not qualify
physical mouse/multitap support, analog output, audio timing, saves, stressed USB
throughput, other firmware, electrical cold power cycling or arbitrary-ROM input.
ACK means sampled by the hook; capture inspection supplies the visual evidence.

## Panel and navigation follow-up

Input Lab now gives each port its own padded rounded panel, with a visible
central gutter. Text and both pointer rows stay inside the panel interiors;
clears do not cross the gutter or bottom corners. Page entry publishes dynamic
fields once instead of enqueueing the same fields twice. The footer explicitly
shows Left/Right navigation, A's lock/unlock action, and whether USB P1 has
paused the physical pad. Controls retain their existing meanings.

Release build `HL-B6D89515-RA`, ROM SHA-256
`9550d0d84f9f9a52623d7c21bc0573f17580b94ff7ba3b0989c6d3c45ac0da47`,
was uploaded with hard reset and inspected through direct HD60 X frames.
Ignored `build/input-ui-acceptance` contains readiness, all five diagnostic
pages, both pad masks, mouse pointers at opposite limits in each panel, mixed
pad/mouse input, release and the final unlocked page. Borders and corners remain
visible, fields stay inset and footer text stays outside the panels. Active USB
showed PAD PAUSED; release restored PHYSICAL PAD READY. The final view is Input
Lab with navigation unlocked and zero active overrides. No SRAM action or audio
playback action was requested during this visual pass.

The final source also passed the existing release/debug, PAL/NTSC and original
68000 hardware contract checks. The same freshly pulled toolchain above was
used. This is visual/input evidence, not an audio or frame-timing qualification.
