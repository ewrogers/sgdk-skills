#!/usr/bin/env python3
"""Create original FM/PSG music with sampled drums and standalone PCM effects."""
import argparse
from pathlib import Path
import struct


def percussion(noise, length):
    """Unsigned 8-bit DAC data at 13,300 Hz; integer-only, deterministic."""
    data = bytearray()
    state = 0xACE1
    for i in range(length):
        if noise:
            state = (state >> 1) ^ (0xB400 if state & 1 else 0)
            value = ((state & 255) - 128) // 2
        else:
            period = 45 + i // 12
            phase = i % period
            value = 4 * min(phase, period - phase) * 88 // period - 88
        data.append(128 + value * (length - i) // length)
    return bytes(data)


def music():
    # VGM 1.60, source60Hz. One four-second loop, two FM voices, PSG and DAC.
    header = bytearray(256)
    header[:4] = b"Vgm "
    for offset, value in [(8, 0x160), (12, 3579545), (0x18, 176400),
                          (0x20, 176400), (0x24, 60),
                          (0x2C, 7670454), (0x34, 256 - 0x34)]:
        struct.pack_into("<I", header, offset, value)
    struct.pack_into("<H", header, 0x28, 9)
    header[0x2A] = 16
    data = bytearray()
    kick, snare = percussion(False, 1330), percussion(True, 1596)
    bank = kick + snare
    data += b"\x67\x66\x00" + struct.pack("<I", len(bank)) + bank
    # DAC stream0 -> YM2612 port0 register2A, bank0, one byte per sample.
    data += b"\x90\x00\x02\x00\x2A\x91\x00\x00\x01\x00"
    data += b"\x92\x00" + struct.pack("<I", 13300)

    def ym(register, value):
        data.extend([0x52, register, value])

    ym(0x22, 0)       # No LFO.
    ym(0x27, 0)       # Normal FM channel3 mode, timers disabled.
    ym(0x2B, 0x80)    # DAC enabled; converted percussion owns that output.
    for channel in range(6):
        ym(0x28, channel if channel < 3 else channel + 1)
    for channel in [0, 1]:
        # Algorithm4: two two-operator stacks. Silence the second stack;
        # the first uses an audible modulator/carrier pair, not just a sine.
        for slot, multiple, level in [(0, 2, 36), (4, 1, 127), (8, 1, 24), (12, 1, 127)]:
            for register, value in [(0x30, multiple), (0x40, level), (0x50, 31),
                                    (0x60, 8), (0x70, 3), (0x80, 0x48), (0x90, 0)]:
                ym(register + slot + channel, value)
        ym(0xB0 + channel, 0x14)
        ym(0xB4 + channel, 0xC0)
    for attenuation in [0x9F, 0xBF, 0xDF, 0xFF]:
        data += bytes([0x50, attenuation])
    # Loop after immutable bank/setup. Explicit note and envelope commands
    # restore a consistent musical state at each repeat.
    struct.pack_into("<I", header, 0x1C, 256 + len(data) - 0x1C)
    notes = [644, 810, 964, 1082, 964, 810, 722, 644]
    periods = [214, 170, 143, 127, 143, 170, 190, 214]
    for frame in range(240):
        if frame % 30 == 0:
            note = frame // 30
            for channel, fnum, block in [(0, notes[note], 4), (1, 644 if note < 4 else 1082, 2)]:
                ym(0x28, channel)
                ym(0xA4 + channel, (block << 3) | (fnum >> 8))
                ym(0xA0 + channel, fnum & 255)
                ym(0x28, 0xF0 | channel)
            period = periods[note]
            data += bytes([0x50, 0x80 | (period & 15), 0x50, period >> 4, 0x50, 0x9A])
        if frame % 30 == 24:
            ym(0x28, 0); ym(0x28, 1)
            data += b"\x50\x9F"
        if frame % 15 == 0:
            offset, length = (0, len(kick)) if frame % 30 == 0 else (len(kick), len(snare))
            data += b"\x94\x00\x93\x00" + struct.pack("<I", offset) + b"\x01" + struct.pack("<I", length)
        if frame % 15 == 7:
            data += b"\x50\xE7\x50\xF9"  # Quiet PSG noise offbeat.
        if frame % 15 == 9:
            data += b"\x50\xFF"
        data += b"\x62"
    data += b"\x94\x00"
    data += b"\x66"
    struct.pack_into("<I", header, 4, len(header) + len(data) - 4)
    return bytes(header + data)


def effect(period, length):
    # Integer triangle wave and decay: reproducible across Python/OS versions.
    samples = bytearray()
    for i in range(length):
        phase = i % period
        triangle = (4 * min(phase, period - phase) * 80 // period) - 80
        samples.append(128 + triangle * (length - i) // length)
    # RIFF chunks have even-byte padding, excluded from their declared size.
    # Older Python wave writers omit this for odd lengths; write our one fixed
    # PCM format explicitly so regeneration is identical on supported versions.
    fmt = struct.pack("<HHIIHH", 1, 1, 13300, 13300, 1, 8)
    body = (b"WAVEfmt " + struct.pack("<I", len(fmt)) + fmt
            + b"data" + struct.pack("<I", length) + samples + bytes(length % 2))
    return b"RIFF" + struct.pack("<I", len(body)) + body


def assets():
    return {"music.vgm": music(), "click.wav": effect(30, 3325),
            "alarm.wav": effect(60, 13300)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    files = assets()
    if any((args.output / name).exists() for name in files):
        parser.error("output already contains lab audio; choose a new directory")
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        (args.output / name).write_bytes(data)
    print(f"Created {len(files)} original audio files in {args.output}")


if __name__ == "__main__":
    main()
