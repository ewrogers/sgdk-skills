#!/usr/bin/env python3
"""Reproduce the experimental lighting study without PixelLab calls."""
import argparse
from pathlib import Path
import tempfile
from build_shadow_rom import ROOT,THEMES,write_comparison


def generate(destination):
    if destination.exists():raise ValueError('Destination exists; choose a new directory')
    destination.mkdir(parents=True)
    for theme in THEMES:
        write_comparison(ROOT/'raw'/f'{theme}-background.png',ROOT/'plans'/f'{theme}-background-banks.json',destination/theme)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true')
    parser.add_argument('--output',type=Path,default=ROOT/'shadow-study');args=parser.parse_args()
    if args.check:
        with tempfile.TemporaryDirectory()as temp:
            generated=Path(temp)/'study';generate(generated)
            for file in generated.rglob('*'):
                if file.is_file()and file.read_bytes()!=(args.output/file.relative_to(generated)).read_bytes():
                    raise ValueError('Export differs: '+str(file.relative_to(generated)))
        print('All four same-palette lighting comparisons reproduced')
    else:generate(args.output);print('Prepared shadow/highlight study')


if __name__=='__main__':main()
