#!/usr/bin/env python3
"""Reproduce prepared images from committed sources and approved study plans offline."""
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parents[1]/'scripts'))
import art_pipeline as art
import background_banks as banks
from png_io import read_png,indexed_png


def rebuild(check=False):
    outputs={}
    for row in json.loads((ROOT/'assets.json').read_text()):
        name=row['id'];w,h,pixels=read_png(ROOT/'raw'/f'{name}.png')
        # Hex tiles were generated with transparent corners. This example
        # deliberately composites those corners onto a shared black backdrop.
        if name=='hex-atlas':pixels=[(0,0,0,255)if p[3]==0 else p for p in pixels]
        plan=art.contract(json.loads((ROOT/'plans'/f'{name}.json').read_text()))
        indices=art.quantize(w,h,pixels,plan)
        outputs[f'{name}.png']=indexed_png(w,h,indices,plan['colors'],row['role']=='sprite')
        if row.get('two_lines'):
            plan=banks.validate_banks(json.loads((ROOT/'plans'/f'{name}-banks.json').read_text()))
            colors,indices,_=banks.prepare(w,h,pixels,plan)
            outputs[row['two_lines']]=indexed_png(w,h,indices,colors)
    for name,data in outputs.items():
        target=ROOT/'final'/name
        if check:
            if target.read_bytes()!=data:raise ValueError('Export differs: '+name)
        else:target.write_bytes(data)
    return len(outputs)


if __name__=='__main__':print(f'{rebuild("--check"in sys.argv)} native exports reproduced')
