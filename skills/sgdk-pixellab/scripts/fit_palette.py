#!/usr/bin/env python3
"""Propose a source-derived RGB333 palette for related assets; preserve artist locks."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import sys
from art_pipeline import LEVELS
from png_io import read_png


def snap(rgb):
    return tuple(min(LEVELS,key=lambda level:(abs(level-channel),level)) for channel in rgb)


def perceptual(rgb):
    # OKLab, D65 sRGB. Used only to choose a proposal, never as a hardware model.
    linear=[v/3294.6 if v<=10.31475 else ((v/255+0.055)/1.055)**2.4 for v in rgb]
    r,g,b=linear
    l=(0.4122214708*r+0.5363325363*g+0.0514459929*b)**(1/3)
    m=(0.2119034982*r+0.6806995451*g+0.1073969566*b)**(1/3)
    s=(0.0883024619*r+0.2817188376*g+0.6299787005*b)**(1/3)
    return (0.2104542553*l+0.7936177850*m-0.0040720468*s,
            1.9779984951*l-2.4285922050*m+0.4505937099*s,
            0.0259040371*l+0.7827717662*m-0.8086757660*s)


def distance(a,b):
    return sum((x-y)**2 for x,y in zip(a,b))


def fit(histogram,count,locked=()):
    if type(count) is not int or not 1<=count<=16:raise ValueError('Color budget must be 1..16')
    if any(len(rgb)!=3 or any(c not in LEVELS for c in rgb) for rgb in locked):
        raise ValueError('Locked colors must already be canonical RGB333 previews')
    chosen=list(dict.fromkeys(locked))
    if len(chosen)>count:raise ValueError('More locked colors than the color budget')
    canonical=Counter()
    for rgb,weight in histogram.items():
        if len(rgb)!=3 or any(type(c) is not int or not 0<=c<=255 for c in rgb) or weight<=0:
            raise ValueError('Invalid histogram entry')
        canonical[snap(rgb)]+=weight
    if not canonical:raise ValueError('No opaque pixels to fit')
    points={rgb:perceptual(rgb) for rgb in set(canonical)|set(chosen)}
    # Retain source light/dark endpoints before spending entries on common midtones.
    for endpoint in [min(canonical,key=lambda c:(points[c][0],c)),max(canonical,key=lambda c:(points[c][0],c))]:
        if len(chosen)<count and endpoint not in chosen:chosen.append(endpoint)
    # Weighted farthest selection keeps small saturated accents without averaging
    # them into grey. Source frequency matters, but only with a square-root weight.
    while len(chosen)<min(count,len(set(canonical)|set(chosen))):
        candidate=max((c for c in canonical if c not in chosen),
                      key=lambda c:(min(distance(points[c],points[p])for p in chosen)*math.sqrt(canonical[c]),c))
        chosen.append(candidate)
    return chosen


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sources',type=Path,nargs='+')
    parser.add_argument('--kind',choices=['sprite','background'],required=True)
    parser.add_argument('--lock',action='append',default=[],help='Protect a canonical six-digit RGB color; repeat as needed')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    try:
        if args.output.exists():raise ValueError('Output exists; choose a new file')
        histogram=Counter()
        for source in args.sources:
            _,_,pixels=read_png(source)
            if any(p[3] not in (0,255) for p in pixels):raise ValueError('Decide partial alpha before fitting a palette')
            if args.kind=='background' and any(p[3]!=255 for p in pixels):raise ValueError('Background source must be opaque')
            histogram.update(p[:3] for p in pixels if p[3])
        locked=[tuple(bytes.fromhex(value.lstrip('#'))) for value in args.lock]
        colors=fit(histogram,15 if args.kind=='sprite' else 16,locked)
        if args.kind=='sprite':colors=[(0,0,0)]+colors
        colors+= [colors[-1]]*(16-len(colors))
        document={'palette':[''.join(f'{c:02x}'for c in rgb)for rgb in colors],
                  'kind':args.kind,'sources':[str(p)for p in args.sources],
                  'note':'Source-derived proposal, not an approved scene plan. Review accents, material ramps and all four active lines.'}
        args.output.parent.mkdir(parents=True,exist_ok=True)
        with args.output.open('x',encoding='utf-8')as file:json.dump(document,file,indent=2);file.write('\n')
        print(f'Proposed one 16-entry line from {len(histogram)} source RGB colors; review before use')
        return 0
    except (ValueError,OSError) as error:
        print(error,file=sys.stderr);return 1


if __name__=='__main__':sys.exit(main())
