#!/usr/bin/env python3
"""Create Raster Lab's original water surface and slow FM underwater phrase."""
import argparse
from pathlib import Path
import struct
import zlib


def indexed_png(width, height, pixels):
    palette = bytes([255, 0, 255, 0, 36, 109, 36, 109, 182, 146, 219, 255,
                     0, 36, 73, 0, 73, 146]) + bytes(30)
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + pixels[y * width:(y + 1) * width] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0))
            + chunk(b'PLTE', palette) + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def surface():
    """Four 32x16 frames; rows 3..15 stay opaque throughout the palette handoff."""
    width, height = 128, 16
    pixels = bytearray(width * height)
    crest = [2, 2, 1, 0, 1, 1, 2, 3, 3, 2, 2, 1, 2, 3, 3, 3,
             2, 1, 1, 0, 0, 1, 2, 2, 3, 3, 2, 1, 1, 2, 3, 3]
    for frame in range(4):
        for x in range(32):
            top = crest[(x + frame * 4) % len(crest)]
            for y in range(top, height):
                noise = ((x + frame * 3) * 73 + (y // 2) * 151) ^ ((x // 3 + y + frame) * 29)
                color = 4 if noise % 7 < 2 else 1
                if y == top: color = 3 if noise % 5 else 2
                elif y <= top + 2: color = 2 if noise % 3 else 3
                elif y < 10 and noise % 11 < 2: color = 2 if y < 6 else 5
                pixels[y * width + frame * 32 + x] = color
    return indexed_png(width, height, pixels)


def bubble():
    pixels = bytearray(32 * 8)
    for frame in range(4):
        for y in range(8):
            for x in range(8):
                distance = (2 * x - 7) ** 2 + (2 * y - 7) ** 2
                if 24 <= distance <= 52:
                    pixels[y * 32 + frame * 8 + x] = 3 if x + y < 7 + frame % 2 else 2
    return indexed_png(32, 8, pixels)


def splash():
    pixels = bytearray(128 * 16)
    def dot(frame, x, y, color):
        if 0 <= x < 32 and 0 <= y < 16: pixels[y * 128 + frame * 32 + x] = color
    for frame in range(4):
        for side in [-1, 1]:
            for step in range(11):
                x = 16 + side * (step + frame * 2)
                y = 15 - step * (12 - step) // 5 - frame * 2
                if frame < 3:
                    dot(frame, x, y, 3); dot(frame, x, y + 1, 2)
            for drop in range(3):
                x = 16 + side * (4 + frame * 3 + drop * 2)
                y = 10 - frame * 2 - drop * 3
                dot(frame, x, y, 3); dot(frame, x, y + 1, 2)
    return indexed_png(128, 16, pixels)


def music():
    """Twelve-second minor/add-nine loop, soft pads and spaced bell notes; no drums."""
    header = bytearray(256)
    header[:4] = b'Vgm '
    for offset, value in [(8, 0x160), (12, 3579545), (0x18, 529200), (0x20, 529200),
                          (0x24, 60), (0x2C, 7670454), (0x34, 256 - 0x34)]:
        struct.pack_into('<I', header, offset, value)
    struct.pack_into('<H', header, 0x28, 9)
    header[0x2A] = 16
    data = bytearray()
    def ym(register, value):
        data.extend([0x52, register, value])
    ym(0x22, 0); ym(0x27, 0); ym(0x2B, 0)
    for ch in range(6): ym(0x28, ch if ch < 3 else ch + 1)
    for attenuation in [0x9F, 0xBF, 0xDF, 0xFF]: data.extend([0x50, attenuation])
    for ch in range(3):
        bell = ch == 2
        for slot, multiple, level in [(0, 2 if bell else 1, 48 if bell else 60),
                                      (4, 1, 127), (8, 1, 34 if bell else 32), (12, 1, 127)]:
            for register, value in [(0x30, multiple), (0x40, level), (0x50, 31 if bell else 12),
                                    (0x60, 18 if bell else 5), (0x70, 6 if bell else 2),
                                    (0x80, 0x88 if bell else 0x28), (0x90, 0)]:
                ym(register + slot + ch, value)
        ym(0xB0 + ch, 4); ym(0xB4 + ch, 0xC0)
    struct.pack_into('<I', header, 0x1C, 256 + len(data) - 0x1C)
    roots = [644, 766, 860, 766, 644, 574]
    fifths = [964, 1147, 644, 1147, 964, 860]
    bells = [1287, 1445, 1531, 1720, 1531, 1445]
    def note(ch, fnum, block):
        ym(0x28, ch)
        ym(0xA4 + ch, (block << 3) | (fnum >> 8))
        ym(0xA0 + ch, fnum & 255)
        ym(0x28, 0xF0 | ch)
    for frame in range(720):
        phrase = frame // 120
        if frame % 120 == 0:
            note(0, roots[phrase], 2)
            note(1, fifths[phrase], 2)
        if frame % 120 == 30: note(2, bells[phrase], 4)
        if frame % 120 == 75: ym(0x28, 2)
        if frame % 120 == 112: ym(0x28, 0); ym(0x28, 1)
        data.append(0x62)
    data.append(0x66)
    struct.pack_into('<I', header, 4, len(header) + len(data) - 4)
    return bytes(header + data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    files = {'water_surface.png': surface(), 'water_bubble.png': bubble(),
             'water_splash.png': splash(), 'music.vgm': music()}
    if any((args.output / name).exists() for name in files):
        parser.error('output already contains lab assets; choose a new directory')
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items(): (args.output / name).write_bytes(data)
    print(f'Created {len(files)} original assets in {args.output}')


if __name__ == '__main__':
    main()
