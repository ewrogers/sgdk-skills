# Mega EverDrive PRO USB debugging and development

Investigated 2026-10-19 using first-party sources. USB debugging feature claims below are source inspection; actual video observations are labeled separately. This extends [USB deployment research](mega-everdrive-pro-usb.md). Python deployment remains owned by `sgdk-project`; any future game-side instrumentation should stay optional, with its own protocol and CPU/RAM/transfer budget.

## Available in official tools

- **ROM deployment and resets:** upload/install/start, hard reset through B2, soft reset through B27, and optional custom FPGA mapper deployment. Hard reset remains our default. [Official examples](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/usb-cmd/README.md)
- **SD access:** host-side file copying in both directions. This could transfer fixtures, generated assets or save files without moving the SD card. [Implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceCmd.cs)
- **Cartridge memory access:** `memrd`/`memwr` target the internal **FCI bus**, covering ROM PSRAM, cartridge SRAM/BRAM and FPGA registers. This is not a documented arbitrary Genesis CPU-address-space access API. The Pro mapping includes two 8 MiB program-memory regions. [Command help](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Help.cs), [Mega mapping](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceIO.cs)
- **Device information and diagnostics:** identity, firmware, RTC and voltages; memory tests write patterns into program/save memory. Do not use destructive `diag` as a routine game deployment check. [Diagnostics](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/Diagnostics.cs), [write implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/Diagnostics.cs)
- **Menu screenshot:** `screen` asks the menu for VRAM/palette data. It is not documented as arbitrary running-game video capture. [Help](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Help.cs), [menu request](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/MenuCmd.cs)

## Requires game instrumentation

Official EDIO demonstrates console-to-PC text with `ed_cmd_usb_wr`, PC-to-console FIFO messages, and a 16-bit mailbox for fast status exchange. Host examples use `usbrd --print`, `fifowr`, and mailbox memory access. These support building logs, assertions, readiness/heartbeat signals, test results and developer commands; those features are proposals for our client, not already implemented. [Guest sample](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/main.c), [guest library](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c), [host examples](https://github.com/krikzz/mega-ed-pub/tree/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio-cmd)

A cooperative game could copy selected main-RAM snapshots into cartridge memory or USB messages. Automatic snapshots of a hung game, complete video reconstruction, and generic live asset replacement are not established capabilities. Prefer a bounded ring buffer for telemetry/assertion messages, limited draining per frame, and explicit overflow/disconnect behavior; hardware tests are required. This is proposed instrumentation, not arbitrary CPU-memory access.

## Debugger boundary and priorities

The inspected official host interface exposes no CPU breakpoints, CPU-register inspection, single-step, GDB server or JTAG debugger. Do not promise emulator-level debugging. [Published command interface](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Help.cs)

SGDK's `KDebug_*` APIs are documented as **Gens KMod** support; they are not EverDrive USB logging/debugging APIs. [SGDK v2.11 header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/kdebug.h)

Custom mapper sources and upload examples exist, but FPGA development is a separate advanced project, not a ready-made CPU debugger. [FPGA examples](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/README.md)

Running-ROM replacement and recovery from a deliberate IRQ-disabled infinite loop were subsequently observed on the user's console, as recorded in the deployment research. Hardware Lab now provides local readiness checks; bounded USB logging, structured test messages and cooperative state inspection remain possible next steps. Reserve emulator debugging for instruction-level investigation.

Hardware Lab's existing boot/reset page now has native tile graphics, an animated
running meter, a top-right build ID, the actual region flag and boot/DATA/BSS
results. It retains the other optional diagnostic pages and ships source and
build instructions, with generated ROMs excluded from Git. These local checks
do not claim a USB round trip: a USB LINK VERIFIED indicator would require
actual host/guest exchange. The readiness screen was uploaded and inspected
through the capture card; see [physical validation](../validation-hardware-readiness.md).

## Separate HDMI capture path: Elgato HD60 X on macOS

Video capture uses the Elgato, independently of EverDrive USB. On this Mac, FFmpeg 9.0.2 AVFoundation captured a live TEST 02 frame from the named `Elgato HD60 X` video device with audio disabled, using explicit 1920×1080, 30 fps and `uyvy422`. The user confirmed Elgato Studio's indicator continued blinking during the direct frame grab. This verifies one-frame concurrent access on this setup, without window capture; sustained simultaneous recordings and audio were not tested.

Elgato's current support article describes its branded Multi App Support as Windows-only and explicitly lists HD60 X as unsupported. Therefore the successful observation is not a vendor-supported general concurrency guarantee. [Official Multi App article](https://help.elgato.com/hc/en-us/articles/360042392672-Elgato-Capture-Devices-and-Multi-App-Support)

Default negotiation selected 3840×2160 and returned the device's invalid-output-resolution warning; explicit 1080p fixed this. Elgato explains that requesting capture above the HDMI input resolution can require unsupported upscaling; choose the input resolution or lower. Its resolution matrix also distinguishes hardware possibilities from software support. The error article carries a legacy-software notice. [Output-resolution explanation](https://help.elgato.com/hc/en-us/articles/17017045597837-Incompatible-Output-Resolution), [HD60 X matrix](https://help.elgato.com/hc/en-us/articles/5293283235469-Elgato-Game-Capture-HD60-X-Supported-Resolutions)

Official Elgato help articles were retrieved through their first-party Help Center JSON API on 2026-10-19 because the HTML endpoints returned HTTP 403. Search snippets omitted the current unsupported-device list; the live article takes precedence.
