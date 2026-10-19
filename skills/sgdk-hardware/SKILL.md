---
name: sgdk-hardware
description: Validate SGDK games beyond an emulator with reset, PAL/NTSC video, sprite limits, DMA/audio contention, controller and cartridge-save checks. Use for console compatibility, flashcart testing, emulator-versus-hardware differences and reproducible hardware bug reports.
---

# SGDK hardware

A ROM running in an emulator is a useful milestone, not console certification.
Start with the reported symptom and an exact build. Separate a software defect,
an emulator setting, a cartridge limitation and a physical console behavior
before proposing a low-level workaround.

## Establish the test environment

Record the ROM SHA-256, release/debug mode, container/SDK/compiler versions,
console model/revision if known, region, display path, peripherals and cartridge
or flashcart model/firmware. For an emulator, record its version and actual
emulated system/region. Unknown fields stay unknown.

For Mega EverDrive Pro USB upload, use sgdk-project's bundled host client when
that skill is installed. Default deployments to hard reset; soft requires an
explicit user request. Keep upload/install success, launch sent, fresh SGDK
startup and actual electrical power cycling as separate observations. This
skill owns those console checks rather than a second upload implementation.

Pull `ewrogers/sgdk:latest`; inspect the installed headers and implementations
before relying on reset, VBlank, audio or mapper behavior. Read
[SGDK contracts and hardware differences](references/contracts.md). Do not
replace SGDK's startup or bus handling just because an emulator accepts an
alternative. Never enable newer 68k instructions, strict-aliasing assumptions or
unsafe interrupt changes to make a test pass.

## Make the smallest useful diagnostic

- Test cold boot and reset separately. Explicitly initialize application state
  on both entry paths. A software forced initialization is not a power cycle.
- Check actual PAL and NTSC execution. A header edit alone does not change the
  video hardware. Keep refresh cadence and gameplay timing distinct.
- Test colors, edge visibility and scanline pressure independently of total
  sprite count. Expected dropout in an intentional overload is not a ROM fault.
- Compare music alone with/without the same bounded DMA workload, then PCM with
  that workload. Do not change several audio settings at once.
- Check both real controller ports and the intended peripheral models. Let the
  chosen SGDK driver own polling and Z80 coordination.
- Make save writes explicit and restricted to the test ROM's own save area.
  Keep save format tests separate from battery retention and flashcart flushing.

Use [Hardware Lab](assets/hardware-lab/README.md) as an observation aid. It uses
normal SGDK APIs and a deliberately small ROM; it is not a destructive RAM
tester or an electrical test suite. Its artwork and reference audio are diagnostic
fixtures, not a prescribed game style.
Its readiness page has a visible build label, boot/DATA/BSS results and a VInt
heartbeat. Record the label and ROM hash, then inspect fresh capture frames when
available. Its optional [EverDrive link check](references/everdrive-link.md)
detects EDIO and verifies a fresh host/ROM exchange; the live lease expires without
renewal while PASS/IDLE preserves the last successful result. Leave console identity unknown. Startup/link checks do not establish an
electrical power cycle. Other Lab pages remain opt-in, including guarded SRAM writes.

## Keep the evidence honest

Follow the [console checklist](references/checklist.md). Mark each observation
as **observed**, **failed** or **not tested**, with its environment. Leave all
physical-console checks unverified when only emulators are available.

Host tests can prove bounded arithmetic and record validation. Compiled target
probes can check base-68000 execution. Neither emulates the VDP, Z80 bus timing,
controllers, SRAM wiring, analog audio or a real display. VInt counters expose
missed loop intervals; they are not an independently calibrated refresh meter.

Refer to sgdk-testing for deterministic replay, sgdk-performance for profiling,
sgdk-input for peripheral setup, sgdk-save for resilient save formats,
sgdk-cartridge for mapper/banking contracts and sgdk-release for packaging.
Keep those implementations in their focused skills instead of duplicating them.
