#!/usr/bin/env python3
"""Create Performance Lab's original two-frame indexed orb without overwriting art."""
import argparse
from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def assets():
    palette = [(0, 0, 0), (32, 32, 32), (100, 176, 240), (224, 240, 252)] + [(0, 0, 0)] * 12
    rows = bytearray()
    for y in range(8):
        rows.append(0)
        for x in range(16):
            local = x % 8
            inside = (local * 2 - 7) ** 2 + (y * 2 - 7) ** 2 <= 49
            rows.append((2 + (x // 8)) if inside else 0)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 16, 8, 8, 3, 0, 0, 0))
           + chunk(b"PLTE", bytes(c for rgb in palette for c in rgb))
           + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))
    return {"orb.png": png}


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
