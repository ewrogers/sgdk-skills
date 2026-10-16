#!/usr/bin/env python3
"""Copy a fresh standalone Hardware Lab and generate its original resources."""
import argparse
import math
from pathlib import Path
import shutil
import struct
import sys
import wave
from hardware_glyphs import rows
from hardware_audio import assets as music
ROOT=Path(__file__).resolve().parents[1]

def font_words():
    words=[]
    for code in range(32,128):
        for bits in rows(chr(code)):
            pixels=[1]+[15 if bits&(1<<(4-x)) else 1 for x in range(5)]+[1,1]
            words.append(sum(p<<(28-4*i) for i,p in enumerate(pixels)))
    return words

def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in music().items():(output/'res'/name).write_bytes(data)
    # Seamless 256-sample reference: six complete cycles, no restart gap.
    with wave.open(str(output/'res/reference.wav'),'wb') as wav:
        wav.setparams((1,2,13300,0,'NONE','not compressed'))
        wav.writeframes(b''.join(struct.pack('<h',int(7500*math.sin(2*math.pi*6*i/256)))for i in range(256)))
    (output/'inc/font.h').write_text('/* Original 5x7 opaque font. */\nstatic const u32 font_tiles[768]={\n'+',\n'.join(f'0x{v:08x}'for v in font_words())+'\n};\n')
    (output/'res/resources.res').write_text('XGM2 reference_music "garden.vgm"\nWAV reference_pcm "reference.wav" XGM2\n')
    return output

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    try:print(prepare(p.parse_args().output))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
