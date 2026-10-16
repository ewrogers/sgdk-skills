#!/usr/bin/env python3
"""Experimental same-palette background comparison using per-pixel S/H masks.

The preview lookup follows ares's Mega Drive color model, not a claim about
analog hardware calibration. Ordinary exports remain unchanged.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import sys
from background_banks import validate_banks
from png_io import read_png,indexed_png

# ares/md/vdp/color.cpp, commit 9408cb43d4948fc3ea6e152a307a34348df3fe04.
LOOKUP=((0,29,52,70,87,101,116,130),
        (0,52,87,116,144,172,206,255),
        (130,144,158,172,187,206,228,255))


def displayed(rgb,mode):
    return tuple(LOOKUP[mode][channel>>5]for channel in rgb)


def prepare(width,height,pixels,banks,lighting):
    if width<=0 or height<=0 or width%8 or height%8 or len(pixels)!=width*height:
        raise ValueError('Expected positive native dimensions on an 8x8 grid')
    if not banks or any(p[3]!=255 for p in pixels):raise ValueError('Expected planned banks and opaque source pixels')
    modes=(1,0,2)if lighting else(1,) # Prefer normal on exact ties.
    cache={}
    for rgb in {p[:3]for p in pixels}:
        choices=[]
        for bank in banks:
            options=[(sum((a-b)**2 for a,b in zip(rgb,displayed(color,mode))),rank,index,mode)
                     for rank,mode in enumerate(modes)for index,color in enumerate(bank['colors'])]
            error,_,index,mode=min(options)
            choices.append((error,index,mode))
        cache[rgb]=choices
    indices=[0]*len(pixels);mask=[0]*len(pixels);preview=[None]*len(pixels)
    counts=Counter();slots=[];error=0
    for ty in range(0,height,8):
        for tx in range(0,width,8):
            positions=[(ty+y)*width+tx+x for y in range(8)for x in range(8)]
            costs=[sum(cache[pixels[i][:3]][b][0]for i in positions)for b in range(len(banks))]
            at=min(range(len(banks)),key=lambda b:(costs[b],banks[b]['slot']))
            bank=banks[at];slots.append(bank['slot']);error+=costs[at]
            for i in positions:
                _,index,mode=cache[pixels[i][:3]][at]
                indices[i]=bank['slot']*16+index;mask[i]={0:15,1:0,2:14}[mode]
                preview[i]=displayed(bank['colors'][index],mode);counts[mode]+=1
    return indices,mask,preview,dict(rgb_squared_error=error,tile_palette_slots=slots,
        mode_pixels={name:counts[mode]for mode,name in enumerate(['shadow','normal','highlight'])},
        displayed_rgb_colors=len(set(preview)))


def write_comparison(source,plan,destination):
    if destination.exists():raise ValueError('Destination exists; choose a new directory')
    banks=validate_banks(json.loads(plan.read_text()));w,h,pixels=read_png(source)
    if(w,h)!=(256,128)or[bank['slot']for bank in banks]!=[0,1]:
        raise ValueError('This showcase requires a 256x128 source and background slots 0/1')
    outputs={};reports={};colors=[banks[0]['colors'][0]]*64
    for bank in banks:colors[bank['slot']*16:bank['slot']*16+16]=bank['colors']
    for name,lighting in [('normal',False),('lit',True)]:
        indices,mask,preview,report=prepare(w,h,pixels,banks,lighting)
        outputs[name+'.png']=indexed_png(w,h,indices,colors)
        # Exact decoded-RGB preview, independent of the hardware palette PNG.
        preview_colors=sorted(set(preview));lookup={c:i for i,c in enumerate(preview_colors)}
        if len(preview_colors)>256:raise ValueError('Preview needs more than indexed PNG capacity')
        outputs[name+'-preview.png']=indexed_png(w,h,[lookup[c]for c in preview],preview_colors)
        reports[name]=report
        if lighting:
            # ResComp logical sprites are limited to 16 hardware pieces. Split
            # the 256x128 mask into two 128x128 sprites, at most 16 pieces each.
            control=[(0,0,0)]*16;control[14]=(255,255,255);control[15]=(255,0,0)
            for half in range(2):
                part=[mask[y*w+half*128+x]for y in range(h)for x in range(128)]
                outputs[f'mask-{half}.png']=indexed_png(128,h,part,control,True)
    normal=reports['normal']['rgb_squared_error'];lit=reports['lit']['rgb_squared_error']
    reports.update(source=source.name,unchanged_base_palettes=True,
        error_reduction_percent=round(100*(normal-lit)/normal,2)if normal else 0,
        model='ares Mega Drive lookup; squared RGB error is not artistic approval',
        mask_limits=dict(max_hardware_pieces=32,max_scanline_pieces=8,max_scanline_pixels=256,
                         worst_case_mask_tiles=512))
    destination.mkdir(parents=True)
    for name,data in outputs.items():(destination/name).write_bytes(data)
    (destination/'report.json').write_text(json.dumps(reports,indent=2)+'\n')
    return reports


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source',type=Path)
    parser.add_argument('plan',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    try:
        report=write_comparison(args.source,args.plan,args.output)
        print(f'Prepared comparison; model RGB error reduction {report["error_reduction_percent"]}%')
    except(ValueError,OSError)as error:print(error,file=sys.stderr);return 1
    return 0


if __name__=='__main__':sys.exit(main())
