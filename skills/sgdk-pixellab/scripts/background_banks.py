#!/usr/bin/env python3
"""Prepare an indexed SGDK background with one planned palette line per 8x8 tile."""
import argparse
import json
from pathlib import Path
import sys
from art_pipeline import palette
from png_io import indexed_png,read_png


def validate_banks(document):
    if not isinstance(document,dict) or set(document)!={'banks'} or not isinstance(document['banks'],list) or not 1<=len(document['banks'])<=4:
        raise ValueError('Expected one to four planned banks')
    banks=[]
    for bank in document['banks']:
        if not isinstance(bank,dict) or set(bank)!={'slot','palette'} or type(bank['slot'])is not int or not 0<=bank['slot']<4:
            raise ValueError('Each bank needs slot 0..3 and a sixteen-entry palette')
        if any(b['slot']==bank['slot']for b in banks):raise ValueError('Palette slots must be unique')
        banks.append(dict(slot=bank['slot'],colors=palette(bank['palette'])))
    if len({b['colors'][0]for b in banks})!=1:
        raise ValueError('This opaque-background workflow requires a common index-zero backdrop color')
    return sorted(banks,key=lambda b:b['slot'])


def prepare(width,height,pixels,banks):
    if width%8 or height%8 or width<=0 or height<=0 or len(pixels)!=width*height:
        raise ValueError('Background dimensions must be positive multiples of eight')
    if any(p[3]!=255 for p in pixels):raise ValueError('Composite transparency deliberately before exporting an opaque background')
    if not banks:raise ValueError('No palette banks')
    colors=[banks[0]['colors'][0]]*64
    for bank in banks:colors[bank['slot']*16:bank['slot']*16+16]=bank['colors']
    cache={}
    for rgb in {p[:3]for p in pixels}:
        cache[rgb]=[]
        for bank in banks:
            distances=[sum((a-b)**2 for a,b in zip(rgb,c))for c in bank['colors']]
            at=min(range(16),key=lambda i:(distances[i],i))
            cache[rgb].append((at,distances[at]))
    indices=[0]*len(pixels);assignments=[];error=0
    for ty in range(0,height,8):
        for tx in range(0,width,8):
            positions=[(ty+y)*width+tx+x for y in range(8)for x in range(8)]
            costs=[sum(cache[pixels[i][:3]][b][1]for i in positions)for b in range(len(banks))]
            chosen=min(range(len(banks)),key=lambda b:(costs[b],banks[b]['slot']))
            slot=banks[chosen]['slot'];assignments.append(slot);error+=costs[chosen]
            for i in positions:indices[i]=slot*16+cache[pixels[i][:3]][chosen][0]
    return colors,indices,{'tile_palette_slots':assignments,'rgb_squared_error':error,
                          'used_slots':sorted(set(assignments)),
                          'backdrop':colors[banks[0]['slot']*16],
                          'note':'Every 8x8 tile uses one line. Configure its common index-zero backdrop; coordinate these slots with sprites/UI.'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path);parser.add_argument('plan',type=Path)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    try:
        if args.output.exists()or(args.report and args.report.exists()):raise ValueError('Output/report exists; choose new files')
        banks=validate_banks(json.loads(args.plan.read_text(encoding='utf-8')))
        w,h,pixels=read_png(args.source);colors,indices,report=prepare(w,h,pixels,banks)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        with args.output.open('xb')as file:file.write(indexed_png(w,h,indices,colors))
        if args.report:
            args.report.parent.mkdir(parents=True,exist_ok=True)
            with args.report.open('x',encoding='utf-8')as file:json.dump(report,file,indent=2);file.write('\n')
        print(f'Prepared {w//8}x{h//8} tiles using slots {report["used_slots"]}; no resizing or raster palette changes')
        return 0
    except (ValueError,OSError)as error:
        print(error,file=sys.stderr);return 1


if __name__=='__main__':sys.exit(main())
