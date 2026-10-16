#!/usr/bin/env python3
"""Create the lab's blocky gradient font and native UI/effect sprites."""
import argparse
from pathlib import Path
from import_art import indexed_png
from art_palette import FX

GLYPHS={
 'A':'0E11111F111111','B':'1E11111E11111E','C':'0F10101010100F',
 'D':'1E11111111111E','E':'1F10101E10101F','F':'1F10101E101010',
 'G':'0F10101711110F','H':'1111111F111111','I':'1F04040404041F',
 'J':'0702020212120C','K':'11121418141211','L':'1010101010101F',
 'M':'111B1515111111','N':'11191915131311','O':'0E11111111110E',
 'P':'1E11111E101010','Q':'0E11111115120D','R':'1E11111E141211',
 'S':'0F10100E01011E','T':'1F040404040404','U':'1111111111110E',
 'V':'11111111110A04','W':'11111115151B11','X':'11110A040A1111',
 'Y':'11110A04040404','Z':'1F01020408101F',
 '0':'0E11131519110E','1':'040C040404040E','2':'1E01010E10101F',
 '3':'1E01010E01011E','4':'1212121F020202','5':'1F10101E01011E',
 '6':'0F10101E11110E','7':'1F010204080808','8':'0E11110E11110E',
 '9':'0E11110F01011E',' ':'00000000000000','!':'04040404040004',
 '?':'0E110204040004','.':'00000000000004',',':'00000000000408',
 ':':'00040000040000',';':'00040000040800','-':'0000001F000000',
 '+':'0004041F040400','/':'01010204081010','%':'191A0204080B13',
 '=':'00001F001F0000','>':'10080402040810','<':'01020408040201',
 '(':'02040808080402',')':'08040202020408','[':'0E08080808080E',
 ']':'0E02020202020E','_':'0000000000001F',"'":'04040800000000',
 '*':'00150E1F0E1500','#':'0A1F0A0A1F0A00',
}

