#!/usr/bin/env python3
"""Create a standalone Compression Lab without replacing existing files."""
import argparse
from pathlib import Path
import shutil
import struct
import sys
import zlib
ROOT = Path(__file__).resolve().parents[1]
COLORS = [(1,1,2),(2,2,4),(2,3,5),(3,4,6),(4,5,6),(6,6,7),(7,7,7),(1,2,3),
          (1,3,4),(2,5,5),(3,6,5),(6,4,2),(7,5,2),(7,6,4),(5,3,3),(3,2,4)]

def pixels(sample):
    rows=[];seed=0x3157
    for y in range(64):
        row=[]
        for x in range(128):
            if sample:
                seed ^= (seed << 7) & 65535;seed ^= seed >> 9;seed ^= (seed << 8) & 65535
                c=seed & 15
            else:
                c=1+(y//8)%4
                if (x-96)**2+(y-15)**2 < 81:c=13
                if y>=44:c=8+(y//4)%3
                if 9<=x<119 and 30<=y<47:c=11 if y%8 else 12
                if 17<=x<111 and 32<=y<47 and (x-17)%24<16:c=0
                if 11<=x<117 and 28<=y<31:c=13
                if 57<=x<71 and 16<=y<31:c=11 if x%8 else 12
                if 55<=x<73 and 14<=y<17:c=13
                if y>=48 and y%4==0 and (x+y)%24<10:c=4
                if x%16==0 and y<28:c=2
            row.append(c)
        rows.append(row)
    return rows

def tiles(rows):
    out=bytearray()
    for ty in range(0,64,8):
        for tx in range(0,128,8):
            for y in range(ty,ty+8):
                for x in range(tx,tx+8,2):out.append(rows[y][x]*16+rows[y][x+1])
    return bytes(out)

def png(rows):
    def block(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data)&0xffffffff)
    palette=bytes(v*255//7 for rgb in COLORS for v in rgb)
    return b'\x89PNG\r\n\x1a\n'+block(b'IHDR',struct.pack('>IIBBBBB',128,64,8,3,0,0,0))+block(b'PLTE',palette)+block(b'IDAT',zlib.compress(b''.join(b'\0'+bytes(row) for row in rows)))+block(b'IEND',b'')

def resources(output):
    res=output/'res';res.mkdir()
    includes=[];entries=[];chunks=[]
    for sample in range(2):
        data=tiles(pixels(sample));(res/f'picture{sample}.bin').write_bytes(data)
        (res/f'picture{sample}.png').write_bytes(png(pixels(sample)))
        for codec in range(3):
            name=f'picture{sample}_{codec}'
            # One BIN per resource file: LZ4W cannot refer to another block's dictionary.
            (res/f'{name}.res').write_text(f'BIN {name} "picture{sample}.bin" 2 2 0 {codec} FALSE\n')
            includes.append(f'#include "{name}.h"')
            entries.append(f'{{{name}, sizeof({name}), {codec}}}')
        for part in range(8):
            name=f'chunk{sample}_{part}'
            (res/f'{name}.bin').write_bytes(data[part*512:(part+1)*512])
            (res/f'{name}.res').write_text(f'BIN {name} "{name}.bin" 2 2 0 LZ4W FALSE\n')
            includes.append(f'#include "{name}.h"');chunks.append(f'{{{name}, sizeof({name}), 2}}')
    colors=','.join(hex((b<<9)|(g<<5)|(r<<1)) for r,g,b in COLORS)
    (output/'src/catalog.c').write_text('#include <genesis.h>\n#include "catalog.h"\n'+'\n'.join(includes)+'\nconst CLAsset cl_assets[2][3] = {\n{'+','.join(entries[:3])+'},\n{'+','.join(entries[3:])+'}};\nconst CLAsset cl_chunks[2][8] = {\n{'+','.join(chunks[:8])+'},\n{'+','.join(chunks[8:])+'}};\nconst u16 cl_palette[16] = {'+colors+'};\n')

def header(region='JUE'):
    if region not in ('JUE','U','E'):raise ValueError('Use JUE, U or E')
    def text(value,width):
        return '{'+','.join(str(b) for b in value.ljust(width).encode('ascii'))+'}'
    fields=[text('SEGA MEGA DRIVE',16),text('(C)2026',16),text('COMPRESSION LAB',48),
            text('COMPRESSION LAB',48),text('GM 00000000-00',14),'0',text('J',16),
            '0','0x1FFFF','0xE0FF0000','0xE0FFFFFF',text('',2),'0','0','0',
            text('',12),text('CODEC TRADE-OFFS',40),text(region,16)]
    return '#include <genesis.h>\n__attribute__((externally_visible)) const ROMHeader rom_header={\n'+',\n'.join(fields)+'};\n'

def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    output.mkdir(parents=True)
    for name in ['inc','src','tools']:shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ['Makefile','README.md']:shutil.copyfile(ROOT/name,output/name)
    (output/'src/boot').mkdir(exist_ok=True)
    (output/'src/boot/rom_head.c').write_text(header())
    resources(output)
    return output

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
    try:print(prepare(parser.parse_args().output))
    except (ValueError,OSError) as error:print(error,file=sys.stderr);raise SystemExit(2)
