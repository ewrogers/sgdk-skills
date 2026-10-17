#!/usr/bin/env python3
"""Export a new standalone Sound Desk with original indexed art and audio."""
import argparse
from pathlib import Path
import runpy
import shutil
import sys
from desk_audio import assets as audio,TRACKS
from desk_art import assets as art
ROOT=Path(__file__).resolve().parents[1]

def prepare(output):
    output=Path(output).resolve()
    if output.exists()or output.is_relative_to(ROOT):raise ValueError('Choose a new output directory outside the source Lab')
    legacy=ROOT/'tools/legacy_audio.py'
    if not legacy.exists():legacy=ROOT.parents[1]/'scripts/make_lab_audio.py'
    files={**runpy.run_path(str(legacy))['assets'](),**audio(),**art()}
    output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    if legacy.resolve()!=(output/'tools/legacy_audio.py').resolve():shutil.copyfile(legacy,output/'tools/legacy_audio.py')
    (output/'res').mkdir()
    for name,data in files.items():(output/('inc'if name.endswith('.h')else'res')/name).write_bytes(data)
    text='IMAGE desk "desk.png" NONE ALL\nXGM2 music "music.vgm"\nWAV click "click.wav" XGM2\nWAV alarm "alarm.wav" XGM2\n'
    for name in TRACKS:text+=f'XGM2 tune_{name} "{name}.vgm"\n'
    for name in ('coin','jump','impact','alert'):
        for suffix in ('','_raw'):text+=f'WAV snd_{name}{suffix} "{name}{suffix}.wav" XGM2\n'
    (output/'res/resources.res').write_text(text)
    header=output/'src/boot/rom_head.c';header.parent.mkdir(exist_ok=True)
    header.write_text('#include <genesis.h>\n__attribute__((externally_visible))\nconst ROMHeader rom_header={.console="SEGA MEGA DRIVE",.copyright="(C)26 SGDK LAB",.title_local="SOUND DESK AUDIO WORKSHOP",.title_int="SOUND DESK AUDIO WORKSHOP",.serial="GM 0000065-00",.IOSupport="J",.rom_start=0,.rom_end=0x0003ffff,.ram_start=0xff0000,.ram_end=0xffffff,.region="JUE"};\n')
    return output
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    try:print(prepare(p.parse_args().output))
    except (OSError,ValueError)as e:p.exit(2,str(e)+'\n')
