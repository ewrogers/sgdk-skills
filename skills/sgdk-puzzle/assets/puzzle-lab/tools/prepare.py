#!/usr/bin/env python3
"""Generate a fresh standalone Puzzle Lab with original native pixel art/audio."""
import argparse
import math
from pathlib import Path
import shutil
import struct
import sys
import wave
from puzzle_art import assets
from puzzle_audio import assets as music
ROOT=Path(__file__).resolve().parents[1]
SOUNDS={'step':(.045,260,220),'push':(.12,140,260),'blocked':(.065,100,60),'undo':(.15,600,240),'bridge':(.3,220,800),'win':(.8,440,1320)}
def prepare(output,lab="garden"):
    if lab not in ('garden','blocks'):raise ValueError('Unknown Lab')
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    generated={**assets(),**music()};output.mkdir(parents=True)
    for name in ('inc','src','tools','variants'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in generated.items():(output/('inc' if name.endswith('.h') else 'res')/name).write_bytes(data)
    text='IMAGE garden "garden.png" NONE ALL\nIMAGE atlas "atlas.png" NONE ALL\nXGM2 garden_music "garden.vgm"\n'
    for name,w,h in [('hero',2,3),('crate',2,2)]:text+=f'SPRITE {name} "{name}.png" {w} {h} NONE 0 NONE NONE\n'
    for name,(seconds,lo,hi) in SOUNDS.items():
        length=int(seconds*13300);phase=0;samples=[]
        for i in range(length):
            t=i/length;freq=lo+(hi-lo)*t;phase+=2*math.pi*freq/13300
            envelope=min(1,i/80)*(1-t)**2
            samples.append(int(10000*envelope*(.8*math.sin(phase)+.2*math.sin(phase*2))))
        with wave.open(str(output/'res'/f'{name}.wav'),'wb') as wav:
            wav.setparams((1,2,13300,0,'NONE','not compressed'));wav.writeframes(b''.join(struct.pack('<h',v)for v in samples))
        text+=f'WAV snd_{name} "{name}.wav" XGM2\n'
    (output/'res/resources.res').write_text(text)
    shutil.copyfile(output/'variants'/('blocks_main.c' if lab=='blocks' else 'garden_main.c'),output/'src/main.c')
    header=output/'src/boot/rom_head.c';header.parent.mkdir(exist_ok=True)
    header_text=(output/'variants/rom_head.c').read_text()
    if lab=='blocks':header_text=header_text.replace('CLOCKWORK GARDEN PUZZLE','FALLING LANTERNS BLOCK LAB').replace('0000063','0000064')
    header.write_text(header_text)
    return output
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--lab',choices=['garden','blocks'],default='garden')
    try:
        args=p.parse_args();print(prepare(args.output,args.lab))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
