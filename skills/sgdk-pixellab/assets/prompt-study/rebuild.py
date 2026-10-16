#!/usr/bin/env python3
"""Reproduce committed native prepared assets offline; --out requires a new directory."""
import argparse
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import art_pipeline as art
from png_io import read_png,indexed_png

ROOT=Path(__file__).resolve().parent


def prepared(name):
    plan=art.contract(json.loads((ROOT/'contracts'/f'{name}.json').read_text()))
    w,h,pixels=read_png(ROOT/'raw'/f'{name}.png')
    art.inspect(w,h,pixels,plan)
    if name=='title-illustration':
        # Exact correction of the generated pseudo-signature in a uniform corner.
        for y in range(h-8,h):
            for x in range(w-32,w):pixels[y*w+x]=pixels[y*w+w-1]
    indices=art.quantize(w,h,pixels,plan)
    return indexed_png(w,h,indices,plan['colors'],plan['kind']=='sprite')


def map_image():
    w,h,pixels=read_png(ROOT/'final/terrain-sixteen.png')
    metadata=json.loads((ROOT/'raw/terrain-sixteen-metadata.json').read_text())
    layout=json.loads((ROOT/'raw/map-native-layout.json').read_text())
    lookup={}
    for tile in metadata['tileset_data']['tiles']:
        key=tuple(tile['corners'][k] for k in layout['corner_order'])
        box=tile['bounding_box']
        if key in lookup or box['width']!=16 or box['height']!=16 or box['x']<0 or box['y']<0 or box['x']+16>w or box['y']+16>h:
            raise ValueError('Invalid or ambiguous terrain atlas bounds')
        lookup[key]=box
    width,height=layout['width'],layout['height']
    if len(layout['cells'])!=width*height:raise ValueError('Invalid map cell count')
    output=[]
    for y in range(height):
        for py in range(16):
            for x in range(width):
                key=tuple(layout['cells'][y*width+x])
                if key not in lookup:raise ValueError('Missing terrain corner configuration')
                box=lookup[key]
                output.extend(pixels[(box['y']+py)*w+box['x']:(box['y']+py)*w+box['x']+16])
    colors=art.palette(json.loads((ROOT/'contracts/terrain-sixteen.json').read_text())['palette'])
    indices=[colors.index(p[:3]) for p in output]
    return indexed_png(width*16,height*16,indices,colors)


def idle_sheet():
    frames=[read_png(ROOT/'final/robot-profile.png')]+[read_png(ROOT/'final'/f'robot-idle-{i}.png') for i in range(1,5)]
    colors=art.palette(json.loads((ROOT/'contracts/robot-profile.json').read_text())['palette'])
    indices=[]
    for y in range(32):
        for w,h,pixels in frames:
            if (w,h)!=(32,32):raise ValueError('Wrong idle frame dimensions')
            for pixel in pixels[y*32:(y+1)*32]:
                indices.append(0 if pixel[3]==0 else colors.index(pixel[:3],1))
    return indexed_png(160,32,indices,colors,True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path);args=parser.parse_args()
    files={p.stem+'.png':prepared(p.stem) for p in sorted((ROOT/'contracts').glob('*.json'))}
    files['map-native.png']=map_image()
    files['robot-idle-sheet.png']=idle_sheet()
    if args.out:
        args.out.mkdir(parents=True,exist_ok=False)
        for name,data in files.items():(args.out/name).write_bytes(data)
    else:
        for name,data in files.items():
            if data!=(ROOT/'final'/name).read_bytes():raise ValueError('Prepared output differs: '+name)
    print(f'{len(files)} native outputs reproduced without network or resizing')


if __name__=='__main__':main()
