#!/usr/bin/env python3
"""Prepare native art and the self-contained SGDK project in a new directory."""
from collections import Counter
import json
from pathlib import Path
import shutil
import sys
import zipfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parents[1]/'scripts'))
from png_io import read_png,indexed_png
from art_pipeline import contract,quantize,inspect
from fit_palette import fit
from background_banks import prepare as bank_background,validate_banks
from make_audio import assets as audio_assets
from glyphs import rows as glyph_rows
from shadow_highlight import prepare as light_background

UI=['000000','000024','242449','24496d','496d92','6d92b6','92b6db','dbb66d',
    'ffdb92','db9249','926d49','6d2449','24926d','49b6b6','b6dbdb','ffffff']
DIRECTIONS=['south','east','north','west']
GLYPHS="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,!?:;'\"-+()0123456789/%*=_$"

class PNGBytes:
    def __init__(self,data):self.data=data
    def read_bytes(self):return self.data


def hexes(colors):return [''.join(f'{c:02x}'for c in rgb)for rgb in colors]
def full_line(colors):return colors+[colors[-1]]*(16-len(colors))
def rgba_source(path):return read_png(path)


def character_frames(name):
    result=[]
    with zipfile.ZipFile(ROOT/'raw'/f'{name}.zip')as package:
        for direction in DIRECTIONS:
            paths=[f'Idle/rotations/{direction}.png']+[f'Idle/animations/Walk/{direction}/frame_{n:03}.png'for n in range(4)]
            for path in paths:
                frame=read_png(PNGBytes(package.read(path)))
                if frame[:2]!=(48,48):raise ValueError('Unexpected native character canvas: '+path)
                result.append(frame)
    return result


def sprite_plan(pixels,locked=None):
    hist=Counter(p[:3]for p in pixels if p[3]>=128)
    colors=[(0,0,0)]+full_line(fit(hist,15,locked if locked is not None else [(0,0,0),(255,255,255),(255,219,146)]))[:15]
    return hexes(colors)


def write_sprite(out,name,w,h,pixels,palette,report):
    p=contract(dict(width=w,height=h,kind='sprite',palette=palette,alpha_threshold=128))
    indices=quantize(w,h,pixels,p);(out/f'{name}.png').write_bytes(indexed_png(w,h,indices,p['colors'],True))
    report[name]=dict(native_size=[w,h],source=inspect(w,h,pixels,p),operation='RGB333 palette mapping; alpha threshold 128; no rescaling')


