# Seeing a real console through a capture card

Use HDMI capture as an observation channel alongside ROM deployment and, when
implemented in the game, USB telemetry. An agent can inspect fresh PNGs or short
sampled sequences. This does not provide continuous perception, controller input,
CPU breakpoints, or frame-accurate timing measurement.

## Responsibility and prerequisites

This skill owns visual evidence collection and comparison. sgdk-project owns
EverDrive build/upload/reset; sgdk-hardware owns interpreting console behavior;
sgdk-debug owns investigating faults and any optional game instrumentation.
No sibling skill is needed to use this helper. No deployment policy or second
reset tool belongs in the capture helper.

The bundled [capture_console.py](../scripts/capture_console.py) uses standard
Python 3.10+ and a host FFmpeg with AVFoundation. Its implemented backend is
**macOS**. Linux V4L2 and Windows DirectShow need separate adapters; no fallback
opens an arbitrary webcam. It installs nothing and does not change desktop app,
emulator, cartridge or persistent capture-card settings. FFmpeg negotiates the
requested format for its capture session, which can still affect another app.

Identify the capture card first. On macOS, FFmpeg lists video/audio names with:

```sh
ffmpeg -hide_banner -f avfoundation -list_devices true -i ""
```

FFmpeg normally exits nonzero after that inventory because no input is selected;
that is distinct from a failed frame capture. Use only the intended capture card
name/index. Do not substitute another camera or desktop input if it is absent.
macOS camera permission must be granted normally if required; do not bypass it.

## Capture fresh evidence

```sh
python3 /path/to/sgdk-testing/scripts/capture_console.py --device "Elgato HD60 X" --output build/capture-001
python3 /path/to/sgdk-testing/scripts/capture_console.py --device "Elgato HD60 X" --output build/capture-002 --frames 4 --sample-fps 4 --after 1
```

Each output folder must be new. The helper refuses overwrites, caps sequences at
16 images, sets a process timeout, and captures video only (`device:none`, `-an`).
Defaults are explicit 1920x1080, 30 requested input fps and `uyvy422`; `--size`
and `--input-fps` can select a supported lower mode. `--after` skips input time
before saving; it does not verify the console finished booting. This is a
bounded command, not a persistent recorder or background automation.

Review `frame-00.png` and, for motion, multiple sequence frames using the host's
image-viewing tool. `capture.json` records UTC capture interval, command, FFmpeg
version, requested mode, actual PNG dimensions and hashes. `capture.log` preserves
driver/format errors. Capture success only proves images were produced; the
manifest deliberately leaves `runtime_verified` false until separate review.
An Elgato error/no-signal screen is still a valid image, not a successful game.

Tie the observation to the ROM SHA-256 and a visible build identifier. Record
what the reviewer actually saw (for example, TEST04 + HARD + RAM PASS, with both
heartbeat states), which files support it, and what remains unknown. Input fps
and sampled-frame timestamps are not the Genesis refresh cadence or measured
input latency. Scaling, deinterlacing, converters, compression and dropped frames
can alter the picture. A static scene or identical hashes alone cannot prove a
hang; use an explicit heartbeat/state signal and inspect several fresh frames.

## Elgato Studio concurrency and the tested HD60 X

On 2026-10-19, FFmpeg 9.0.2 captured the named HD60 X on this Mac while the user
watched Elgato Studio and confirmed its preview stayed live. No Studio window
capture was used. Short direct frame/burst capture worked; simultaneous long
recordings and audio were not tested. Treat concurrent access as observed on
that setup, not guaranteed vendor support: Elgato explicitly lists HD60 X as
unsupported by its branded [Multi App feature](https://help.elgato.com/hc/en-us/articles/360042392672-Elgato-Capture-Devices-and-Multi-App-Support).

Initial automatic negotiation chose 3840x2160 and produced the capture card's
invalid-resolution screen. Explicit 1920x1080 fixed it. Avoid asking the device
to upscale above its HDMI input, and prefer the same supported mode as the user's
preview app. If either preview stalls, stop the extra capture session and
coordinate ownership; do not automatically close or reconfigure the user's app.
[Elgato resolution explanation](https://help.elgato.com/hc/en-us/articles/17017045597837-Incompatible-Output-Resolution),
[HD60 X supported formats](https://help.elgato.com/hc/en-us/articles/5293283235469-Elgato-Game-Capture-HD60-X-Supported-Resolutions),
[FFmpeg AVFoundation documentation](https://ffmpeg.org/ffmpeg-devices.html#avfoundation).

## USB debugging alongside video

EverDrive USB is a separate channel from HDMI capture. Official EDIO examples
demonstrate console-to-PC text, PC-to-console messages and a 16-bit mailbox.
These can support bounded logs, assertions, readiness signals and test results
**with game instrumentation**. Our current uploader does not implement those
features. Use a bounded ring buffer, a per-frame drain budget, and explicit
disconnect/overflow behavior rather than blocking the game's frame loop.
[Official EDIO source](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/main.c),
[guest library](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c).

Host memory commands access cartridge ROM PSRAM/SRAM/BRAM/registers, not arbitrary
Genesis main RAM. A game must export selected RAM state for cooperative snapshots.
Official `screen` is a menu VRAM/palette request, not running-game capture.
The published interface has no CPU breakpoint/register/single-step/GDB debugger.
SGDK `KDebug_*` targets Gens KMod, not EverDrive USB. Do not run cartridge memory
diagnostics as a harmless readiness test: they write program/save-memory patterns.
[Official commands](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Help.cs),
[Mega memory mapping](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceIO.cs),
[SGDK KDebug header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/kdebug.h).
