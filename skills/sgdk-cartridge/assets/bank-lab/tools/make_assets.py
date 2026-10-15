#!/usr/bin/env python3
"""Generate Bank Lab's original stage art, palettes, markers and FM phrases."""
import argparse
import binascii
from pathlib import Path
import struct
import zlib


COLORS = [
    [(0, 0, 0), (36, 0, 0), (73, 36, 0), (109, 36, 0), (182, 73, 0),
     (255, 146, 36), (255, 219, 109), (255, 255, 219), (36, 36, 36), (73, 73, 73)],
    [(0, 0, 0), (0, 0, 36), (0, 36, 73), (0, 36, 109), (0, 109, 146),
     (36, 182, 219), (109, 219, 255), (219, 255, 255), (36, 36, 36), (73, 73, 73)],
]


def png(width, height, pixels, colors):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    palette = bytes(c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color)
    rows = b''.join(b'\0' + bytes(pixels[y * width:(y + 1) * width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0))
            + chunk(b'PLTE', palette) + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def stage_art(stage):
    floor = [1 if x == 0 or y == 0 else 2 if (x == 7 or y == 7) else 8
             for y in range(8) for x in range(8)]
    pixels = [0] * (64 * 16)
    for frame in range(4):
        for y in range(16):
            for x in range(16):
                dx, dy = abs(2 * x - 15), abs(2 * y - 15)
                inside = dx + dy <= 18 if stage == 0 else dx <= 9 and dy <= 13 - dx // 2
                if not inside: continue
                color = 3 if dx + dy > 14 else 4 + ((x // 3 + frame) % 3)
                if x < 7 and y < 7 and (x + y) % 4 == frame: color = 7
                pixels[y * 64 + frame * 16 + x] = color
    colors = COLORS[stage]
    palette = bytearray()
    for color in colors + [(0, 0, 0)] * (16 - len(colors)):
        # ResComp's 3-bit VDP quantization, represented as big-endian CRAM words.
        r, g, b = (v >> 5 for v in color)
        palette += struct.pack('>H', (b << 9) | (g << 5) | (r << 1))
    return png(8, 8, floor, colors), png(64, 16, pixels, colors), bytes(palette)


def music(stage):
    frames, step = (240, 20) if stage == 0 else (360, 30)
    header = bytearray(256); header[:4] = b'Vgm '
    for offset, value in [(8, 0x160), (12, 3579545), (0x18, frames * 735),
                          (0x20, frames * 735), (0x24, 60), (0x2C, 7670454), (0x34, 204)]:
        struct.pack_into('<I', header, offset, value)
    struct.pack_into('<H', header, 0x28, 9); header[0x2A] = 16
    data = bytearray()
    def ym(reg, value): data.extend([0x52, reg, value])
    ym(0x22, 0); ym(0x27, 0); ym(0x2B, 0)
    for ch in range(6): ym(0x28, ch if ch < 3 else ch + 1)
    for value in [0x9F, 0xBF, 0xDF, 0xFF]: data.extend([0x50, value])
    for ch in range(2):
        for slot, multiple, level in [(0, 2, 42 + stage * 8), (4, 1, 127), (8, 1, 28), (12, 1, 127)]:
            for reg, value in [(0x30, multiple), (0x40, level), (0x50, 31), (0x60, 8),
                               (0x70, 3), (0x80, 0x48), (0x90, 0)]: ym(reg + slot + ch, value)
        ym(0xB0 + ch, 4); ym(0xB4 + ch, 0xC0)
    struct.pack_into('<I', header, 0x1C, 256 + len(data) - 0x1C)
    notes = [644, 810, 964, 1287, 964, 810, 722, 964, 1082, 964, 810, 722] if stage == 0 else [644, 766, 964, 1147, 964, 766, 574, 722, 860, 1082, 860, 722]
    for frame in range(frames):
        if frame % step == 0:
            for ch, value, block in [(0, notes[frame // step], 4), (1, 644 if frame < frames // 2 else 574, 2)]:
                ym(0x28, ch); ym(0xA4 + ch, (block << 3) | (value >> 8)); ym(0xA0 + ch, value & 255)
                ym(0x28, 0xF0 | ch)
        if frame % step == step - 3: ym(0x28, 0); ym(0x28, 1)
        data.append(0x62)
    data.append(0x66); struct.pack_into('<I', header, 4, len(header) + len(data) - 4)
    return bytes(header + data)


def assets():
    files, crc = {}, []
    for stage, name in enumerate(['a', 'b']):
        floor, relic, palette = stage_art(stage)
        marker = b'BANK LAB STAGE ' + bytes([ord('A') + stage])
        files.update({f'stage_{name}_floor.png': floor, f'stage_{name}_relic.png': relic,
                      f'stage_{name}_palette.bin': palette, f'stage_{name}_marker.bin': marker,
                      f'music_{name}.vgm': music(stage)})
        crc.append(binascii.crc_hqx(palette + marker, 0xFFFF))
    files['stage_checks.inc'] = ('#ifndef BANK_LAB_STAGE_CHECKS_H\n#define BANK_LAB_STAGE_CHECKS_H\n'
                               + f'#define STAGE_A_CRC 0x{crc[0]:04X}\n#define STAGE_B_CRC 0x{crc[1]:04X}\n'
                               + '#endif\n').encode()
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('output', type=Path)
    args = parser.parse_args(); files = assets()
    if any((args.output / name).exists() for name in files): parser.error('Choose a new output directory; assets already exist')
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in files.items(): (args.output / name).write_bytes(data)
    print(f'Created {len(files)} original stage files in {args.output}')


if __name__ == '__main__': main()
