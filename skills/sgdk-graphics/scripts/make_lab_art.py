#!/usr/bin/env python3
"""Reproduce the lab's original indexed test art into a new directory."""
import argparse
from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def png(width, height, pixel, colors):
    palette = colors + [(0, 0, 0)] * (16 - len(colors))
    rows = b"".join(b"\0" + bytes(pixel(x, y) for x in range(width)) for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
            + chunk(b"PLTE", bytes(c for rgb in palette for c in rgb))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def hero_pixel(x, y):
    frame, animation = x // 16, y // 16
    x, y = x % 16, y % 16
    radius = (5 + frame) if animation else (5 + (frame % 2))
    distance = abs(x - 7) + abs(y - 7)
    if distance > radius:
        return 0
    if distance == radius:
        return 1
    return 3 if x == 7 or y == 7 else 2


def images():
    return {
        "far.png": png(512, 256, lambda x, y: 2 if x % 32 == 0 or y % 32 == 0 else 1,
                       [(0, 0, 0), (36, 36, 36), (36, 36, 72)]),
        "near.png": png(512, 256,
                        lambda x, y: 1 + (x // 8) % 3 if y % 64 in (40, 41) and x % 64 < 32 else 0,
                        [(0, 0, 0), (0, 108, 252), (0, 216, 252), (180, 216, 252)]),
        "hero.png": png(64, 32, hero_pixel,
                        [(0, 0, 0), (72, 144, 252), (36, 72, 144), (252, 252, 252)]),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    art = images()
    if any((args.output / name).exists() for name in art):
        parser.error("output contains lab art already; choose a new directory")
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in art.items():
        (args.output / name).write_bytes(data)
    print(f"Created {len(art)} original indexed images in {args.output}")


if __name__ == "__main__":
    main()
