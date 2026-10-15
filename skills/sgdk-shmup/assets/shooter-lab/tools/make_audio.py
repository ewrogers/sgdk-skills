#!/usr/bin/env python3
"""Create original arcade FM/PSG music and short PCM cues for Shooter Lab."""
import argparse
import io
import math
from pathlib import Path
import struct
import wave


def music(victory=False, boss=False, combat=False, second=False, over=False):
    step=12 if victory else 24 if boss else 40 if second else 30 if combat else 60
    notes=([644,810,964,1287,964,1287,1445,1620,1445,1287,964,1287] if victory else
           [644,766,964,766,1082,964,766,722,860,1082,1287,1082,964,860,766,722,
            644,810,964,1287,1217,1082,964,810,860,964,1082,1217,1082,964,722,644])
    beats=len(notes) if victory else 32
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
                ym(reg+slot+ch, (10 if reg==0x50 and ch!=0 else 4 if reg==0x60 else 1 if reg==0x70 else value) if not victory else value)
        ym(0xB0+ch,4); ym(0xB4+ch,0xC0)
    if not victory:
        struct.pack_into('<I',header,0x1C,256+len(data)-0x1C)
        struct.pack_into('<I',header,0x20,frames*735)
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step
            root=((644,574,511,574) if over else (644,574,766,608) if second else (644,860,964,766) if combat else (644,766,574,722))[(beat//8)%4]
            if victory:
                voices=[(0,notes[beat%len(notes)],4),(1,root,2),(2,notes[(beat+4)%len(notes)],3)]
            elif combat:
                # Optimistic climbing motifs over a steady low pulse. Leave
                # room for firing cues; no busy pop arpeggio or constant hats.
                melody=(964,860,766,722,644,766,574,608) if second else (644,964,1082,1287,1147,964,860,1082)
                voices=[(0,melody[beat%8],4),(1,root if beat%2==0 else root*3//2,2)]
                if beat%4==0: voices.append((2,root*3//2,3))
            else:
                # Slow minor pads and widely spaced bell tones, not an arcade jig.
                melody=(964,860,766,644,722,574,644,483) if over else (1287,964,1532,1147,1445,964,860,1147)
                voices=[(0,melody[(beat//2)%8],4)] if beat%2==0 else []
                if beat%(2 if boss else 4)==0:
                    voices += [(1,root,2),(2,root*3//2,3)]
            for ch,note,block in voices:
                ym(0x28,ch); ym(0xA4+ch,(block<<3)|(note>>8)); ym(0xA0+ch,note&255)
                ym(0x28,0xF0|ch)
            if victory or boss:
                data.extend((0x50,0xE4,0x50,0xFC))
            elif beat%4==0:
                # Quiet sustained PSG shimmer.
                data.extend((0x50,0x85,0x50,0x24,0x50,0x9C))
        if (victory or boss) and frame%step==3: data.extend((0x50,0xFF))
        if victory and frame%step==step-3:
            for ch in range(3): ym(0x28,ch)
        data.append(0x62)
    data.append(0x66)
    struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def sound(kind):
    if kind=='hit':
        rate=13300; samples=[]; seed=771
        for i in range(rate//2):
            t=i/rate; seed=(seed*1664525+1013904223)&0xffffffff
            noise=((seed>>16)/32768-1)
            phase=2*math.pi*(600*t-900*t*t+500*t*t*t)
            value=(.5*math.sin(phase)+.22*math.sin(phase*.5)+.28*noise)*math.exp(-t*7)
            samples.append(round(value*27000*min(1,i/40)))
        output=io.BytesIO()
        with wave.open(output,'wb') as wav:
            wav.setparams((1,2,rate,0,'NONE','not compressed'))
            wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
        return output.getvalue()
    sequences={'empty':[(180,.025),(120,.025)],'fire':[(1320,.025),(880,.025)],
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
    return {'title.vgm':music(),'stage2.vgm':music(combat=True,second=True),'game_over.vgm':music(over=True),'course.vgm':music(combat=True),'victory.vgm':music(True),'boss.vgm':music(boss=True),
            **{name+'.wav':sound(name) for name in ['fire','coin','bomb','warning','hit','kill','empty']}}


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path)
    args=parser.parse_args(); files=assets()
    if any((args.output/name).exists() for name in files): parser.error('Choose a new output folder')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created original FM/PSG music and seven PCM cues')


if __name__=='__main__': main()
