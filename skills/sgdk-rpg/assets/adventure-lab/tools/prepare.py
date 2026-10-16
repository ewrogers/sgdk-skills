#!/usr/bin/env python3
"""Offline native art preparation; no service calls and no resizing."""
from collections import Counter
import io,json,shutil,zipfile
from pathlib import Path
from png_io import read_png,indexed_png
from art_pipeline import contract,quantize
from fit_palette import fit
from background_banks import prepare as bank_background,validate_banks
from adventure_glyphs import rows
from adventure_audio import assets as audio_assets
ROOT=Path(__file__).resolve().parents[1]
UI=['000000','000024','242449','24496d','496d92','249292','92dbdb','ffdb92',
    'db9249','db4949','922449','249249','92db92','6d496d','b692db','ffffff']
DIRECTIONS=['south','east','north','west']
class PNGBytes:
    def __init__(self,data):self.data=data
    def read_bytes(self):return self.data

def full_line(colors):return colors+[colors[-1]]*(16-len(colors))
def hexes(colors):return [''.join(f'{n:02x}'for n in c)for c in colors]

def field_frames():
    frames=[]
    with zipfile.ZipFile(ROOT/'raw/field.zip')as package:
        for direction in DIRECTIONS:
            paths=[f'Idle/rotations/{direction}.png']+[f'Idle/animations/Walk/{direction}/frame_{f:03}.png'for f in range(4)]
            for path in paths:
                w,h,pixels=read_png(PNGBytes(package.read(path)))
                if(w,h)!=(36,36):raise ValueError('Unexpected native field canvas: '+path)
                if direction=='south':
                    # Keep the generated head. Re-author the jacket, separated
                    # hands and legs at native size; animated coat tails had
                    # merged into an implausibly long hand on the down view.
                    for y in range(16,36):pixels[y*36:(y+1)*36]=[(0,0,0,0)]*36
                    colors={'C':(236,224,184,255),'S':(35,49,65,255),'K':(24,24,48,255),
                            'I':(236,160,128,255),'G':(46,94,100,255),'P':(45,46,65,255)}
                    body=['.CCSSSSCC.','CCCSSSSCCC','ICCSSSSCCI','ICCSSSSCCI',
                          'KCCSSSSCCK','.CCGGGGCC.','..CPPPPC..']
                    for dy,row in enumerate(body):
                        for dx,char in enumerate(row):
                            if char!='.':pixels[(16+dy)*36+13+dx]=colors[char]
                    phase=paths.index(path)-1
                    for leg,x in enumerate([15,19]):
                        length=6 if phase<0 else 6+([1,0,-1,0][phase]if leg==0 else[-1,0,1,0][phase])
                        for dy in range(length):
                            for dx in range(2):pixels[(23+dy)*36+x+dx]=colors['K'if dy>=length-2 else'P']
                padded=[(0,0,0,0)]*1600
                for y in range(36):padded[(y+4)*40+2:(y+4)*40+38]=pixels[y*36:(y+1)*36]
                frames.append(padded)
    return frames

