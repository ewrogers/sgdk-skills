#!/usr/bin/env python3
"""Create original coin art, join/pickup/clear sounds and seven FM themes."""
import argparse
import io
import math
import random
import wave
from pathlib import Path
import struct
import zlib


EFFECT_COLORS=[(0,0,0),(73,36,0),(182,109,0),(255,182,36),(255,219,109),(255,255,219),
               (36,36,36),(73,73,73),(146,146,146),(219,219,219),(73,0,0),(182,36,36),
               (255,146,109),(0,0,0),(255,182,36),(255,219,109)]


def indexed(width,height,pixels):
    def chunk(k,d): return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d))
    rows=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+
            chunk(b'PLTE',bytes(c for rgb in EFFECT_COLORS for c in rgb))+
            chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))


def smoke():
    pixels=[0]*(128*32)
    for frame in range(4):
        radius=[5,7,8,6][frame]
        for y in range(32):
            for x in range(32):
                inside=False; edge=False
                for cx,cy in [(10,10),(21,12),(14,21),(7,19),(23,22)]:
                    distance=(x-cx)**2+(y-cy)**2
                    if distance<=radius**2: inside=True
                    if (radius-1)**2<=distance<=radius**2: edge=True
                if inside and not (frame==3 and (x*3+y*7)%11<3):
                    pixels[y*128+frame*32+x]=7 if edge else 9 if y<15 else 8
    return indexed(128,32,pixels)


def mega_coin():
    pixels=[0]*(128*32)
    for frame in range(4):
        for y in range(32):
            for x in range(32):
                dx,dy=2*x-31,2*y-31; radius=dx*dx+dy*dy
                if radius>28*28: continue
                color=1 if radius>25*25 else 2 if radius>22*22 else 3
                if 8<x<23 and 8<y<23 and ((x in (11,12,19,20)) or y in (10,11,20,21)): color=4
                if abs(x-y-[-12,0,12,24][frame])<2: color=5
                letter=["10001","11011","10101","10001","10001","10001","10001"]
                if 11<=x<21 and 9<=y<23 and letter[(y-9)//2][(x-11)//2]=='1': color=1
                pixels[y*128+frame*32+x]=color
    return indexed(128,32,pixels)


def laser():
    pixels=[0]*(48*16)
    for frame in range(3):
        for y in range(16):
            for x in range(16):
                distance=abs(y-7) if frame==0 else abs(x-y) if frame==1 else abs(x-7)
                if 1<=x<=14 and 1<=y<=14 and distance<3:
                    pixels[y*48+frame*16+x]=5 if distance==0 else 12 if distance==1 else 11
    return indexed(48,16,pixels)


def coins():
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    colors=EFFECT_COLORS
    pixels=[]
    for y in range(8):
        pixels.append(0)
        for frame in range(4):
            radius=[3,2,1,2][frame]
            for x in range(8):
                dx,dy=abs(x-3),abs(y-3)
                color=0
                if dx<=radius and dy<=3 and dx+dy<=radius+2:
                    color=1 if dx==radius or dy==3 else 3 if x<4 else 2
                    if x<3 and y<3: color=5
                    elif dx==0 and dy<2: color=4
                pixels.append(color)
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',32,8,8,3,0,0,0))+
            chunk(b'PLTE',bytes(c for color in colors for c in color))+
            chunk(b'IDAT',zlib.compress(bytes(pixels),9))+chunk(b'IEND',b''))


