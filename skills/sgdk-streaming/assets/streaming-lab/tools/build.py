#!/usr/bin/env python3
"""Build a fresh Streaming Lab with the SGDK Docker toolchain."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
from prepare import prepare


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--platform',choices=['linux/arm64','linux/amd64'])
    parser.add_argument('--mode',choices=['release','debug'],default='release')
    parser.add_argument('--region',choices=['auto','ntsc','pal'],default='auto')
    parser.add_argument('--demo',action='store_true',help='Automatically walk both directions using the slow upload budget')
    args=parser.parse_args()
    try:
        output=prepare(args.output)
        if args.region!='auto':
            path=output/'src/boot/rom_head.c'
            path.write_text(path.read_text().replace('.region="JUE"','.region="'+('E' if args.region=='pal' else 'U')+'"'))
        command=['docker','run','--rm']
        if args.platform:command+=['--platform',args.platform]
        if hasattr(os,'getuid'):command+=['--user',f'{os.getuid()}:{os.getgid()}']
        command+=['--mount',f'type=bind,source={output},target=/workspace','ewrogers/sgdk:latest','make',args.mode]
        if args.demo:command+=['EXTRA_FLAGS=-DSC_AUTOPLAY']
        result=subprocess.run(command)
        if result.returncode:return result.returncode
        print(f'Streaming Lab: {output/"out/rom.bin"}')
        return 0
    except (OSError,ValueError) as error:print(error,file=sys.stderr);return 2


if __name__=='__main__':raise SystemExit(main())
