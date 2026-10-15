#!/usr/bin/env python3
"""Write Menu Lab's original 5x7 font as a 128x48 indexed PNG."""
import argparse
from pathlib import Path
import struct
import zlib


# Seven rows of five bits. Lowercase intentionally shares the uppercase design.
GLYPHS = {
    " ": [0]*7,
    "A": [14,17,17,31,17,17,17], "B": [30,17,17,30,17,17,30],
    "C": [14,17,16,16,16,17,14], "D": [30,17,17,17,17,17,30],
    "E": [31,16,16,30,16,16,31], "F": [31,16,16,30,16,16,16],
    "G": [14,17,16,23,17,17,15], "H": [17,17,17,31,17,17,17],
    "I": [14,4,4,4,4,4,14], "J": [7,2,2,2,18,18,12],
    "K": [17,18,20,24,20,18,17], "L": [16,16,16,16,16,16,31],
    "M": [17,27,21,21,17,17,17], "N": [17,25,21,19,17,17,17],
    "O": [14,17,17,17,17,17,14], "P": [30,17,17,30,16,16,16],
    "Q": [14,17,17,17,21,18,13], "R": [30,17,17,30,20,18,17],
    "S": [15,16,16,14,1,1,30], "T": [31,4,4,4,4,4,4],
    "U": [17,17,17,17,17,17,14], "V": [17,17,17,17,17,10,4],
    "W": [17,17,17,21,21,21,10], "X": [17,17,10,4,10,17,17],
    "Y": [17,17,10,4,4,4,4], "Z": [31,1,2,4,8,16,31],
    "0": [14,17,19,21,25,17,14], "1": [4,12,4,4,4,4,14],
    "2": [14,17,1,2,4,8,31], "3": [30,1,1,14,1,1,30],
    "4": [2,6,10,18,31,2,2], "5": [31,16,16,30,1,1,30],
    "6": [14,16,16,30,17,17,14], "7": [31,1,2,4,8,8,8],
    "8": [14,17,17,14,17,17,14], "9": [14,17,17,15,1,1,14],
    ".": [0,0,0,0,0,12,12], ",": [0,0,0,0,4,4,8],
    ":": [0,12,12,0,12,12,0], "-": [0,0,0,31,0,0,0],
    "/": [1,2,2,4,8,8,16], "+": [0,4,4,31,4,4,0],
    "<": [2,4,8,16,8,4,2], ">": [8,4,2,1,2,4,8],
    "?": [14,17,1,2,4,0,4], "!": [4,4,4,4,4,0,4],
    "#": [10,10,31,10,31,10,10], "=": [0,0,31,0,31,0,0],
    "(": [2,4,8,8,8,4,2], ")": [8,4,2,2,2,4,8],
    "_": [0,0,0,0,0,0,31], "'": [4,4,8,0,0,0,0],
}


def pixels():
    image = bytearray(128 * 48)
    for slot in range(96):
        rows = GLYPHS.get(chr(slot + 32).upper(), GLYPHS["?"])
        for y, row in enumerate(rows):
            for x in range(5):
                if row & (16 >> x):
                    image[(slot // 16 * 8 + y) * 128 + slot % 16 * 8 + x + 1] = 15
    return bytes(image)


def font_png():
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    data = pixels()
    rows = b"".join(b"\0" + data[y * 128:(y + 1) * 128] for y in range(48))
    palette = b"\0\0\0" * 15 + b"\xff\xff\xff"
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 128, 48, 8, 3, 0, 0, 0))
            + chunk(b"PLTE", palette) + chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new PNG path; existing files are never overwritten")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    try:
        with args.output.open("xb") as stream:
            stream.write(font_png())
    except FileExistsError:
        parser.error("output already exists; choose a new path")
    print(args.output)


if __name__ == "__main__":
    main()
