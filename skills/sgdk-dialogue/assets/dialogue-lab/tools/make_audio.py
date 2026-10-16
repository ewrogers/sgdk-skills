#!/usr/bin/env python3
"""Original gentle 6/8 harbor tune and finite dialogue cues, composed offline."""
import argparse
import io
import math
from pathlib import Path
import struct
import wave

FNUM = [644,682,722,766,810,860,911,964,1022,1082,1147,1217]
MELODY = [
    [62,65,69,None,67,65], [62,65,67,None,65,62],
    [60,64,67,None,64,62], [60,65,69,None,67,65],
    [58,62,65,None,62,60], [58,62,67,None,65,62],
    [62,65,69,72,69,65], [61,64,69,None,64,61],
]
# Root, third and fifth: Dm, G, C, F, Bb, Gm, Dm, A.
CHORDS = [(38,41,45),(43,47,50),(36,40,43),(41,45,48),
          (34,38,41),(43,46,50),(38,41,45),(45,49,52)]
EIGHTHS = [19,17,18,19,17,18]  # gentle lilt; dotted-quarter pulse about 67 BPM


def music():
    header = bytearray(256); header[:4] = b'Vgm '
    frames = sum(EIGHTHS) * 16
    for offset, value in [(8,0x160),(0x18,frames*735),(0x24,60),(0x2C,7670454),(0x34,204)]:
        struct.pack_into('<I',header,offset,value)
    data = bytearray()
    def port(reg,value,second=False): data.extend((0x53 if second else 0x52,reg,value))
    def ym(ch,reg,value): port(reg+ch%3,value,ch>=3)
    def key(ch,on): port(0x28,(0xF0 if on else 0)|(ch if ch<3 else ch+1))
    def note(ch,midi):
        block=midi//12-1; fnum=FNUM[midi%12]
        key(ch,False); ym(ch,0xA4,(block<<3)|(fnum>>8)); ym(ch,0xA0,fnum&255); key(ch,True)
    port(0x22,0); port(0x27,0); port(0x2B,0)
    for ch in range(6): key(ch,False)
    for ch in range(5):
        for slot in (0,4,8,12):
            carrier=slot in (8,12)
            level=([35,53,56,40,52][ch] if carrier else [70,80,82,85,76][ch])
            attack=[18,8,7,24,16][ch]
            for reg,value in [(0x30,1),(0x40,level),(0x50,attack),(0x60,5 if ch in (0,4) else 2),
                              (0x70,2),(0x80,0x57 if ch in (0,4) else 0x48),(0x90,0)]:
                ym(ch,reg+slot,value)
        ym(ch,0xB0,4); ym(ch,0xB4,0x80 if ch==1 else 0x40 if ch==2 else 0xC0)
    struct.pack_into('<I',header,0x1C,256+len(data)-0x1C)
    struct.pack_into('<I',header,0x20,frames*735)
    for bar in range(16):
        root,third,fifth=CHORDS[bar%8]
        for beat,length in enumerate(EIGHTHS):
            if beat==0:
                note(1,third+24); note(2,fifth+24)
            if beat in (0,3): note(3,root if beat==0 else root+7)
            pitch=MELODY[bar%8][beat]
            if pitch is not None: note(0,pitch)
            if bar>=8 and beat==3: note(4,fifth+36)
            for frame in range(length):
                if frame==length-3 and beat+1<6 and MELODY[bar%8][beat+1] is not None: key(0,False)
                if frame==length-5: key(3,False)
                if frame==length-1: key(4,False)
                data.append(0x62)
    data.append(0x66)
    struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def effect(name):
    phrases = {'tick':[(720,.055)], 'turn':[(440,.05),(660,.08)],
               'choice':[(440,.05),(550,.05),(660,.09)], 'done':[(660,.08),(550,.08),(440,.18)]}
    samples=[]; rate=13300
    for pitch,duration in phrases[name]:
        length=round(duration*rate)
        for i in range(length):
            t=i/rate
            value=math.sin(2*math.pi*pitch*t)+.12*math.sin(4*math.pi*pitch*t)
            envelope=min(1,i/40,(length-1-i)/80)*math.exp(-t*15)
            samples.append(round(value*max(0,envelope)*(3000 if name=='tick' else 5500)))
    output=io.BytesIO()
    with wave.open(output,'wb') as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'))
        wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return output.getvalue()


def assets():
    return {'harbor.vgm':music(), **{name+'.wav':effect(name) for name in ['tick','turn','choice','done']}}


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path); parser.add_argument('--force',action='store_true')
    args=parser.parse_args(); files=assets()
    if not args.force and any((args.output/name).exists() for name in files): parser.error('Output exists; choose a new folder or use --force')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Wrote one original shanty loop and four finite cues')


if __name__=='__main__': main()
