#!/usr/bin/env python3
"""Copy the native color showcase into a new folder and build through Docker."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parent
THEMES=['fantasy','sci-fi','modern','colonial']


def make_project(destination):
    if destination.exists():raise ValueError('Destination exists; choose a new directory')
    shutil.copytree(ROOT/'rom',destination)
    (destination/'res').mkdir(exist_ok=True)
    lines=[]
    for theme in THEMES:
        identifier=theme.replace('-','')
        for kind,source,definition in [('bg','background-two-lines','IMAGE'),('actor','actor','SPRITE'),('item','object','SPRITE'),('ui','ui','SPRITE')]:
            filename=f'{theme}-{source}.png';shutil.copyfile(ROOT/'final'/filename,destination/'res'/filename)
            settings='NONE'if definition=='IMAGE'else ('2 2 NONE 0 NONE'if kind=='item'else'8 4 NONE 0 NONE'if kind=='ui'else'4 4 NONE 0 NONE')
            lines.append(f'{definition} {identifier}_{kind} "{filename}" {settings}')
    (destination/'res/resources.res').write_text('\n'.join(lines)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--platform');parser.add_argument('--mode',choices=['release','debug'],default='release');args=parser.parse_args()
    make_project(args.output)
    command=['docker','run','--rm']
    if args.platform:command+=['--platform',args.platform]
    if hasattr(os,'getuid'):command+=['--user',f'{os.getuid()}:{os.getgid()}']
    command+=['--mount',f'type=bind,source={args.output.resolve()},target=/workspace','ewrogers/sgdk:latest','make',args.mode]
    result=subprocess.run(command)
    if result.returncode:return result.returncode
    rom=args.output/'out/rom.bin';data=bytearray(rom.read_bytes())
    if data[0x100:0x104]!=b'SEGA':raise ValueError('Invalid built ROM header')
    # SGDK pads first. Update the header's declared end to that actual extent;
    # this header field is outside the checksum range beginning at 0x200.
    struct.pack_into('>I',data,0x1A4,len(data)-1);rom.write_bytes(data)
    print(f'Color showcase: {rom} ({len(data):,} bytes)')
    return 0


if __name__=='__main__':raise SystemExit(main())
