#!/usr/bin/env python3
"""Inspect native PNGs and prepare explicit, indexed SGDK palette exports; never resize."""
import argparse
import json
from pathlib import Path
import re
import sys
import struct
import zlib
from png_io import indexed_png, read_png

LEVELS = (0,36,73,109,146,182,219,255)
BAYER = {'none': ((0,),), 'bayer2': ((0,2),(3,1)),
         'bayer4': ((0,8,2,10),(12,4,14,6),(3,11,1,9),(15,7,13,5))}


def palette(values):
    if not isinstance(values,list) or len(values) != 16: raise ValueError('Plan exactly 16 palette entries, including index 0')
    if any(not isinstance(v,str) or not re.fullmatch(r'#?[0-9a-fA-F]{6}',v) for v in values):
        raise ValueError('Palette entries must be six-digit RGB hex')
    result = [tuple(bytes.fromhex(v.lstrip('#'))) for v in values]
    if any(c not in LEVELS for rgb in result for c in rgb):
        raise ValueError('Palette must use canonical VDP RGB levels: 0,36,73,109,146,182,219,255')
    return result


def contract(document):
    allowed = {'width','height','kind','palette','frame_width','frame_height','alpha_threshold','dither'}
    if not isinstance(document,dict) or set(document)-allowed: raise ValueError('Unknown asset-contract field')
    out = dict(document)
    for name in ['width','height']:
        if type(out.get(name)) is not int or not 1 <= out[name] <= 2048: raise ValueError(f'{name}: expected 1..2048')
    for name, fallback in [('frame_width','width'),('frame_height','height')]:
        out.setdefault(name,out[fallback])
        if type(out[name]) is not int or out[name] < 1 or out[fallback] % out[name]:
            raise ValueError(f'{name}: must divide the image dimensions')
        if out[name] % 8: raise ValueError(f'{name}: SGDK export needs 8-pixel alignment; explicitly pad native art first, never resize')
    if out.get('kind') not in ['sprite','background']: raise ValueError('kind: sprite or background')
    out['colors'] = palette(out['palette'])
    out.setdefault('dither','none')
    if out['dither'] not in BAYER: raise ValueError('dither: none, bayer2 or bayer4')
    if 'alpha_threshold' in out and (type(out['alpha_threshold']) is not int or not 1 <= out['alpha_threshold'] <= 255):
        raise ValueError('alpha_threshold: expected 1..255')
    return out


def inspect(width,height,pixels,plan):
    if (width,height) != (plan['width'],plan['height']):
        raise ValueError(f'Wrong native dimensions: {width}x{height}; expected {plan["width"]}x{plan["height"]}. Do not rescale')
    threshold = plan.get('alpha_threshold',128)
    opaque = [(x%width,x//width,p) for x,p in enumerate(pixels) if p[3] >= threshold]
    allowed = plan['colors'][1:] if plan['kind'] == 'sprite' else plan['colors']
    mask = {(x,y) for x,y,_ in opaque}
    bounds = [min(x for x,y in mask),min(y for x,y in mask),max(x for x,y in mask),max(y for x,y in mask)] if mask else None
    isolated = [[x,y] for x,y in sorted(mask) if not any((x+dx,y+dy) in mask for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)])]
    fw,fh = plan['frame_width'],plan['frame_height']
    frames=[]
    for fy in range(0,height,fh):
        for fx in range(0,width,fw):
            points=[(x-fx,y-fy) for y in range(fy,fy+fh) for x in range(fx,fx+fw) if pixels[y*width+x][3] >= threshold]
            frames.append({'bounds':[min(x for x,y in points),min(y for x,y in points),max(x for x,y in points),max(y for x,y in points)] if points else None,
                           'edge_pixels':sum(x in (0,fw-1) or y in (0,fh-1) for x,y in points)})
    tiles=set()
    for ty in range(0,height,8):
        for tx in range(0,width,8):
            tiles.add(tuple(pixels[(ty+y)*width+tx+x] for y in range(8) for x in range(8)))
    return {'palette_vdp_words':[f'0x{((r>>5)<<1)|((g>>5)<<5)|((b>>5)<<9):03X}' for r,g,b in plan['colors']],
            'width':width,'height':height,'opaque_pixels':len(opaque),'opaque_colors':len({p[:3] for _,_,p in opaque}),
            'off_palette_pixels':sum(p[:3] not in allowed for _,_,p in opaque),
            'partial_alpha_pixels':sum(p[3] not in (0,255) for p in pixels),
            'transparent_pixels':sum(p[3] == 0 for p in pixels),'bounds':bounds,
            'isolated_pixels':isolated,'frames':frames,'unique_8x8_patterns':len(tiles),
            'seam_mismatched_rows':sum(pixels[y*width] != pixels[y*width+width-1] for y in range(height)),
            'note':'Seam, isolated-pixel and edge counts are review hints, not visual correctness or hardware sprite counts'}


