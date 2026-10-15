#!/usr/bin/env python3
"""Create the lab's original indexed shapes in a new directory."""
import argparse
from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def image(size, filled, color):
    palette = [(0, 0, 0), (36, 36, 72), (36, 36, 36),
               (72, 180, 252), (252, 180, 36), (252, 252, 252)] + [(0, 0, 0)] * 10
    rows = b"".join(b"\0" + bytes(color if x < filled and y < filled else 0
                                 for x in range(size)) for y in range(size))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 3, 0, 0, 0))
            + chunk(b"PLTE", bytes(c for rgb in palette for c in rgb))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def assets():
    return {"player.png": image(16, 12, 3), "target.png": image(16, 12, 4),
            "shot.png": image(8, 4, 5)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    files = assets()
    if any((args.output / name).exists() for name in files):
        parser.error("lab art already exists; choose a new directory")
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        (args.output / name).write_bytes(data)


if __name__ == "__main__":
    main()
