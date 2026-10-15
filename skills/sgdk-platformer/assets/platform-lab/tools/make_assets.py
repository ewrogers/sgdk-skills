#!/usr/bin/env python3
"""Create original native-resolution indexed art and a matching Tiled course.

Writes a new output directory only. No scaling, quantization or external packages.
"""
import argparse
from pathlib import Path
import struct
import zlib

# PAL0 stone, PAL1 player, PAL2 enemies/items, PAL3 HUD (SDK font).
PALETTES = [
    [(0,0,0),(36,36,36),(73,73,73),(109,109,109),(146,146,146),(182,182,182),
     (219,219,219),(255,255,255),(36,73,73),(73,109,109),(109,146,146),
     (146,182,182),(109,73,36),(182,146,73),(219,182,109),(255,219,146)],
    [(0,0,0),(36,36,73),(36,73,109),(73,109,182),(73,146,219),(146,219,255),
     (219,255,255),(255,255,255),(73,36,36),(146,73,36),(219,146,73),
     (255,219,146),(73,73,73),(109,109,109),(182,182,182),(219,219,219)],
    [(0,0,0),(36,36,36),(73,73,73),(146,146,146),(219,219,219),(255,255,255),
     (73,146,73),(146,219,109),(182,109,36),(255,219,73),(146,36,73),
     (255,109,109),(73,73,146),(146,146,255),(73,146,182),(146,219,255)]
]


def png(w,h,pixels,palette):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+bytes(pixels[y*w:(y+1)*w]) for y in range(h))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,3,0,0,0))+
            chunk(b'PLTE',bytes(c for rgb in palette for c in rgb))+
            chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))


class Sheet:
    def __init__(self,w,h,frames):
        self.w,self.h,self.frames=w,h,frames
        self.pixels=[0]*(w*h*frames)
    def dot(self,f,x,y,c):
        if 0<=x<self.w and 0<=y<self.h:
            self.pixels[y*self.w*self.frames+f*self.w+x]=c
    def box(self,f,x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w): self.dot(f,xx,yy,c)
    def image(self,palette):
        return png(self.w*self.frames,self.h,self.pixels,palette)


def layout():
    rows=[list('.'*80) for _ in range(18)]
    def rect(x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w): rows[yy][xx]=c
    rect(0,16,80,2,'#')
    rect(0,0,1,18,'#'); rect(79,0,1,18,'#')
    rect(7,13,4,1,'-'); rect(10,10,4,1,'-'); rect(15,11,4,1,'-')
    rect(18,16,4,2,'.')
    for x,y in [(23,15),(24,14),(25,13)]:
        rows[y][x]='/'; rect(x,y+1,1,16-y,'#')
    rect(26,13,4,4,'#')
    for x,y in [(30,13),(31,14),(32,15)]:
        rows[y][x]='\\'; rect(x,y+1,1,16-y,'#')
    # The chute teaches wall jumps; boots also allow going over the sides.
    rect(38,8,1,8,'#'); rect(41,8,1,8,'#')
    rect(48,16,4,2,'.')
    rect(57,12,1,4,'*')
    rect(66,12,7,4,'?')
    return [''.join(row) for row in rows]


