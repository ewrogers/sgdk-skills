#!/usr/bin/env python3
"""Deterministic native panel pieces, resident fonts and harbor tiles."""
import argparse
from pathlib import Path
from glyphs import GLYPHS
from pixel_io import indexed_png, read_png

UI = ['000000','000024','242449','24496d','496d92','92b6db','ffdb92','b6926d',
      '49496d','6d6d92','9292b6','b6b6db','6ddbdb','ffdb6d','db6d92','ffffdb']
WORLD = ['000024','242449','244949','496d6d','6d4949','926d49','dbb66d','ffdb92',
         '49496d','6db6b6','b6dbdb','494949','926d6d','b6926d','dbdbb6','ffffdb']
PORTRAIT = ['000000','242449','49496d','926d49','db926d','ffdb92','ffffff','6d2449',
            'b6496d','db9292','246d6d','49b6b6','92dbdb','242424','494949','929292']


def assets():
    width = 96 * 8
    pixels = [1] * (width * 48)
    for face in range(2):
        for color, ink in enumerate([15, 13, 12]):
            bank = face * 3 + color
            for code in range(32, 128):
                rows = GLYPHS.get(chr(code).upper(), GLYPHS['?'])
                for y in range(7):
                    bits = int(rows[y * 2:y * 2 + 2], 16)
                    for x in range(5):
                        if bits & (16 >> x):
                            px = (code - 32) * 8 + x + 1 + (int(y < 3) if face else 0)
                            pixels[(bank * 8 + y) * width + px] = ink
                            if face: pixels[(bank * 8 + y) * width + px + 1] = ink
    result = {'fonts.png': indexed_png(width, 48, pixels, UI)}
    panels = [1] * (24 * 24)
    for piece in range(9):
        column, row = piece % 3, piece // 3
        for y in range(8):
            for x in range(8):
                value = 1
                if (column == 0 and x == 0) or (column == 2 and x == 7) or (row == 0 and y == 0) or (row == 2 and y == 7): value = 2
                elif (column == 0 and x == 1) or (column == 2 and x == 6) or (row == 0 and y == 1) or (row == 2 and y == 6): value = 4
                elif (column == 0 and x == 2) or (row == 0 and y == 2): value = 5
                elif (column == 2 and x == 5) or (row == 2 and y == 5): value = 3
                panels[(row * 8 + y) * 24 + column * 8 + x] = value
    result['panels.png'] = indexed_png(24, 24, panels, UI)
    icons = [1] * (32 * 8)
    for frame in range(2):
        for y in range(3):
            for x in range(5 - 2 * y): icons[(y + 2 + frame) * 32 + frame * 8 + 1 + x + y] = 15
    for y in range(2, 6):
        for x in range(2, 6): icons[y * 32 + 16 + x] = 13 if x in (2,5) or y in (2,5) else 1
    result['icons.png'] = indexed_png(32, 8, icons, UI)
    world = [0] * (128 * 8)
    for tile in range(16):
        for y in range(8):
            for x in range(8):
                value = 0
                if tile == 1: value = 6 if (x,y) == (2,3) else 9 if (x,y) == (6,6) else 0
                elif tile == 2: value = 1 if y >= 5 else 0
                elif tile in (3,8,10):
                    value = 2 if y in (0,7) or (x + (4 if y < 4 else 0)) % 8 == 0 else 3
                    if tile == 8 and 2 <= x <= 5 and 1 <= y <= 6: value = 7 if x != 4 and y != 4 else 5
                elif tile == 4: value = 4 if y in (0,7) else 5 if x % 7 else 4
                elif tile in (5,6): value = 3 if y == (2 if tile == 5 else 5) and x % 6 < 3 else 2
                elif tile == 7: value = 7 if 2 <= x <= 5 and 1 <= y <= 6 else 4 if x in (1,6) else 0
                elif tile == 9: value = 4 if y >= abs(x - 4) else 0
                elif tile == 11: value = 1
                elif tile == 12: value = 3
                world[y * 128 + tile * 8 + x] = value
    result['world.png'] = indexed_png(128, 8, world, WORLD)
    return result


def pack_portrait(paths):
    if len(paths) != 5: raise ValueError('Expected the original portrait plus four animation frames')
    frames = [read_png(path) for path in paths]
    if any(frame[:2] != (32,32) for frame in frames): raise ValueError('Portrait frames must be native 32x32; never resize')
    palette = [tuple(bytes.fromhex(color)) for color in PORTRAIT]
    pixels = []
    for y in range(32):
        for _, _, image in frames:
            for red, green, blue, alpha in image[y * 32:(y + 1) * 32]:
                if alpha < 128: pixels.append(0); continue
                rgb = (red,green,blue)
                pixels.append(min(range(1,16), key=lambda i: sum((rgb[c] - palette[i][c]) ** 2 for c in range(3))))
    return indexed_png(160, 32, pixels, PORTRAIT, transparent=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--portraits', type=Path, help='optional source courier-0..4.png and keeper-0..4.png')
    args = parser.parse_args()
    generated = assets()
    if args.portraits:
        for name in ['courier','keeper']:
            generated[name + '.png'] = pack_portrait([args.portraits / f'{name}-{i}.png' for i in range(5)])
    if not args.force and any((args.output / name).exists() for name in generated): parser.error('Output exists; choose a new folder or regenerate explicitly with --force')
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in generated.items(): (args.output / name).write_bytes(data)
    print(f'Wrote {len(generated)} native indexed assets')


if __name__ == '__main__': main()
