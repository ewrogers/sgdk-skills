#!/usr/bin/env python3
"""Create original arcade FM/PSG music and short PCM cues for Shooter Lab."""
import argparse
import io
import math
from pathlib import Path
import struct
import wave


def music(victory=False):
    step=12 if victory else 12
    notes=([644,810,964,1287,964,1287,1445,1620,1445,1287,964,1287] if victory else
           [644,766,964,766,1082,964,766,722,860,1082,1287,1082,964,860,766,722,
            644,810,964,1287,1217,1082,964,810,860,964,1082,1217,1082,964,722,644])
    beats=len(notes) if victory else 64
    frames=step*beats
    header=bytearray(256); header[:4]=b'Vgm '
    for offset,value in [(8,0x160),(12,3579545),(0x18,frames*735),
                         (0x24,60),(0x2C,7670454),(0x34,204)]:
        struct.pack_into('<I',header,offset,value)
    struct.pack_into('<H',header,0x28,9); header[0x2A]=16
    data=bytearray()
    def ym(reg,value): data.extend((0x52,reg,value))
    ym(0x22,0); ym(0x27,0); ym(0x2B,0)
    for channel in range(6): ym(0x28,channel if channel<3 else channel+1)
    for value in (0x9F,0xBF,0xDF,0xFF): data.extend((0x50,value))
    for ch in range(3):
        for slot,multiple,level in [(0,2,40),(4,1,127),(8,1,23 if ch==0 else 32),(12,1,127)]:
            for reg,value in [(0x30,multiple),(0x40,level),(0x50,31),(0x60,7),(0x70,3),(0x80,0x48),(0x90,0)]:
                ym(reg+slot+ch,value)
        ym(0xB0+ch,4); ym(0xB4+ch,0xC0)
    if not victory:
        struct.pack_into('<I',header,0x1C,256+len(data)-0x1C)
        struct.pack_into('<I',header,0x20,frames*735)
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step
            root=(644,860,964,644)[(beat//8)%4]
            for ch,note,block in [(0,notes[beat%len(notes)],4),
                                  (1,root if beat%2==0 else root*3//2,2),
                                  (2,notes[(beat+4)%len(notes)],3)]:
                ym(0x28,ch); ym(0xA4+ch,(block<<3)|(note>>8)); ym(0xA0+ch,note&255)
                if ch!=2 or beat%2: ym(0x28,0xF0|ch)
            data.extend((0x50,0xE4,0x50,0xFA))
        if frame%step==3: data.extend((0x50,0xFF))
        if frame%step==step-3:
            for ch in range(3): ym(0x28,ch)
        data.append(0x62)
    data.append(0x66)
    struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def sound(kind):
    sequences={'fire':[(1320,.025),(880,.025)],
               'coin':[(1760,.04),(2640,.15)],
               'bomb':[(110,.12),(73,.2),(55,.15)],
               'warning':[(440,.1),(220,.1),(440,.1)],
               'hit':[(330,.05),(220,.05),(110,.12)],
               'kill':[(440,.035),(220,.06)]}
    samples=[]; rate=13300
    for frequency,duration in sequences[kind]:
        for i in range(round(rate*duration)):
            t=i/rate
            envelope=min(1,t/.003)*math.exp(-t*14)
            signal=math.sin(2*math.pi*frequency*t)+.2*math.sin(4*math.pi*frequency*t)
            samples.append(round(signal*envelope*.5*32767))
    output=io.BytesIO()
    with wave.open(output,'wb') as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'))
        wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return output.getvalue()


def assets():
    return {'course.vgm':music(),'victory.vgm':music(True),
            **{name+'.wav':sound(name) for name in ['fire','coin','bomb','warning','hit','kill']}}


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path)
    args=parser.parse_args(); files=assets()
    if any((args.output/name).exists() for name in files): parser.error('Choose a new output folder')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created original FM/PSG music and six PCM cues')


if __name__=='__main__': main()