def sound(kind):
    rate=13300
    if kind=='poof':
        rng=random.Random(416); samples=[]
        for i in range(rate//4):
            t=i/rate; envelope=min(1,t/0.002)*math.exp(-t*18)
            samples.append(round((rng.uniform(-1,1)*0.45+math.sin(2*math.pi*90*t)*0.2)*envelope*32767))
        data=io.BytesIO()
        with wave.open(data,'wb') as wav:
            wav.setparams((1,2,rate,0,'NONE','not compressed')); wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
        return data.getvalue()
    notes={"join":[(659,0.12),(988,0.12),(1318,0.26)],
           "coin":[(1760,0.045),(2637,0.20)],
           "clear":[(523,0.12),(659,0.12),(784,0.12),(1047,0.60)],
           "hit":[(520,0.06),(380,0.06),(260,0.15)],
           "laser":[(1600,0.025),(1000,0.025),(600,0.06)]}[kind]
    samples=[]
    for frequency,duration in notes:
        for i in range(round(rate*duration)):
            t=i/rate
            envelope=min(1,t/0.003)*math.exp(-t*(22 if kind=="coin" else 7))
            signal=(math.sin(2*math.pi*frequency*t)+0.25*math.sin(2*math.pi*frequency*2.01*t))
            samples.append(round(max(-1,min(1,signal*envelope*0.55))*32767))
    data=io.BytesIO()
    with wave.open(data,'wb') as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'))
        wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return data.getvalue()


def music(kind):
    step=[40,20,30,15,24,10,15][kind]; frames=step*(12 if kind==6 else 24)
    notes=[[644,810,964,1287,964,810,722,964,1082,964,810,722],
           [722,910,1082,1445,1287,1082,910,1082,1287,1082,910,722],
           [644,766,964,1147,964,766,574,722,860,1082,860,722],
           [644,810,964,1147,1082,910,860,722,766,964,1082,1287],
           [574,860,1082,1287,1082,860,644,964,1147,1445,1147,964],
           [644,644,766,860,644,964,910,766,574,860,766,722],
           [644,810,964,1287,964,1287,1445,1620,1445,1287,964,1287]][kind]
    header=bytearray(256); header[:4]=b'Vgm '
    for offset,value in [(8,0x160),(12,3579545),(0x18,frames*735),
                         (0x20,frames*735),(0x24,60),(0x2C,7670454),(0x34,204)]:
        struct.pack_into('<I',header,offset,value)
    struct.pack_into('<H',header,0x28,9); header[0x2A]=16
    data=bytearray()
    def ym(reg,value): data.extend([0x52,reg,value])
    ym(0x22,0); ym(0x27,0); ym(0x2B,0)
    for ch in range(6): ym(0x28,ch if ch<3 else ch+1)
    for value in [0x9F,0xBF,0xDF,0xFF]: data.extend([0x50,value])
    for ch in range(3 if kind==5 else 2):
        for slot,multiple,level in [(0,2,42+kind*6),(4,1,127),(8,1,28),(12,1,127)]:
            for reg,value in [(0x30,multiple),(0x40,level),(0x50,20 if kind==2 else 31),
                              (0x60,8),(0x70,3),(0x80,0x48),(0x90,0)]: ym(reg+slot+ch,value)
        ym(0xB0+ch,4); ym(0xB4+ch,0xC0)
    if kind!=6: struct.pack_into('<I',header,0x1C,256+len(data)-0x1C)
    for frame in range(frames):
        onset=frame%30 in (0,18) if kind==3 else frame%step==0
        note=frame//30*2+(frame%30>=18) if kind==3 else frame//step
        if onset:
            for ch,value,block in [(0,notes[note%12],4),(1,notes[(frame//30 if kind==3 else frame//(step*6))%12],2)]:
                ym(0x28,ch); ym(0xA4+ch,(block<<3)|(value>>8)); ym(0xA0+ch,value&255); ym(0x28,0xF0|ch)
        if kind==5:
            if frame%20==0:
                ym(0x28,2); ym(0xA6,(3<<3)|(notes[(frame//20)%12]>>8)); ym(0xA2,notes[(frame//20)%12]&255); ym(0x28,0xF2)
            if frame%20==14: ym(0x28,2)
            if frame%30==0: data.extend([0x50,0xE4,0x50,0xF8])
            if frame%30==3: data.extend([0x50,0xFF])
        if (frame%30 in (15,27) if kind==3 else frame%step==step-3): ym(0x28,0); ym(0x28,1)
        data.append(0x62)
    data.append(0x66); struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def assets():
    files={'coins.png':coins(),'smoke.png':smoke(),'mega.png':mega_coin(),'laser.png':laser()}
    files.update({name+'.wav':sound(name) for name in ['join','coin','clear','hit','poof','laser']})
    files.update({name+'.vgm':music(i) for i,name in enumerate(['menu','room_one','room_two','pause','room_three','boss','victory'])})
    return files


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path)
    args=parser.parse_args(); files=assets()
    if any((args.output/name).exists() for name in files): parser.error('Choose a new destination; generated assets already exist')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print(f'Created {len(files)} original assets in {args.output}')


if __name__=='__main__': main()
