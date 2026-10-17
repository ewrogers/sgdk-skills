#!/usr/bin/env python3
"""Generate a fresh standalone Combat Lab with original native pixel art/audio."""
import argparse
import math
from pathlib import Path
import shutil
import struct
import sys
import wave
from combat_art import assets
from combat_audio import assets as music
ROOT=Path(__file__).resolve().parents[1]
SOUNDS={'swing':(.09,220,500),'fire':(.14,900,200),'impact':(.14,170,60),'block':(.18,1400,900),'fall':(.3,220,45),'join':(.2,440,880),'win':(.8,660,1320)}
def prepare(output):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    generated={**assets(),**music()};output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in generated.items():(output/('inc' if name.endswith('.h') else 'res')/name).write_bytes(data)
    text='IMAGE court "court.png" NONE ALL\nIMAGE title "title.png" NONE ALL\nXGM2 court_music "court.vgm"\n'
    for name,w,h in [('fighters',3,4),('bolts',1,1),('sparks',2,2),('swords',3,3)]:text+=f'SPRITE {name} "{name}.png" {w} {h} NONE 0 NONE NONE\n'
    for name,(seconds,lo,hi) in SOUNDS.items():
        length=int(seconds*13300);phase=0;samples=[]
        for i in range(length):
            t=i/length;freq=lo+(hi-lo)*t;phase+=2*math.pi*freq/13300
            envelope=min(1,i/80)*(1-t)**2
            samples.append(int(10000*envelope*(.8*math.sin(phase)+.2*math.sin(phase*2))))
        with wave.open(str(output/'res'/f'{name}.wav'),'wb') as wav:
            wav.setparams((1,2,13300,0,'NONE','not compressed'));wav.writeframes(b''.join(struct.pack('<h',v)for v in samples))
        text+=f'WAV snd_{name} "{name}.wav" XGM2\n'
    (output/'res/resources.res').write_text(text);return output
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    try:print(prepare(p.parse_args().output))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
