#!/usr/bin/env python3
"""Build the Asterfall catalog in a new folder using the published SGDK image."""
import argparse,os,struct,subprocess
from pathlib import Path
from prepare import prepare


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--preview',choices=['title','naming','saves','world','field','menu','status','equipment','inventory','spells','journal','dialogue','options'])
    parser.add_argument('--platform');parser.add_argument('--mode',choices=['release','debug'],default='release');args=parser.parse_args()
    prepare(args.output)
    if args.preview:
        source=args.output/'src/main.c'
        source.write_text('#define ASTERFALL_PREVIEW_SCREEN '+args.preview.upper()+'\n'+source.read_text())
    cmd=['docker','run','--rm']
    if args.platform:cmd+=['--platform',args.platform]
    if hasattr(os,'getuid'):cmd+=['--user',f'{os.getuid()}:{os.getgid()}']
    cmd+=['--mount',f'type=bind,source={args.output.resolve()},target=/workspace','ewrogers/sgdk:latest','make',args.mode]
    result=subprocess.run(cmd)
    if result.returncode:return result.returncode
    path=args.output/'out/rom.bin';data=bytearray(path.read_bytes())
    if data[0x100:0x104]!=b'SEGA':raise ValueError('Invalid ROM header')
    struct.pack_into('>I',data,0x1a4,len(data)-1);path.write_bytes(data)
    print(f'Asterfall: {path} ({len(data):,} bytes)')
    return 0


if __name__=='__main__':raise SystemExit(main())
