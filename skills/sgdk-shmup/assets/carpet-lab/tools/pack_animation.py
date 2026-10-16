#!/usr/bin/env python3
"""Pack five native animation frames into one indexed sprite sheet."""
import argparse
from pathlib import Path
from art_palette import ACTORS
from import_art import read_png,indexed_png

def pack(paths):
    frames=[read_png(p) for p in paths]; width,height=frames[0][:2]
    if any(f[:2]!=(width,height) for f in frames): raise ValueError('Frame sizes differ; never resize animation frames')
    colors=[tuple(bytes.fromhex(c)) for c in ACTORS]; lookup={}; result=[]
    for y in range(height):
        for _,_,pixels in frames:
            for r,g,b,a in pixels[y*width:(y+1)*width]:
                if a<128: result.append(0); continue
                rgb=(r,g,b)
                if rgb not in lookup: lookup[rgb]=min(range(1,16),key=lambda i:sum((rgb[j]-colors[i][j])**2 for j in range(3)))
                result.append(lookup[rgb])
    return indexed_png(width*len(frames),height,result,ACTORS)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('prefix',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    if args.output.exists(): parser.error('Output already exists')
    args.output.write_bytes(pack([Path(str(args.prefix)+'_'+str(i)+'.png') for i in range(5)]))
if __name__=='__main__': main()
