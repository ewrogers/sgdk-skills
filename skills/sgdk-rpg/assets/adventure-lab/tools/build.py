#!/usr/bin/env python3
"""Prepare and build Adventure Lab in a new directory through SGDK Docker."""
import argparse,os,struct,subprocess
from pathlib import Path
from prepare import prepare

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--platform')
    parser.add_argument('--mode',choices=['release','debug'],default='release')
    parser.add_argument('--preview',choices=['title','help','camp','party','route','battle','credits']);args=parser.parse_args()
    prepare(args.output)
    if args.preview:
        source=args.output/'src/main.c';source.write_text('#define ADVENTURE_PREVIEW '+args.preview.upper()+'\n'+source.read_text())
    command=['docker','run','--rm']
    if args.platform:command+=['--platform',args.platform]
    if hasattr(os,'getuid'):command+=['--user',f'{os.getuid()}:{os.getgid()}']
    command+=['--mount',f'type=bind,source={args.output.resolve()},target=/workspace','ewrogers/sgdk:latest','make',args.mode]
    result=subprocess.run(command)
    if result.returncode:return result.returncode
    rom=args.output/'out/rom.bin';data=bytearray(rom.read_bytes());struct.pack_into('>I',data,0x1a4,len(data)-1);rom.write_bytes(data)
    print(f'Adventure Lab: {rom} ({len(data):,} bytes)');return 0
if __name__=='__main__':raise SystemExit(main())
