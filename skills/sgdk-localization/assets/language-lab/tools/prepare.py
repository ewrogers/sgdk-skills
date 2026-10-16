#!/usr/bin/env python3
"""Prepare a standalone Language Lab in a new directory."""
import argparse
from pathlib import Path
import shutil
import sys
from catalog import build
from loc_art import ferry

ROOT=Path(__file__).resolve().parents[1]


def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if any(output.is_relative_to(ROOT/name) for name in ('inc','src','tools','res')):
        raise ValueError('Output cannot be inside a source directory')
    generated,report=build(ROOT/'res/catalog.json')
    output.mkdir(parents=True)
    for name in ('inc','src','tools','res'):
        shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    for name,content in generated.items():
        path=output/name;path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('xb') as destination:destination.write(content)
    (output/'res/ferry.png').write_bytes(ferry())
    with (output/'res/resources.res').open('a',encoding='utf-8') as resources:
        resources.write('TILESET ferry_tiles "ferry.png" NONE NONE ROW\n')
    return output,report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path);args=parser.parse_args()
    try:
        output,report=prepare(args.output)
        print(f'{output}: {report["font"]["tiles"]} font tiles, {report["text_bytes"]} text bytes')
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
