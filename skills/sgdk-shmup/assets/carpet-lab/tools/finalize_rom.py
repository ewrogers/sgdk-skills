#!/usr/bin/env python3
"""Set the cartridge ROM extent after SGDK has padded the image."""
from pathlib import Path
import struct
import sys


def finalize(path):
    data = bytearray(Path(path).read_bytes())
    if len(data) < 512 or len(data) % 131072 or data[0x100:0x104] != b'SEGA':
        raise ValueError('Expected a padded Mega Drive ROM')
    struct.pack_into('>I', data, 0x1A4, len(data) - 1)
    Path(path).write_bytes(data)


if __name__ == '__main__':
    finalize(sys.argv[1])