def prepare(destination):
    if destination.exists():raise ValueError('Destination exists; choose a new directory')
    shutil.copytree(ROOT/'src',destination/'src');shutil.copytree(ROOT/'inc',destination/'inc')
    shutil.copyfile(ROOT/'Makefile',destination/'Makefile');out=destination/'res';out.mkdir()
    report={};lines=[]
    for name in ['title','world','town','dungeon','forest','coast','intro-sky']:
        w,h,pixels=read_png(ROOT/'raw'/f'{name}.png')
        if any(p[3]!=255 for p in pixels):raise ValueError('Unexpected transparent background: '+name)
        if name=='intro-sky':
            # The generator invented black lettering in the quiet sky despite
            # the no-text brief. Replace only that empty-sky strip at native size.
            background=Counter(pixels[17*w+55:17*w+266]).most_common(1)[0][0]
            before=list(pixels)
            for y in range(18,30):
                for x in range(55,266):pixels[y*w+x]=background
            report['intro_sky_cleanup']=dict(region=[55,18,266,30],replacement=list(background),
                changed_pixels=sum(a!=b for a,b in zip(before,pixels)),reason='Remove invented title-like text in empty sky; source retained')

        banks=[]
        for slot,ys in enumerate([range(0,h//2),range(h//2,h)]):
            histogram=Counter(pixels[y*w+x][:3]for y in ys for x in range(w))
            colors=[(0,0,0)]+fit(histogram,15);colors=full_line(colors)
            banks.append(dict(slot=slot,palette=hexes(colors)))
        palettes=validate_banks(dict(banks=banks));colors,indices,metrics=bank_background(w,h,pixels,palettes)
        if name=='title':
            # Operators affect only the illustration between the title lettering
            # and menu. Normal-priority actors never share this mask's scanlines.
            indices,_,preview,normal=light_background(w,h,pixels,palettes,False)
            crop=pixels[56*w:120*w]
            lit,mask,lit_preview,lighting=light_background(w,64,crop,palettes,True)
            indices[56*w:120*w]=lit
            preview[56*w:120*w]=lit_preview
            display_colors=sorted(set(preview));lookup={c:i for i,c in enumerate(display_colors)}
            (out/'title-preview.png').write_bytes(indexed_png(w,h,[lookup[c]for c in preview],display_colors))
            control=[(0,0,0)]*16;control[14]=(255,255,255);control[15]=(255,0,0)
            for half in range(2):
                part=[mask[y*w+half*128+x]for y in range(64)for x in range(128)]
                (out/f'title-mask{half}.png').write_bytes(indexed_png(128,64,part,control,True))
                lines.append(f'SPRITE art_title_mask{half} "title-mask{half}.png" 16 8 NONE 0 NONE NONE')
            report['title_lighting']=dict(region=[0,56,256,120],lighting=lighting,
                max_hardware_sprites=16,max_scanline_sprites=8,max_scanline_pixels=256,
                max_mask_tiles=256,normal=normal,
                operation='Title-only shadow/highlight operators; high-priority background; unchanged palette lines; no HBlank CRAM writes')
        filename=name+'.png';(out/filename).write_bytes(indexed_png(w,h,indices,colors))
        report[name]=dict(native_size=[w,h],banks=banks,metrics=metrics)
        lines.append(f'IMAGE art_{name.replace("-","_")} "{filename}" NONE')
    # One palette spans all actors, all directions, all walk frames and portraits.
    characters={name:character_frames(name)for name in ['hero','guide','sage']}
    portraits={name:read_png(ROOT/'raw'/f'portrait-{name}.png')for name in characters}
    pixels=[p for frames in characters.values()for _,_,frame in frames for p in frame]
    pixels += [p for _,_,frame in portraits.values()for p in frame]
    actor_palette=sprite_plan(pixels);report['actor_palette']=actor_palette
    for name,frames in characters.items():
        for kind in ['idle','walk']:
            w=192;h=48 if kind=='idle'else 192;sheet=[(0,0,0,0)]*(w*h)
            for direction in range(4):
                for f in range(1 if kind=='idle'else 4):
                    _,_,frame=frames[direction*5+(0 if kind=='idle'else f+1)]
                    ox=direction*48 if kind=='idle'else f*48;oy=0 if kind=='idle'else direction*48
                    for y in range(48):sheet[(oy+y)*w+ox:(oy+y)*w+ox+48]=frame[y*48:(y+1)*48]
            identifier=f'{name}_{kind}';write_sprite(out,identifier,w,h,sheet,actor_palette,report)
            lines.append(f'SPRITE art_{identifier} "{identifier}.png" 6 6 NONE 0 NONE')
        w,h,px=portraits[name];write_sprite(out,'portrait_'+name,w,h,px,actor_palette,report)
        lines.append(f'IMAGE art_portrait_{name} "portrait_{name}.png" NONE')
    # Menu icons own PAL1. They are not visible simultaneously with world art.
    icons=sorted((ROOT/'raw').glob('icon-*.png'))+sorted((ROOT/'raw').glob('spell-*.png'))
    icon_frames={file:read_png(file)for file in icons}
    for file,(w,h,px)in list(icon_frames.items()):
        if file.stem=='icon-potion':
            # The requested green tonic came back blue. Preserve its silhouette
            # and glints, remapping only the saturated cyan/blue liquid ramp.
            corrected=[];changed=0
            for red,green,blue,alpha in px:
                if alpha and blue>70 and blue>red*1.3 and green>red*1.3:
                    value=(red,max(green,blue),min(blue//3,green//2),alpha);changed+=1
                else:value=(red,green,blue,alpha)
                corrected.append(value)
            icon_frames[file]=(w,h,corrected)
            report['tonic_cleanup']=dict(operation='Blue/cyan liquid ramp to emerald; silhouette and highlights preserved',changed_pixels=changed)
    icon_palette=sprite_plan([p for _,_,pixels in icon_frames.values()for p in pixels],
        [(0,0,0),(255,255,255),(0,182,73),(36,182,219),(255,182,73)])
    for file in icons:
        w,h,px=icon_frames[file];name=file.stem.replace('-','_');write_sprite(out,name,w,h,px,icon_palette,report)
        lines.append(f'IMAGE art_{name} "{name}.png" NONE')
    # The intro front layer is a tile plane cutout, so transparent index zero
    # reveals the sky behind it. It uses PAL2 while actors are absent.
    w,h,px=read_png(ROOT/'raw/intro-front.png');front_palette=sprite_plan(px)
    write_sprite(out,'intro_front',w,h,px,front_palette,report)
    lines.append('IMAGE art_intro_front "intro_front.png" NONE')
    w,h,px=read_png(ROOT/'raw/panel.png');p=contract(dict(width=w,height=h,kind='background',palette=UI))
    indices=quantize(w,h,px,p);(out/'panel.png').write_bytes(indexed_png(w,h,indices,p['colors']))
    lines.append('IMAGE art_panel "panel.png" NONE');report['ui_palette']=UI
    # Actual atlas is 8 columns by 10 rows, despite the tool schema saying 10x8.
    fw,fh,font=read_png(ROOT/'raw/font.png');columns=fw//8
    if fw%8 or fh%8 or (fw//8)*(fh//8)!=len(GLYPHS):raise ValueError('Unknown generated glyph atlas layout')
    # Native, hand-authored 5x7 letterforms replace the irregular generated font.
    for label,index in [('ivory',15),('gold',8)]:
        sheet=[1]*(768*8)
        for code in range(32,127):
            for y,row in enumerate(glyph_rows(chr(code))):
                for x in range(5):
                    if row & (1<<(4-x)):sheet[y*768+(code-32)*8+x+1]=index
        (out/f'font_{label}.png').write_bytes(indexed_png(768,8,sheet,p['colors']))
        lines.append(f'TILESET art_font_{label} "font_{label}.png" NONE NONE ROW')
    large=[0]*(416*16)
    for i,char in enumerate('ABCDEFGHIJKLMNOPQRSTUVWXYZ'):
        positions=[(x*2+dx+2,y*2+dy)for y,row in enumerate(glyph_rows(char))for x in range(5)
                   if row&(1<<(4-x))for dx in range(2)for dy in range(2)]
        for x,y in positions:
            if x<15 and y<15:large[(y+1)*416+i*16+x+1]=2
        for x,y in positions:large[y*416+i*16+x]=15 if y<8 else 8
    (out/'font_title.png').write_bytes(indexed_png(416,16,large,p['colors']))
    lines.append('TILESET art_font_title "font_title.png" NONE NONE ROW')
    for name,fill in [('map_pin',12),('map_pin_selected',8)]:
        pin=[0]*256
        for y in range(13):
            width=5-abs(5-y)if y<=10 else 1
            for x in range(8-width,9+width):
                if 0<=x<16:pin[y*16+x]=15 if abs(x-8)==width else fill
        pin[13*16+8]=15
        for y in range(3,7):
            for x in range(6,10):pin[y*16+x]=15
        (out/f'{name}.png').write_bytes(indexed_png(16,16,pin,p['colors'],True))
        lines.append(f'IMAGE art_{name} "{name}.png" NONE')
    circle=[0]*256
    for y in range(16):
        for x in range(16):
            distance=(x-8)**2+(y-8)**2
            if 17<=distance<=36:circle[y*16+x]=8 if distance<28 else 15
    (out/'map_location.png').write_bytes(indexed_png(16,16,circle,p['colors'],True))
    lines.append('IMAGE art_map_location "map_location.png" NONE')
    lines.append('SPRITE art_travel_pin "map_pin_selected.png" 2 2 NONE 0 NONE')
    report['authored_map_pins']='Native 16x16 circle markers and gold bobbing pin, shared UI palette; no generation or rescaling'
    report['font']=dict(source_size=[fw,fh],glyph_grid=[columns,fh//8],glyphs=GLYPHS,operation='Generated atlas retained as reference; production uses authored native 5x7 bitmap glyphs, opaque navy cells and integer 2x title')
    for name,data in audio_assets().items():(out/name).write_bytes(data)
    for name in ['title','travel','town','dungeon','forest','coast','menu']:lines.append(f'XGM2 music_{name} "{name}.vgm"')
    for name in ['move','accept','back','denied','talk','travel','page','name']:lines.append(f'WAV sfx_{name} "{name}.wav" XGM2')
    (out/'resources.res').write_text('\n'.join(lines)+'\n')
    (destination/'asset-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