def assets():
    terrain=Sheet(16,16,7)
    for f in range(1,7):
        for y in range(16):
            for x in range(16):
                color=2
                if f==1: color=4 if y==0 else 1 if y==15 or x==15 else 3 if y==1 else 2
                if f==2: color=(10 if y<2 else 8 if y<4 else 0)
                if f in (3,4):
                    edge=15-x if f==3 else x
                    color=4 if y==edge else 2 if y>edge else 0
                if f==5: color=13 if y in (0,7,15) or x in (0,7,15) else 12
                if f==6: color=8 if y in (0,15) or x in (0,15) else 1
                terrain.dot(f,x,y,color)
    hero=Sheet(16,24,10)
    for f in range(10):
        bob=1 if f==1 else 0
        hero.box(f,3,2+bob,10,10,1); hero.box(f,4,3+bob,8,8,4)
        hero.box(f,5,5+bob,7,4,2); hero.box(f,7,5+bob,5,2,6)
        hero.box(f,4,12+bob,8,7,1); hero.box(f,5,12+bob,6,5,3)
        hero.box(f,5,13+bob,3,2,5); hero.box(f,2,13+bob,2,5,4)
        hero.box(f,12,12+bob,2,6,4)
        offset=(0,0,-1,0,1,0,1,1,0,0)[f]
        hero.box(f,4+offset,19,3,4,1); hero.box(f,9-offset,19,3,4,1)
        hero.box(f,4+offset,21,4,2,5); hero.box(f,9-offset,21,4,2,5)
        if f==7: hero.box(f,13,6,2,5,6) # wall contact
        if f>=8:
            hero.box(f,0,14,3,2,10 if f==8 else 11)
            hero.box(f,1,18,2,1,11)
    enemies=Sheet(16,16,20)
    for kind in range(5):
        color=(6,8,10,12,14)[kind]
        for phase in range(4):
            f=kind*4+phase
            y=3+(phase&1)
            enemies.box(f,2,y,12,9,1); enemies.box(f,3,y+1,10,6,color)
            enemies.box(f,8,y+2,5,2,5)
            enemies.box(f,3+(phase&1),y+9,3,2,3)
            enemies.box(f,10-(phase&1),y+9,3,2,3)
            if kind==2: # spring under the hopper
                enemies.box(f,5,12,6,1,11); enemies.box(f,6,14,4,1,11)
            if kind==3:
                enemies.box(f,0,3+(phase&1)*3,3,2,13)
                enemies.box(f,13,3+(phase&1)*3,3,2,13)
            if kind==4 and phase<2:
                enemies.box(f,1,1,14,3,3); enemies.box(f,0,3,3,10,4)
                enemies.box(f,13,3,3,10,4)
    items=Sheet(16,16,13)
    for f in range(8):
        if f==6: # moving platform
            items.box(f,0,0,16,3,14); items.box(f,0,3,16,4,2)
            for x in range(0,16,4): items.box(f,x,4,2,2,15)
            continue
        if f==7: # checkpoint post
            items.box(f,3,0,2,16,3); items.box(f,5,1,9,6,7); continue
        col=(13,9,11,9,15,7)[f]
        for y in range(2,14):
            for x in range(2,14):
                if abs(x-7)+abs(y-7)<=8: items.dot(f,x,y,col if x+y<17 else col-1)
        if f==0:
            items.box(f,5,4,3,7,5); items.box(f,5,9,6,3,5)
        elif f==1:
            for x,y in [(8,3),(7,4),(6,5),(7,6),(8,7),(7,8),(6,9),(5,10)]: items.box(f,x,y,2,2,5)
        elif f==2:
            for x,y in [(5,5),(7,7),(5,9)]: items.box(f,x,y,4,2,5)
        elif f in (3,4): items.box(f,7,5,2,6,5)
        else: items.box(f,5,5,6,6,5)
    # Golden boots overlay the player feet without duplicating the whole sheet.
    for frame,offset in [(8,-1),(9,0),(10,1)]:
        for x in (4+offset,9-offset):
            items.box(frame,x,11,3,3,8)
            items.box(frame,x,12,4,3,9)
            items.box(frame,x,12,3,1,5)
    # Short blue charge arcs become bright gold when the dash is ready.
    for frame,color in [(11,15),(12,9)]:
        for x,y in [(0,3),(1,2),(2,1),(14,3),(15,4),(0,12),(1,13),(14,11),(15,10)]:
            items.dot(frame,x,y,color)
        items.box(frame,7,0,2,2,5 if frame==12 else color)
    rows=layout()
    header='/* Generated together with course.tmx by tools/make_assets.py. */\nstatic const char level_rows[18][81] = {\n'
    import json
    header+=''.join('    '+json.dumps(row)+',\n' for row in rows)+'};\n'
    codes={'.':0,'#':1,'-':2,'/':3,'\\':4,'*':5,'?':6}
    data=[]
    for y in range(32):
        for x in range(128): data.append(codes[rows[y][x]]+1 if y<18 and x<80 else 1)
    csv=',\n'.join(','.join(map(str,data[y*128:(y+1)*128])) for y in range(32))
    tmx=('<?xml version="1.0" encoding="UTF-8"?>\n<map version="1.10" tiledversion="1.11.2" orientation="orthogonal" renderorder="right-down" width="128" height="32" tilewidth="16" tileheight="16" infinite="0" nextlayerid="2" nextobjectid="1">\n'
         '<tileset firstgid="1" source="course.tsx"/>\n<layer id="1" name="Ground" width="128" height="32"><data encoding="csv">\n'+csv+'\n</data></layer>\n</map>\n')
    tsx=('<?xml version="1.0" encoding="UTF-8"?>\n<tileset version="1.10" tiledversion="1.11.2" name="course" tilewidth="16" tileheight="16" tilecount="7" columns="7">\n<image source="course.png" width="112" height="16"/>\n</tileset>\n')
    return {'res/course.png':terrain.image(PALETTES[0]),'res/hero.png':hero.image(PALETTES[1]),
            'res/enemies.png':enemies.image(PALETTES[2]),'res/items.png':items.image(PALETTES[2]),
            'res/course.tmx':tmx.encode(),'res/course.tsx':tsx.encode(),'inc/layout.h':header.encode()}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    files=assets()
    if any((args.output/name).exists() for name in files): parser.error('Source files already exist; choose a new output folder')
    for name,data in files.items():
        path=args.output/name
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(data)
    print('Created native-resolution art, Tiled map and collision layout')


if __name__=='__main__': main()