def quantize(width,height,pixels,plan):
    if any(p[3] not in (0,255) for p in pixels) and 'alpha_threshold' not in plan:
        raise ValueError('Partial alpha needs an explicit alpha_threshold decision')
    if plan['kind'] == 'background' and any(p[3] != 255 for p in pixels):
        raise ValueError('Opaque background requested but source has transparency; composite deliberately first')
    colors = plan['colors']
    start = 1 if plan['kind'] == 'sprite' else 0
    matrix = BAYER[plan['dither']]; n = len(matrix)
    indices=[]
    cache={}
    for index,p in enumerate(pixels):
        if plan['kind'] == 'sprite' and p[3] < plan.get('alpha_threshold',128): indices.append(0); continue
        rgb=p[:3]
        if rgb not in cache:
            nearest=sorted(range(start,16),key=lambda i:(sum((rgb[c]-colors[i][c])**2 for c in range(3)),i))[:2]
            a,b=nearest
            delta=[colors[b][c]-colors[a][c] for c in range(3)]
            norm=sum(d*d for d in delta)
            weight=max(0,min(1,sum((rgb[c]-colors[a][c])*delta[c] for c in range(3))/norm)) if norm else 0
            cache[rgb]=(a,b,weight)
        a,b,weight=cache[rgb]
        x=(index%width)%plan['frame_width']; y=(index//width)%plan['frame_height']
        indices.append(b if n > 1 and weight > (matrix[y%n][x%n]+0.5)/(n*n) else a)
    return indices


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation',choices=['inspect','validate','prepare'])
    parser.add_argument('source',type=Path); parser.add_argument('contract',type=Path)
    parser.add_argument('--output',type=Path); parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    try:
        if args.report and args.report.exists(): raise ValueError('Report already exists; choose a new file')
        plan=contract(json.loads(args.contract.read_text(encoding='utf-8')))
        w,h,pixels=read_png(args.source); report={'source':inspect(w,h,pixels,plan)}
        if args.operation == 'validate':
            if report['source']['off_palette_pixels']: raise ValueError(f"Off-palette pixels: {report['source']['off_palette_pixels']}")
            if report['source']['partial_alpha_pixels']: raise ValueError('Partial alpha is not a VDP transparency mode')
            if plan['kind'] == 'background' and report['source']['transparent_pixels']: raise ValueError('Background source is not opaque')
        if args.operation == 'prepare':
            if not args.output: raise ValueError('prepare requires --output in a new location')
            if args.output.exists(): raise ValueError('Output already exists; choose a new file')
            indices=quantize(w,h,pixels,plan)
            report['changed_opaque_rgb_pixels']=sum(p[3] >= plan.get('alpha_threshold',128) and p[:3] != plan['colors'][i] for p,i in zip(pixels,indices))
            output=indexed_png(w,h,indices,plan['colors'],plan['kind'] == 'sprite')
            prepared=[(*plan['colors'][i],0 if plan['kind'] == 'sprite' and i == 0 else 255) for i in indices]
            report['prepared']=inspect(w,h,prepared,plan)
            args.output.parent.mkdir(parents=True,exist_ok=True)
            with args.output.open('xb') as file: file.write(output)
        if args.report:
            if args.report.exists(): raise ValueError('Report already exists; choose a new file')
            args.report.parent.mkdir(parents=True,exist_ok=True)
            with args.report.open('x',encoding='utf-8') as file: json.dump(report,file,indent=2); file.write('\n')
        print(json.dumps(report,indent=2))
        return 0
    except (OSError,ValueError,KeyError,IndexError,struct.error,zlib.error) as error:
        print(str(error),file=sys.stderr); return 1


if __name__ == '__main__': sys.exit(main())
