#!/usr/bin/env python3
"""Create Enemy Lab's original indexed pulse, projectile and waypoint sheets."""
import argparse
import io
import math
from pathlib import Path
import struct
import wave
import zlib

COLORS=[(0,0,0),(0,0,0),(36,36,36),(73,73,73),(109,109,109),(146,146,146),
        (182,182,182),(36,73,146),(73,146,219),(146,219,255),(219,255,255),
        (73,36,0),(182,109,36),(255,182,73),(255,219,146),(255,255,255)]


def indexed(width,height,pixels):
    def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+
            chunk(b'PLTE',bytes(c for rgb in COLORS for c in rgb))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))


def assets():
    status=[0]*(48*16)
    glyphs={
        '!': ["01110","01110","01110","01110","00100","00000","00100"],
        'z': ["00000","00000","11110","00100","01000","11110","00000"],
        'Z': ["11111","00001","00010","00100","01000","10000","11111"]
    }
    def glyph(frame,char,left,top):
        for y,row in enumerate(glyphs[char]):
            for x,value in enumerate(row):
                if value=='1': status[(top+y)*48+frame*16+left+x]=15
    glyph(0,'!',5,2)
    for frame in [1,2]:
        glyph(frame,'z',1,7-(frame-1)); glyph(frame,'Z',8,2+(frame-1))
    dots=[0]*64
    for y in range(2,6):
        for x in range(2,6): dots[y*8+x]=15 if x in (3,4) or y in (3,4) else 8
    shots=[0]*(24*8)
    for frame in range(3):
        for y in range(8):
            for x in range(8):
                distance=abs(y-3) if frame==0 else abs(x-y) if frame==1 else abs(x-3)
                if 1<=x<=6 and 1<=y<=6 and distance<2: shots[y*24+frame*8+x]=15 if not distance else 8
    pulse=[0]*(256*64)
    for frame,radius in enumerate([8,16,24,31]):
        for y in range(64):
            for x in range(64):
                distance=(x-32)**2+(y-32)**2
                if (radius-1)**2<=distance<=radius**2 and (frame<3 or (x+y)%3):
                    pulse[y*256+frame*64+x]=9 if frame<2 else 8
    hearts=[0]*(32*16); goal=[0]*(32*16)
    for frame in range(2):
        for y in range(16):
            for x in range(16):
                inside=((x-5)**2+(y-5)**2<13 or (x-10)**2+(y-5)**2<13 or
                        (5<=y<=12 and abs(2*x-15)<=24-2*y))
                if inside: hearts[y*32+frame*16+x]=12 if frame==0 else 10
                if 2<=x<=13 and 1<=y<=14:
                    goal[y*32+frame*16+x]=10 if x in (2,13) or y in (1,14) else 9
                if (6<=x<=9 and 4<=y<=11) or (x==5 and y in (7,8)) or (x==10 and y in (7,8)):
                    goal[y*32+frame*16+x]=13 if frame==0 else 15
    return {'marker.png':indexed(8,8,dots),'shot.png':indexed(24,8,shots),
            'pulse.png':indexed(256,64,pulse),'status.png':indexed(48,16,status),
            'tries.png':indexed(32,16,hearts),'exit.png':indexed(32,16,goal)}


def clear_jingle():
    """A short rising chime with a small ROM footprint."""
    rate=13300; samples=bytearray()
    for frequency in (523.25,659.25,783.99,1046.5):
        length=rate//10
        for i in range(length):
            envelope=min(i/40,1)*(1-i/length)**2
            tone=math.sin(2*math.pi*frequency*i/rate)+0.25*math.sin(4*math.pi*frequency*i/rate)
            samples.append(128+round(68*envelope*tone))
    output=io.BytesIO()
    with wave.open(output,'wb') as sound:
        sound.setparams((1,1,rate,0,'NONE','not compressed')); sound.writeframes(samples)
    return output.getvalue()


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path)
    args=parser.parse_args(); files={**assets(),'clear.wav':clear_jingle()}
    if any((args.output/name).exists() for name in files): parser.error('Choose a new output folder; assets already exist')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print(f'Created {len(files)-1} indexed sprite sheets and a clear jingle')


if __name__=='__main__': main()
