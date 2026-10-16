#!/usr/bin/env python3
"""Build a normal/S/H comparison without changing the gallery's source art."""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parents[1]/'scripts'))
from shadow_highlight import write_comparison
THEMES=['modern','fantasy','sci-fi','colonial']


def make_project(destination):
    if destination.exists():raise ValueError('Destination exists; choose a new directory')
    shutil.copytree(ROOT/'shadow-rom',destination);(destination/'res').mkdir(exist_ok=True)
    lines=[];reports={}
    for theme in THEMES:
        name=theme.replace('-','');folder=destination/'res'/theme
        reports[theme]=write_comparison(ROOT/'raw'/f'{theme}-background.png',ROOT/'plans'/f'{theme}-background-banks.json',folder)
        for variant in ['normal','lit']:
            lines.append(f'IMAGE {name}_{variant} "{theme}/{variant}.png" NONE')
        for half in range(2):lines.append(f'SPRITE {name}_mask{half} "{theme}/mask-{half}.png" 16 16 NONE 0 NONE')
        for kind,dimensions in [('actor','4 4'),('object','2 2')]:
            source=f'{theme}-{kind}.png';shutil.copyfile(ROOT/'final'/source,destination/'res'/source)
            lines.append(f'SPRITE {name}_{kind} "{source}" {dimensions} NONE 0 NONE')
    (destination/'res/resources.res').write_text('\n'.join(lines)+'\n')
    (destination/'comparison-report.json').write_text(json.dumps(reports,indent=2)+'\n')
    return reports


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--platform');parser.add_argument('--mode',choices=['release','debug'],default='release');args=parser.parse_args()
    make_project(args.output);cmd=['docker','run','--rm']
    if args.platform:cmd+=['--platform',args.platform]
    if hasattr(os,'getuid'):cmd+=['--user',f'{os.getuid()}:{os.getgid()}']
    cmd+=['--mount',f'type=bind,source={args.output.resolve()},target=/workspace','ewrogers/sgdk:latest','make',args.mode]
    result=subprocess.run(cmd)
    if result.returncode:return result.returncode
    path=args.output/'out/rom.bin';rom=bytearray(path.read_bytes())
    if rom[0x100:0x104]!=b'SEGA':raise ValueError('Invalid ROM header')
    struct.pack_into('>I',rom,0x1A4,len(rom)-1);path.write_bytes(rom)
    print(f'Shadow/highlight comparison: {path} ({len(rom):,} bytes)')
    return 0


if __name__=='__main__':sys.exit(main())
