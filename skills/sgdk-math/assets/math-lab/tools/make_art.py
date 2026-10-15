#!/usr/bin/env python3
"""Generate the lab's original indexed pixel sprites without imaging packages."""
import argparse
from pathlib import Path
import struct
import zlib

PALETTE = [(0, 0, 0), (36, 36, 73), (73, 73, 109), (109, 109, 146),
           (146, 146, 182), (182, 182, 219), (219, 219, 255), (255, 255, 255),
           (36, 73, 146), (73, 146, 219), (146, 219, 255), (219, 255, 255),
           (146, 36, 109), (219, 73, 182), (255, 146, 219), (255, 219, 255)]


def png(width, height, pixels):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + bytes(pixels[y * width:(y + 1) * width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0))
            + chunk(b'PLTE', bytes(c for rgb in PALETTE for c in rgb))
            + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def assets():
    stars = [0] * (24 * 8)
    for frame in range(3):
        for y in range(8):
            for x in range(8):
                distance = abs(x - 3) + abs(y - 3)
                if distance <= frame:
                    stars[y * 24 + frame * 8 + x] = 3 + frame * 2 if distance == 0 else 2 + frame
    orbs = [0] * (64 * 16)
    for frame in range(4):
        for y in range(16):
            for x in range(16):
                distance = (2 * x - 15) ** 2 + (2 * y - 15) ** 2
                if distance <= 164:
                    color = 8 if distance > 130 else 9 if x + y > 17 else 10
                    if frame & 1: color = {8: 12, 9: 13, 10: 14}[color]
                    if (x - 5) ** 2 + (y - 4) ** 2 < 7: color = 11 if not frame & 1 else 15
                    if frame >= 2 and 6 <= x <= 10 and y in (6, 7): color = 7
                    orbs[y * 64 + frame * 16 + x] = color
    return {'star.png': png(24, 8, stars), 'orb.png': png(64, 16, orbs)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    files = assets()
    if any((args.output / name).exists() for name in files):
        parser.error('Choose a new output folder; source art already exists')
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        (args.output / name).write_bytes(data)
    print('Created two original indexed sprite sheets')


if __name__ == '__main__':
    main()
