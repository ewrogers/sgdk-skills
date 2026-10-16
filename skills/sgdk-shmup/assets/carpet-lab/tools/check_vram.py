#!/usr/bin/env python3
"""Check all Carpet Lab scenes against the linked ROM's actual SGDK resources."""
import argparse
from pathlib import Path
import re
import struct

COMMON=('hero','teapot','kite','beetle','sun','moon','storm','ribbon','effects')
BOSSES=('scarab','serpent','lantern')

def analyze(rom,symbols,cache_tiles,banner_line_tiles):
    def word(at): return struct.unpack_from('>H',rom,at)[0]
    def pointer(at): return struct.unpack_from('>I',rom,at)[0]
    def tiles(name): return word(symbols[name]+2)
    def image(name): return word(pointer(symbols[name]+4)+2)
    def sprite(name):
        address=symbols[name+'_image']; unique=set()
        for a in range(word(address+8)):
            animation=pointer(pointer(address+10)+a*4)
            for f in range(rom[animation]):
                frame=pointer(pointer(animation+2)+f*4)
                unique.add(pointer(frame+2))
        return sum(word(t+2) for t in unique)
    # The renderer checks the H40 plane tables at C000/D000/E000. Patterns
    # end at C000; SGDK's 96 font tiles and the sprite cache sit below it.
    font_start=0xC000//32-tiles('gradient_font')
    scenery_limit=font_start-cache_tiles
    fixed=16+1+tiles('hud_tiles')+tiles('health_tiles')+tiles('label_font')+1
    common=sum(sprite(n) for n in COMMON)
    rows=[]
    for stage,boss in enumerate(BOSSES,1):
        scenery_end=fixed+image('far_'+str(stage))+image('near_'+str(stage))
        actors=common+sprite(boss)+banner_line_tiles*2
        rows.append((f'Stage {stage}',scenery_end,actors))
    for name,art in [('Title','title_image'),('Credits','far_3')]:
        rows.append((name,fixed+image(art)+tiles('title_font'),common+sprite('button')))
    for name,scenery_end,actors in rows:
        if scenery_end>scenery_limit: raise ValueError(f'{name}: scenery uses {scenery_end} tiles below a {scenery_limit}-tile boundary')
        if actors>cache_tiles: raise ValueError(f'{name}: actors and banners need {actors} tiles; cache holds {cache_tiles}')
    return scenery_limit,rows

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project',type=Path,nargs='?',default=Path(__file__).resolve().parents[1])
    args=parser.parse_args();project=args.project
    try:
        definitions=(project/'inc/view.h').read_text()
        cache=int(re.search(r'^#define FRAME_CACHE_TILES (\d+)$',definitions,re.M)[1])
        banner=int(re.search(r'^#define BANNER_LINE_TILES (\d+)$',definitions,re.M)[1])
        symbols={}
        for line in (project/'out/symbol.txt').read_text().splitlines():
            fields=line.split()
            if len(fields)>=3:
                try: symbols[fields[2]]=int(fields[0],16)
                except ValueError: pass
        limit,rows=analyze((project/'out/rom.bin').read_bytes(),symbols,cache,banner)
    except (OSError,ValueError,KeyError,struct.error,TypeError) as error: parser.exit(1,f'VRAM check failed: {error}\n')
    for name,end,actors in rows: print(f'{name}: scenery {end}/{limit} tiles; actor cache {actors}/{cache} tiles')
if __name__=='__main__':main()