def font_pixels():
    width=96*8; data=[1]*(width*8)
    for code in range(32,128):
        rows=GLYPHS.get(chr(code).upper(),GLYPHS['?'])
        for y in range(7):
            row=int(rows[y*2:y*2+2],16)
            for x in range(6):
                if row&(16>>(x*5//6)): data[(y+1)*width+(code-32)*8+x+1]=2
        for y in range(7):
            row=int(rows[y*2:y*2+2],16)
            for x in range(6):
                if row&(16>>(x*5//6)): data[y*width+(code-32)*8+x]=15 if y<2 else 7 if y<4 else 6 if y<6 else 5
    return data

def assets():
    font=font_pixels(); files={'font.png':indexed_png(768,8,font,FX)}
    alphabet=' ACDEFHILMNOPRSTUVWY'
    width=len(alphabet)*16; data=[0]*(width*16)
    for n,c in enumerate(alphabet):
        for y in range(16):
            for x in range(16): data[y*width+n*16+x]=font[(y//2)*768+(ord(c)-32)*8+x//2]
    files['big_font.png']=indexed_png(width,16,data,FX)
    label_alphabet=' ABDEFGIKLMNOPRSTVWY'
    lw=len(label_alphabet)*8; labels=[1]*(lw*32)
    for selected in range(2):
        background=2 if selected else 1
        for y in range(16):
            for x in range(lw): labels[(selected*16+y)*lw+x]=background
        for n,c in enumerate(label_alphabet):
            for y in range(8):
                for x in range(8):
                    pixel=font[y*768+(ord(c)-32)*8+x]
                    labels[(selected*16+y+4)*lw+n*8+x]=background if pixel==1 else pixel
    files['label_font.png']=indexed_png(lw,32,labels,FX)
    # The effect atlas has four bursts, four spinning gems, three familiars,
    # a shield rim and three differently shaped hostile projectiles.
    count=15; width=count*16; data=[0]*(width*16)
    def dot(f,x,y,c):
        if 0<=x<16 and 0<=y<16: data[y*width+f*16+x]=c
    for f in range(4):
        for y in range(16):
            for x in range(16):
                d=abs(x-7)+abs(y-7)
                if f*2<=d<=f*2+3: dot(f,x,y,15 if f<2 else 6 if f==2 else 5)
                half=(5,4,2,4)[f]
                # Every spin frame has a closed tapered perimeter, including
                # the narrow side-on frame; the facets never end in a cut edge.
                limit=max(0,half-abs(y-7)*half//7)
                if abs(y-7)<=6 and abs(x-7)<=limit:
                    edge=abs(x-7)==limit or abs(y-7)==6
                    dot(4+f,x,y,12 if edge else 15 if x<7 else 14)
    for f in range(3):
        for y in range(16):
            for x in range(16):
                d=(x-7)**2+(y-7)**2
                if d<27: dot(8+f,x,y,15 if d<6 else (6,14,10)[f])
                # Upper-left quarter of the large shield, mirrored at runtime.
                ring=(x-15)**2+(y-15)**2
                if 169<=ring<=225: dot(11,x,y,15 if ring<196 else 14)
                if f==0 and abs(x-7)+abs(y-7)<5: dot(12,x,y,15 if abs(x-7)+abs(y-7)<2 else 10)
                if f==1 and 10<d<28: dot(13,x,y,14)
                if f==2 and (abs(x-7)<2 or abs(y-7)<2) and d<25: dot(14,x,y,15 if d<5 else 10)
    files['effects.png']=indexed_png(width,16,data,FX)
    icons=18; hud_width=icons*16; hud=[1]*(hud_width*16)
    def hp(f,x,y,c):
        if 0<=x<16 and 0<=y<16: hud[y*hud_width+f*16+x]=c
    for f in range(icons):
        selected=f>=13
        for y in range(16):
            for x in range(16):
                d=abs(x-7)+abs(y-7)
                if selected: hp(f,x,y,2)
                if d<8: hp(f,x,y,6 if selected else 2)
                if d<6: hp(f,x,y,2 if selected else 1)
        base=f-9 if selected else f
        if f<3:
            rows=GLYPHS['ABC'[f]]
            for y in range(7):
                bits=int(rows[y*2:y*2+2],16)
                for x in range(5):
                    if bits&(16>>x): hp(f,x+5,y+4,15)
            for x,y in [(7,0),(0,7),(14,7),(7,14)]: hp(f,x,y,(5,14,10)[f])
        elif f==3:
            for y in range(6,11):
                for x in range(2,14): hp(f,x,y,6 if y in (6,10) else 4)
            hp(f,2,5,15);hp(f,13,11,15)
        else:
            # Fixed HUD symbols: wind, fork, spell, familiar, barrier, gem,
            # stage flag and loadout seal. Selected copies use brighter rims.
            symbol={4:'>',5:'Y',6:'*',7:'O',8:'U',9:'*',10:'T',11:'M',12:'+'}.get(base,'*')
            rows=GLYPHS[symbol]
            for y in range(7):
                bits=int(rows[y*2:y*2+2],16)
                for x in range(5):
                    if bits&(16>>x): hp(f,x+5,y+4,15 if selected else 14 if base in (4,7,8) else 6)
    files['hud.png']=indexed_png(hud_width,16,hud,FX)
    button_pixels=[]
    for y in range(16):
        for f in range(3):button_pixels.extend(hud[y*hud_width+f*16:y*hud_width+f*16+16])
    files['buttons.png']=indexed_png(48,16,button_pixels,FX)
    # Keep only the HUD icons in use, then add crossed-out disabled variants.
    chosen=[3,4,5,6,7,8,13,14,15,16,17]; packed_width=16*16
    packed=[1]*(packed_width*16)
    for y in range(16):
        for n,source in enumerate(chosen):
            packed[y*packed_width+n*16:y*packed_width+n*16+16]=hud[y*hud_width+source*16:y*hud_width+source*16+16]
        for n in range(5):
            for x in range(16):
                color=hud[y*hud_width+(4+n)*16+x]
                if color!=1: color=2
                if 3<=x<=12 and y==14-x: color=3
                packed[y*packed_width+(11+n)*16+x]=color
    files['hud.png']=indexed_png(packed_width,16,packed,FX)
    # Nine partial-fill tiles plus two jewel end caps: no per-hit tile uploads.
    bar_width=11*8; bar=[1]*(bar_width*8)
    for frame in range(11):
        for y in range(8):
            for x in range(8):
                if frame<9:
                    color=7 if y==0 else 3 if y==7 else 5 if y==6 else 1
                    if 1<=y<=5 and x<frame: color=7 if y==1 else 6 if y==2 else 5 if y<5 else 4
                else:
                    d=abs(x-(4 if frame==9 else 3))+abs(y-3)
                    color=15 if d<2 else 14 if d<3 else 13 if d<4 else 1
                bar[y*bar_width+frame*8+x]=color
    files['health.png']=indexed_png(bar_width,8,bar,FX)
    # One resident 32x16 pattern repeated across the beam; the flowing pulse
    # comes from mirroring it, with no frame-by-frame tile upload.
    ribbon=[0]*(32*16)
    for y in range(16):
        for x in range(32):
            distance=abs(y-7)
            if distance<=6:
                ribbon[y*32+x]=12 if distance==6 else 13 if distance>=4 else 14 if distance>=2 else 15
            if distance in (2,3) and (x+y)%12<3: ribbon[y*32+x]=15
    files['ribbon.png']=indexed_png(32,16,ribbon,FX)
    return files

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args();files=assets()
    if any((args.output/n).exists() for n in files): parser.error('Choose a new output folder; UI assets exist')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created gradient fonts and native UI/effect atlas')
if __name__=='__main__': main()
