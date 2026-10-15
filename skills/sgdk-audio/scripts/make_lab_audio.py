#!/usr/bin/env python3
"""Create original PSG music and PCM effects for Sound Lab, without replacing files."""
import argparse
import io
from pathlib import Path
import struct
import wave


def music():
    # VGM 1.60: 60 Hz commands, one PSG tone, two seconds with a loop point.
    header = bytearray(256)
    header[:4] = b"Vgm "
    for offset, value in [(8, 0x160), (12, 3579545), (0x18, 88200),
                          (0x1C, 256 - 0x1C), (0x20, 88200), (0x24, 60),
                          (0x2C, 7670454), (0x34, 256 - 0x34)]:
        struct.pack_into("<I", header, offset, value)
    struct.pack_into("<H", header, 0x28, 9)
    header[0x2A] = 16
    data = bytearray()
    for attenuation in [0x9F, 0xBF, 0xDF, 0xFF]:
        data += bytes([0x50, attenuation])
    # PSG divider values for a short original arpeggio, not a borrowed tune.
    for period in [428, 339, 285, 214, 285, 339, 381, 428]:
        data += bytes([0x50, 0x80 | (period & 15), 0x50, period >> 4, 0x50, 0x94])
        data += bytes([0x62]) * 12
        data += bytes([0x50, 0x9F]) + bytes([0x62]) * 3
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
    out = io.BytesIO()
    with wave.open(out, "wb") as wav:
        wav.setparams((1, 1, 13300, length, "NONE", "not compressed"))
        wav.writeframes(samples)
    return out.getvalue()


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