def prepare(destination):
    if destination.exists():raise ValueError('Choose a new output directory')
    shutil.copytree(ROOT/'src',destination/'src');shutil.copytree(ROOT/'inc',destination/'inc')
    shutil.copyfile(ROOT/'Makefile',destination/'Makefile');out=destination/'res';out.mkdir()
    lines=[];report={}
    for name in ['title','harbor','battle']:
        w,h,pixels=read_png(ROOT/'raw'/f'{name}.png')
        if any(p[3]!=255 for p in pixels):raise ValueError('Background must be opaque: '+name)
        if name=='battle':
            # The returned scene included tiny people despite the no-character
            # brief. Replace only its bottom 24 rows with authored stone floor.
            for y in range(104,128):
                for x in range(w):
                    edge=y%8==0 or (x+(16 if (y//8)%2 else 0))%32==0
                    pixels[y*w+x]=(36,49,61,255)if edge else(24,37,49,255)
            report['battle_cleanup']='Native 320x24 authored stone floor removes invented figures; source retained'
        banks=[]
        for slot,ys in enumerate([range(h//2),range(h//2,h)]):
            hist=Counter(pixels[y*w+x][:3]for y in ys for x in range(w))
            banks.append(dict(slot=slot,palette=hexes(full_line([(0,0,0)]+fit(hist,15)))))
        colors,indices,metrics=bank_background(w,h,pixels,validate_banks(dict(banks=banks)))
        (out/f'{name}.png').write_bytes(indexed_png(w,h,indices,colors))
        lines.append(f'IMAGE art_{name} "{name}.png" NONE');report[name]=dict(size=[w,h],banks=banks,metrics=metrics)
    sources={name:read_png(ROOT/'raw'/f'{name}.png')for name in ['mara','ivo','neri','crab','moth','warden']}
    fields=field_frames()
    hist=Counter(p[:3]for _,_,px in sources.values()for p in px if p[3]>=128)
    hist.update(p[:3]for frame in fields for p in frame if p[3]>=128)
    locked=[(0,0,0),(255,255,255),(36,36,73),(255,219,146),(146,73,36)]
    sprite_palette=hexes([(0,0,0)]+full_line(fit(hist,15,locked))[:15]);report['sprite_palette']=sprite_palette
    for name,(w,h,px)in sources.items():
        expected=(32,48)if name in ['mara','ivo','neri']else(64,64)if name=='warden'else(32,32)
        if(w,h)!=expected:raise ValueError('Unexpected native dimensions: '+name)
        p=contract(dict(width=w,height=h,kind='sprite',palette=sprite_palette,alpha_threshold=128))
        (out/f'{name}.png').write_bytes(indexed_png(w,h,quantize(w,h,px,p),p['colors'],True))
        lines.append(f'SPRITE art_{name} "{name}.png" {w//8} {h//8} NONE 0 NONE NONE')
    for kind in ['idle','walk']:
        w=160;h=40 if kind=='idle'else 160;px=[(0,0,0,0)]*(w*h)
        for d in range(4):
            for f in range(1 if kind=='idle'else 4):
                frame=fields[d*5+(0 if kind=='idle'else f+1)]
                ox=d*40 if kind=='idle'else f*40;oy=0 if kind=='idle'else d*40
                for y in range(40):px[(oy+y)*w+ox:(oy+y)*w+ox+40]=frame[y*40:(y+1)*40]
        p=contract(dict(width=w,height=h,kind='sprite',palette=sprite_palette,alpha_threshold=128))
        (out/f'field_{kind}.png').write_bytes(indexed_png(w,h,quantize(w,h,px,p),p['colors'],True))
        lines.append(f'SPRITE art_field_{kind} "field_{kind}.png" 5 5 NONE 0 NONE NONE')
    report['field_cleanup']='South-facing lower body re-authored below row 16 to separate short jacket, hands and walking legs; generated heads and other directions retained'
    report['field_padding']='All 36x36 source frames padded to 40x40 at +2,+4; no rescaling'
    colors=[tuple(bytes.fromhex(c))for c in UI]
    for label,ink in [('ivory',15),('gold',7)]:
        sheet=[1]*(768*8)
        for code in range(32,127):
            for y,row in enumerate(rows(chr(code))):
                for x in range(5):
                    if row&(1<<(4-x)):sheet[y*768+(code-32)*8+x+1]=ink
        (out/f'font_{label}.png').write_bytes(indexed_png(768,8,sheet,colors))
        lines.append(f'TILESET art_font_{label} "font_{label}.png" NONE NONE ROW')
    # Original one-tile panel corners/edges and spell/cursor symbols.
    panel=[1]*576
    for y in range(24):
        for x in range(24):
            if x in(0,23)or y in(0,23):panel[y*24+x]=3
            elif x in(1,22)or y in(1,22):panel[y*24+x]=7 if y<12 else 8
            if (x in(2,3,20,21))and(y in(2,3,20,21)):panel[y*24+x]=6
    (out/'panel.png').write_bytes(indexed_png(24,24,panel,colors));lines.append('IMAGE art_panel "panel.png" NONE')
    effect=[0]*(32*32*4)
    for f in range(4):
        for y in range(32):
            for x in range(32):
                dx=x-16;dy=y-16;radius=4+f*3;dist=dx*dx+dy*dy
                if abs(dist-radius*radius)<radius*2 or(abs(dx)<2 and abs(dy)<radius):
                    effect[y*128+f*32+x]=15 if (x+y+f)%3==0 else 7 if f%2 else 6
    (out/'effect.png').write_bytes(indexed_png(128,32,effect,colors,True));lines.append('SPRITE art_effect "effect.png" 4 4 NONE 0 NONE NONE')
    arrow=[0]*256
    for y in range(3,8):
        for x in range(6,10):arrow[y*16+x]=15 if x in(6,9)else 7
    for y in range(8,14):
        half=13-y
        for x in range(8-half,9+half):arrow[y*16+x]=15 if x in(8-half,8+half)else 7
    (out/'target.png').write_bytes(indexed_png(16,16,arrow,colors,True));lines.append('SPRITE art_target "target.png" 2 2 NONE 0 NONE NONE')
    (out/'ui.png').write_bytes(indexed_png(16,1,list(range(16)),colors));lines.append('PALETTE ui_palette "ui.png"')
    for name,data in audio_assets().items():(out/name).write_bytes(data)
    for name in ['title','harbor','battle','guardian','victory','defeat','ending']:lines.append(f'XGM2 music_{name} "{name}.vgm"')
    for name in ['move','accept','back','denied','hit','spell','heal','win','lose','save']:lines.append(f'WAV sfx_{name} "{name}.wav" XGM2')
    (out/'resources.res').write_text('\n'.join(lines)+'\n');report['ui_palette']=UI
    (destination/'asset-report.json').write_text(json.dumps(report,indent=2)+'\n');return report
