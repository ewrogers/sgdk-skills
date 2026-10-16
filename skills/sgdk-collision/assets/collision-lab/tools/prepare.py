#!/usr/bin/env python3
"""Copy and generate Collision Lab in a fresh standalone directory."""
import argparse
from pathlib import Path
import shutil
import sys
from collision_art import assets
ROOT=Path(__file__).resolve().parents[1]

def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    generated=assets();output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in generated.items():(output/('inc' if name.endswith('.h') else 'res')/name).write_bytes(data)
    sizes={'actor':(2,2),'shot':(1,1),'wall':(1,4),'dot':(1,1),'cursor':(1,1)}
    (output/'res/resources.res').write_text(''.join(f'SPRITE {name} "{name}.png" {w} {h} NONE 0 NONE NONE\n' for name,(w,h) in sizes.items()))
    return output

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
    try:print(prepare(parser.parse_args().output))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
