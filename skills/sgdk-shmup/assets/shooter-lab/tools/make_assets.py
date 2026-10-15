#!/usr/bin/env python3
"""Draw Shooter Lab's original indexed space art at its final pixel size."""
import argparse
from pathlib import Path
import struct
import zlib

PALETTES=[
    [(0,0,0),(0,0,36),(0,36,73),(36,73,109),(73,109,146),(109,146,182),(182,219,255),(255,255,255)]+[(0,0,0)]*8,
    [(0,0,0),(36,36,73),(73,73,109),(109,109,146),(182,182,219),(219,219,255),(255,255,255),(255,255,255),
     (73,36,36),(146,73,36),(219,109,36),(255,182,73),(36,73,109),(73,146,219),(146,219,255),(255,255,255)],
    [(0,0,0),(36,36,36),(73,73,73),(109,109,109),(182,182,182),(219,219,219),(255,255,255),(255,255,255),
     (109,36,36),(182,73,36),(255,146,73),(255,219,146),(73,36,109),(146,73,182),(219,146,255),(255,219,255)],
    [(0,0,0),(73,36,36),(146,36,73),(219,73,109),(255,146,182),(255,219,219),(255,255,255),(255,255,255),
     (73,73,36),(146,146,36),(219,219,73),(255,255,146),(36,73,109),(73,146,219),(146,219,255),(255,255,255)]
]

def png(width,height,pixels,palette):
    def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width]) for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+chunk(b'PLTE',bytes(v for rgb in palette for v in rgb))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')

class Sheet:
    def __init__(self,w,h,frames): self.w,self.h,self.n=w,h,frames; self.data=[0]*(w*h*frames)
    def dot(self,f,x,y,c):
        if 0<=x<self.w and 0<=y<self.h: self.data[y*self.w*self.n+f*self.w+x]=c
    def box(self,f,x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w): self.dot(f,xx,yy,c)
    def image(self,p): return png(self.w*self.n,self.h,self.data,p)

def assets():
    ship=Sheet(16,16,4)
    for f in range(4):
        for y in range(1,14):
            for x in range(16):
                if abs(2*x-15)<=y+1: ship.dot(f,x,y,1 if abs(2*x-15)>y-2 else 4)
        ship.box(f,6,4,4,6,12); ship.box(f,7,4,2,4,14)
        ship.box(f,2,11,3,3,10); ship.box(f,11,11,3,3,10)
        ship.box(f,6,13,4,2,11 if f&1 else 10)
        if f>=2:
            ship.box(f,5,7,6,6,1); ship.box(f,6,8,4,4,7)
    enemies=Sheet(16,16,6)
    for kind in range(3):
        for phase in range(2):
            f=kind*2+phase
            enemies.box(f,2,3,12,8,1); enemies.box(f,3,4,10,6,9 if kind==0 else 13 if kind==1 else 3)
            enemies.box(f,0,6,3,6,8 if kind<2 else 2); enemies.box(f,13,6,3,6,8 if kind<2 else 2)
            enemies.box(f,6,7,4,6,10 if kind<2 else 14)
            enemies.box(f,5,4,6,2,6 if phase else 11)
            if kind==2: enemies.box(f,4,1,8,3,4)
    boss=Sheet(32,32,3)
    for f in range(3):
        for y in range(4,26):
            for x in range(2,30):
                if abs(x-16)<14-abs(y-14)//3: boss.dot(f,x,y,1 if x in (2,29) or y in (4,25) else 9 if f<2 else 13)
        boss.box(f,9,5,14,16,2); boss.box(f,11,7,10,10,3)
        boss.box(f,13,10,6,6,11 if f<2 else 15)
        for x in (4,24): boss.box(f,x,15,4,13,2); boss.box(f,x+1,21,2,6,10 if f&1 else 14)
        boss.box(f,8,26,16,3,1)
    bullets=Sheet(8,8,3)
    for f in range(3):
        for y in range(8):
            for x in range(8):
                if f==0 and 2<=x<=5 and 1<=y<=6: bullets.dot(f,x,y,14 if x in (2,5) else 15)
                if f==1 and (2*x-7)**2+(2*y-7)**2<=32: bullets.dot(f,x,y,2 if x+y>8 else 4)
                if f==2 and abs(x-3)+abs(y-3)<=3: bullets.dot(f,x,y,10 if abs(x-3)+abs(y-3)>1 else 15)
    pickups=Sheet(16,16,2)
    for f in range(2):
        pickups.box(f,2,2,12,12,8 if not f else 12); pickups.box(f,3,3,10,10,10 if not f else 13)
        if f==0:
            pickups.box(f,5,4,2,8,15); pickups.box(f,7,4,4,2,15); pickups.box(f,7,7,4,2,15); pickups.box(f,9,5,2,3,15)
        else:
            pickups.box(f,5,4,2,8,15); pickups.box(f,7,4,3,2,15); pickups.box(f,7,7,3,2,15); pickups.box(f,7,10,3,2,15); pickups.box(f,9,5,2,6,15)
    effects=Sheet(16,16,4)
    for f in range(4):
        for y in range(16):
            for x in range(16):
                d=abs(x-7)+abs(y-7)
                if f*2<=d<=f*2+3: effects.dot(f,x,y,15 if f==0 else 10 if f<3 else 3)
    stars=[1]*(256*256)
    seed=211
    for i in range(110):
        seed=(seed*1664525+1013904223)&0xffffffff; x=(seed>>16)&255
        seed=(seed*1664525+1013904223)&0xffffffff; y=(seed>>16)&255
        stars[y*256+x]=3+i%4
        if i%17==0 and x<255: stars[y*256+x+1]=3
    return {'ship.png':ship.image(PALETTES[1]),'enemies.png':enemies.image(PALETTES[2]),
            'boss.png':boss.image(PALETTES[2]),'bullets.png':bullets.image(PALETTES[3]),
            'pickups.png':pickups.image(PALETTES[3]),'effects.png':effects.image(PALETTES[3]),
            'stars.png':png(256,256,stars,PALETTES[0])}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
    args=parser.parse_args();files=assets()
    if any((args.output/name).exists() for name in files): parser.error('Choose a new output folder; source art exists')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created seven original native-resolution indexed sheets')

if __name__=='__main__': main()
