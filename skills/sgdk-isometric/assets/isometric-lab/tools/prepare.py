#!/usr/bin/env python3
"""Generate a fresh standalone Isometric Lab. No third-party Python dependencies."""
import argparse
from pathlib import Path
import shutil
import sys
from iso_art import assets
from iso_audio import assets as music
ROOT=Path(__file__).resolve().parents[1]

def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    generated={**assets(),**music()};output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in generated.items():(output/('inc' if name.endswith('.h') else 'res')/name).write_bytes(data)
    sizes={'hero':(3,4),'deck':(4,3),'post':(2,5),'ramp_l':(4,6),'ramp_r':(4,6),'tower':(4,8),'tree':(4,6),'fountain':(4,4),'marker':(2,1)}
    text='IMAGE ground "ground.png" NONE ALL\nXGM2 garden_music "garden.vgm"\n'
    text+=''.join(f'SPRITE {name} "{name}.png" {w} {h} NONE 0 NONE NONE\n' for name,(w,h) in sizes.items())
    (output/'res/resources.res').write_text(text)
    return output

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    try:print(prepare(p.parse_args().output))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
