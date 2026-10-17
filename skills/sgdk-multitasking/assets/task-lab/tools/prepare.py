#!/usr/bin/env python3
"""Prepare a new standalone Task Lab, with independently packed original atlases."""
import argparse
from pathlib import Path
import shutil
import struct
import sys
import zlib
ROOT=Path(__file__).resolve().parents[1]
COLORS=[(0,0,1),(1,1,3),(1,2,4),(2,3,4),(2,4,3),(4,5,4),(1,6,2),(5,7,1),
        (7,7,1),(7,4,0),(7,1,1),(7,6,6),(4,2,3),(3,1,4),(2,1,3),(1,1,2)]

def pixels(variant):
    rows=[]
    for y in range(128):
        row=[]
        for x in range(128):
            cloud=((x//4)*13+(y//3)*7+((x//12)^(y//7))*11)&31
            c=1+(y//18)%4
            if y<64 and cloud<3:c=5
            if (x-(96 if variant==0 else 30))**2+(y-26)**2<180:c=9
            ridge=61+((x//8*7+x//24*3+variant*11)%19)
            if y>=ridge:c=3+(x//16+variant)%3
            if y>86:c=2+(y//5)%3
            if 12<x<116 and 88<=y<105:
                c=10 if y%8 else 9
                if (x-14)%24<15 and y>91:c=1
            if 26<x<36 and 67<=y<89:c=11
            if 23<x<39 and 64<=y<68:c=9
            if y>108 and (x+y//2)%23<7 and y%4==0:c=6
            row.append(c)
        rows.append(row)
    return rows

def tiles(rows):
    out=bytearray()
    for ty in range(0,128,8):
        for tx in range(0,128,8):
            for y in range(ty,ty+8):
                for x in range(tx,tx+8,2):out.append(rows[y][x]*16+rows[y][x+1])
    return bytes(out)

def png(rows):
    def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',128,128,8,3,0,0,0))+chunk(b'PLTE',bytes(c*255//7 for rgb in COLORS for c in rgb))+chunk(b'IDAT',zlib.compress(b''.join(b'\0'+bytes(row) for row in rows)))+chunk(b'IEND',b'')

def header(region='JUE'):
    if region not in ('U','E','JUE'):raise ValueError('Use U, E or JUE')
    def field(value,size):return '{'+','.join(str(c) for c in value.ljust(size).encode('ascii'))+'}'
    values=[field('SEGA MEGA DRIVE',16),field('(C)2026',16),field('SPARE TIME LAB',48),field('SPARE TIME LAB',48),
            field('GM 00000000-00',14),'0',field('J',16),'0','0x1FFFF','0xE0FF0000','0xE0FFFFFF',
            field('',2),'0','0','0',field('',12),field('USER TASK EXAMPLES',40),field(region,16)]
    return '#include <genesis.h>\n__attribute__((externally_visible)) const ROMHeader rom_header={\n'+',\n'.join(values)+'};\n'

def resources(output):
    res=output/'res';res.mkdir()
    includes=[]
    for v in range(2):
        (res/f'atlas{v}.bin').write_bytes(tiles(pixels(v)));(res/f'atlas{v}.png').write_bytes(png(pixels(v)))
        for codec,name in [('NONE',f'raw{v}'),('APLIB',f'atlas{v}')]:
            (res/f'{name}.res').write_text(f'BIN {name} "atlas{v}.bin" 2 2 0 {codec} FALSE\n')
            includes.append(f'#include "{name}.h"')
    palette=','.join(hex((b<<9)|(g<<5)|(r<<1)) for r,g,b in COLORS)
    (output/'src/catalog.c').write_text('''#include "platform.h"
'''+ '\n'.join(includes)+'''
const u16 mt_palette[16]={'''+palette+'''};
int mt_decode(u16 variant,u8 *out,u16 capacity,void *ctx) {
    (void)ctx;
    if(variant>1 || capacity<MT_BYTES || !out)return 0;
    const u8 *packed=variant?atlas1:atlas0;
    const u8 *expected=variant?raw1:raw0;
    if(unpack(COMPRESSION_APLIB,(u8 *)packed,out)!=MT_BYTES)return 0;
    for(u16 i=0;i<MT_BYTES;i++)if(out[i]!=expected[i])return 0;
    return 1;
}
''')

def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    output.mkdir(parents=True)
    for name in ['src','inc','tools']:shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ['Makefile','README.md']:shutil.copyfile(ROOT/name,output/name)
    (output/'src/boot').mkdir(exist_ok=True);(output/'src/boot/rom_head.c').write_text(header())
    resources(output);return output

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
    try:print(prepare(parser.parse_args().output))
    except (ValueError,OSError) as error:print(error,file=sys.stderr);raise SystemExit(2)
