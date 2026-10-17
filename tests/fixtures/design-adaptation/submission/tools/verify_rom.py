#!/usr/bin/env python3
"""Validate this standard SGDK linear build, without modifying it."""
import hashlib
import json
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1] if len(sys.argv) > 1 else 'out/rom.bin')
data = bytearray(path.read_bytes())
assert len(data) == 131072, 'expected one 128 KiB SGDK padding block'
u32 = lambda offset: struct.unpack_from('>I', data, offset)[0]
assert data[0x100:0x110] == b'SEGA MEGA DRIVE '
assert data[0x120:0x150].decode().strip() == 'LAST SERVICE'
assert data[0x150:0x180].decode().strip() == 'LAST SERVICE'
assert data[0x190:0x1a0].decode().strip() == 'J'
assert data[0x1f0:0x200].decode().strip() == 'JUE'
assert u32(0x1a0) == 0 and u32(0x1a4) == len(data)-1
assert data[0x1b0:0x1bc] == b'  ' + bytes(10), 'no save RAM'
assert 0x200 <= u32(4) < len(data) and u32(4) % 2 == 0
assert u32(0) == 0xE1000000, 'SDK stack vector wraps on 24-bit bus'
stored = struct.unpack_from('>H', data, 0x18e)[0]
data[0x18e:0x190] = bytes(2)
checksum = 0
for (word,) in struct.iter_unpack('>I', data):
    checksum ^= word
checksum = ((checksum >> 16) ^ checksum) & 0xffff
assert checksum == stored, 'SGDK sizebnd XOR-folded checksum mismatch'
print(json.dumps({'valid': True, 'path': str(path), 'bytes': len(data),
                  'checksum': f'{stored:04x}',
                  'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}, indent=2))
