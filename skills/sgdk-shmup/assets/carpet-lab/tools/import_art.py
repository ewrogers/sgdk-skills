#!/usr/bin/env python3
"""Normalize native PNG artwork to the explicit VDP palettes, without resizing.

Builds use the committed indexed PNGs. This optional importer uses only Python's
standard library and takes a directory of the original PixelLab exports.
"""
import argparse
from pathlib import Path
import struct
import zlib
from art_palette import ACTORS, FX, FAR, NEAR

def read_png(path):
    data=path.read_bytes(); chunks={}; cursor=8
    if data[:8]!=b'\x89PNG\r\n\x1a\n': raise ValueError('Expected PNG')
    while cursor<len(data):
        length=struct.unpack_from('>I',data,cursor)[0]; kind=data[cursor+4:cursor+8]; payload=data[cursor+8:cursor+8+length]
        if zlib.crc32(kind+payload)!=struct.unpack_from('>I',data,cursor+8+length)[0]: raise ValueError('PNG checksum mismatch')
        chunks.setdefault(kind,bytearray()).extend(payload); cursor+=length+12
    w,h,depth,mode,_,_,interlace=struct.unpack('>IIBBBBB',chunks[b'IHDR'])
    if depth!=8 or interlace or mode not in (2,3,6): raise ValueError('Expected noninterlaced 8-bit RGB/RGBA/indexed PNG')
    bpp={2:3,3:1,6:4}[mode]; raw=zlib.decompress(chunks[b'IDAT']); stride=w*bpp
    prior=bytearray(stride); pixels=[]; cursor=0
    for _ in range(h):
        filter_type=raw[cursor]; cursor+=1; row=bytearray(raw[cursor:cursor+stride]); cursor+=stride
        if len(row)!=stride or filter_type>4: raise ValueError('Malformed PNG row')
        for x in range(stride):
            a=row[x-bpp] if x>=bpp else 0; b=prior[x]; c=prior[x-bpp] if x>=bpp else 0
            estimate=a+b-c; da,db,dc=abs(estimate-a),abs(estimate-b),abs(estimate-c)
            prediction=0 if filter_type==0 else a if filter_type==1 else b if filter_type==2 else (a+b)//2 if filter_type==3 else a if da<=db and da<=dc else b if db<=dc else c
            row[x]=(row[x]+prediction)&255
        for x in range(w):
            at=x*bpp
            if mode==3:
                n=row[at]; rgb=chunks[b'PLTE'][n*3:n*3+3]; alpha=chunks.get(b'tRNS',b'')
                pixels.append((*rgb,alpha[n] if n<len(alpha) else 255))
            else: pixels.append((*row[at:at+3],row[at+3] if mode==6 else 255))
        prior=row
    return w,h,pixels

def indexed_png(w,h,indices,colors):
    def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+bytes(indices[y*w:(y+1)*w]) for y in range(h))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,3,0,0,0))+chunk(b'PLTE',b''.join(bytes.fromhex(c) for c in colors))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')

def normalize(source,target,colors,transparent):
    w,h,pixels=read_png(source); palette=[tuple(bytes.fromhex(c)) for c in colors]; lookup={}; indices=[]
    for pixel in pixels:
        if transparent and pixel[3]<128: indices.append(0); continue
        rgb=pixel[:3]
        if rgb not in lookup:
            lookup[rgb]=min(range(1 if transparent else 0,16),key=lambda n:sum((rgb[j]-palette[n][j])**2 for j in range(3)))
        indices.append(lookup[rgb])
    target.write_bytes(indexed_png(w,h,indices,colors))
    return w,h,len(set(indices))

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('source',type=Path); parser.add_argument('output',type=Path)
    args=parser.parse_args(); plans={}
    for i in range(3): plans['far'+str(i+1)]=(FAR[i],False); plans['near'+str(i+1)]=(NEAR[i],True)
    plans['title']=(FAR[0],False)
    for name in ['hero','boss_scarab','boss_serpent','boss_lantern','enemy_teapot','enemy_kite','enemy_beetle']: plans[name]=(ACTORS,True)
    for name in ['spell_sun','spell_moon','spell_storm']: plans[name]=(FX,True)
    if any((args.output/(name+'.png')).exists() for name in plans): parser.error('Choose a new output folder; imported art already exists')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,(colors,transparent) in plans.items():
        result=normalize(args.source/(name+'.png'),args.output/(name+'.png'),colors,transparent)
        print(name,*result)
if __name__=='__main__': main()
