#!/usr/bin/env python3
"""Compose Carpet Lab's original FM/PSG score and short PCM effects offline."""
import argparse
import io
import math
from pathlib import Path
import struct
import wave

FNUM=[644,682,722,766,810,860,911,964,1022,1082,1147,1217]
# Storybook Hijaz-like intervals; this is original game music, not a claim of
# authenticity to a particular regional musical tradition.
SCALE=[0,1,4,5,7,8,10,12]
MELODIES={
 'title':[0,2,3,4,6,4,3,2,1,2,3,2,0,1,0,0],
 'loadout':[0,2,4,3,2,4,6,4,3,2,1,2,4,3,2,0],
 'stage1':[0,2,3,4,2,3,6,4,3,2,1,2,0,2,4,6],
 'stage2':[4,6,4,3,2,3,2,1,0,1,2,4,3,2,1,0],
 'stage3':[0,1,2,4,6,4,2,1,0,2,3,4,3,2,1,0],
 'boss1':[0,0,2,1,0,3,2,1,0,2,4,2,1,0,1,2],
 'boss2':[4,3,2,1,0,1,2,3,4,6,4,3,2,1,0,2],
 'boss3':[0,1,0,2,1,3,2,4,3,6,4,3,2,1,2,0],
 'clear':[0,2,3,4,6,7,6,4,3,4,6,7,7,7,7,7],
 'credits':[0,2,3,4,6,7,6,4,3,2,4,6,4,3,2,0],
 'over':[4,3,2,1,0,0,1,0,3,2,1,0,0,0,0,0]}

def music(name):
    step={'title':14,'loadout':9,'stage1':8,'stage2':11,'stage3':8,
          'boss1':7,'boss2':8,'boss3':6,'clear':7,'credits':10,'over':18}[name]
    beats=16 if name=='clear' else 64; frames=beats*step
    header=bytearray(256); header[:4]=b'Vgm '
    for offset,value in [(8,0x160),(12,3579545),(0x18,frames*735),(0x24,60),(0x2C,7670454),(0x34,204)]: struct.pack_into('<I',header,offset,value)
    struct.pack_into('<H',header,0x28,9); header[0x2A]=16
    data=bytearray()
    def port(reg,value,second=False): data.extend((0x53 if second else 0x52,reg,value))
    def ym(ch,reg,value): port(reg+ch%3,value,ch>=3)
    def key(ch,on): port(0x28,(0xF0 if on else 0)|(ch if ch<3 else ch+1))
    def note(ch,midi):
        block=midi//12-1; fnum=FNUM[midi%12]
        key(ch,False); ym(ch,0xA4,(block<<3)|(fnum>>8)); ym(ch,0xA0,fnum&255); key(ch,True)
    port(0x22,0);port(0x27,0);port(0x2B,0)
    for ch in range(6): key(ch,False)
    for v in (0x9F,0xBF,0xDF,0xFF): data.extend((0x50,v))
    for ch in range(4):
        for slot in (0,4,8,12):
            carrier=slot in (8,12)
            multiple=(1 if carrier else 2 if ch==0 else 1)
            level=(22 if ch==0 else 36 if ch==1 else 30 if ch==2 else 38) if carrier else (48 if ch==0 else 60)
            attack=31 if ch in (0,2,3) else 12
            for reg,value in [(0x30,multiple),(0x40,level),(0x50,attack),(0x60,11 if ch==0 else 6),(0x70,3),(0x80,0x56 if ch==0 else 0x48),(0x90,0)]: ym(ch,reg+slot,value)
        ym(ch,0xB0,4 if ch!=3 else 7);ym(ch,0xB4,0xC0)
    if name!='clear':
        struct.pack_into('<I',header,0x1C,256+len(data)-0x1C);struct.pack_into('<I',header,0x20,frames*735)
    melody=MELODIES[name]
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step; degree=melody[beat%16]
            root=(38,41,39,36)[(beat//16)%4]
            # Plucked lead, sustained answering reed, and a low fifth drone.
            octave=62 if name not in ('boss3','over') else 50
            if name=='clear': octave=62
            note(0,octave+SCALE[degree])
            if beat%4==0: note(1,root+24+(7 if beat%8 else 0));note(2,root)
            elif beat%4==2 and name not in ('title','over'): note(2,root+7)
            if name not in ('title','over') and beat%8 in (0,3,4,6):
                note(3,26 if beat%8 in (0,4) else 38)
                data.extend((0x50,0xE4,0x50,0xF9 if beat%8 in (0,4) else 0xF6))
        if frame%step==2: key(3,False);data.extend((0x50,0xFF))
        if frame%step==step-2: key(0,False)
        data.append(0x62)
    data.append(0x66);struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)

def effect(name):
    sequences={'fire':[(900,.025),(1350,.025)],'gem':[(1100,.04),(1650,.07),(2200,.11)],
               'upgrade':[(550,.06),(825,.06),(1100,.08),(1650,.18)],
               'hit':[(240,.06),(140,.1),(70,.2)],'kill':[(350,.04),(175,.08)],
               'boss':[(110,.12),(165,.12),(110,.2)],'clear':[(660,.07),(880,.08),(1320,.2)],
               'ribbon':[(330,.04),(440,.04),(550,.04),(440,.04)],'select':[(660,.035),(990,.04)],'denied':[(180,.05),(120,.05)]}
    rate=13300;samples=[];seed=731
    for pitch,duration in sequences[name]:
        for i in range(round(rate*duration)):
            t=i/rate;seed=(seed*1664525+1013904223)&0xffffffff
            noise=((seed>>16)/32768-1) if name in ('hit','kill','boss') else 0
            value=(math.sin(2*math.pi*pitch*t)+.18*math.sin(4*math.pi*pitch*t)+.25*noise)
            if name=='ribbon': value=.55*math.sin(2*math.pi*pitch*t+1.1*math.sin(2*math.pi*22*t))
            envelope=min(1,i/32)*math.exp(-t*(18 if name=='fire' else 10))
            samples.append(round(value*envelope*16000))
    output=io.BytesIO()
    with wave.open(output,'wb') as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'));wav.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return output.getvalue()

def assets():
    return {**{name+'.vgm':music(name) for name in MELODIES},**{name+'.wav':effect(name) for name in ['fire','gem','upgrade','hit','kill','boss','clear','select','denied','ribbon']}}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);args=parser.parse_args();files=assets()
    if any((args.output/n).exists() for n in files): parser.error('Choose a new output folder; audio sources exist')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created eleven original music tracks and ten short effects')
if __name__=='__main__': main()
