#!/usr/bin/env python3
"""Copy and generate a standalone Streaming Lab in a new directory."""
import argparse
from pathlib import Path
import shutil
import sys
from stream_art import assets

ROOT=Path(__file__).resolve().parents[1]


def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if any(output.is_relative_to(ROOT/name) for name in ('inc','src','tools','res')):
        raise ValueError('Output cannot be inside a source directory')
    generated=assets()
    output.mkdir(parents=True)
    for name in ('inc','src','tools'):
        shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in generated.items():
        (output/('inc' if name.endswith('.h') else 'res')/name).write_bytes(data)
    (output/'res/resources.res').write_text('BIN stream_hero "hero.bin" 4 4 0 NONE FALSE\nBIN stream_pages "pages.bin" 4 4 0 NONE TRUE\n',encoding='utf-8')
    return output


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args()
    try:print(prepare(args.output))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
