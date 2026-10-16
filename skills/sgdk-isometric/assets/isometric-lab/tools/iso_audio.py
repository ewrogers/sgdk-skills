#!/usr/bin/env python3
"""Original garden FM theme; generated register data, no recordings."""
import struct
FNUM=(644,682,722,766,810,860,911,964,1022,1082,1147,1217)
THEMES={'garden':(24,50,'major',[0,2,4,6,4,2,1,0,-1,1,3,4,2,1,0,-2])}


def music(name):
    step,root,mode,melody=THEMES[name];scale=(0,2,3,5,7,8,10)if mode=='minor'else(0,2,4,5,7,9,11)
    frames=step*128;header=bytearray(256);header[:4]=b'Vgm '
    for offset,value in [(8,0x160),(12,3579545),(0x18,frames*735),(0x20,frames*735),(0x24,60),(0x2C,7670454),(0x34,204)]:struct.pack_into('<I',header,offset,value)
    struct.pack_into('<H',header,0x28,9);header[0x2A]=16;data=bytearray()
    def port(reg,val,second=False):data.extend((0x53 if second else 0x52,reg,val))
    def ym(ch,reg,val):port(reg+ch%3,val,ch>=3)
    def key(ch,on):port(0x28,(0xf0 if on else 0)|(ch if ch<3 else ch+1))
    def note(ch,midi):
        key(ch,False);f=FNUM[midi%12];ym(ch,0xa4,((midi//12-1)<<3)|(f>>8));ym(ch,0xa0,f&255);key(ch,True)
    def degree(n):return scale[n%7]+12*(n//7)
    port(0x22,0x08);port(0x27,0);port(0x2b,0)
    for ch in range(6):key(ch,False)
    for val in [0x9f,0xbf,0xdf,0xff]:data.extend((0x50,val))
    for ch in range(5):
        # Harp/bell lead, two mellow string voices, round bass and soft answer.
        for slot in [0,4,8,12]:
            carrier=slot in (8,12)
            level=(26 if ch==0 else 45 if ch in(1,2)else 32 if ch==3 else 43)if carrier else(52 if ch==0 else 67)
            for reg,val in [(0x30,1 if carrier else 2 if ch==0 else 1),(0x40,level),(0x50,24 if ch==0 else 10 if ch in(1,2)else 27),(0x60,9 if ch==0 else 4),(0x70,3),(0x80,0x57 if ch==0 else 0x36),(0x90,0)]:ym(ch,reg+slot,val)
        ym(ch,0xb0,4);ym(ch,0xb4,0xc0)
    struct.pack_into('<I',header,0x1c,256+len(data)-0x1c)
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step;chord=(0,3,5,4)[(beat//16)%4];chord_root=root+degree(chord)-12
            # Two related eight-bar phrases with an answering upper register.
            m=melody[beat%16]+(1 if 64<=beat<96 and beat%8 in(2,6)else 0)
            note(0,root+12+degree(m))
            if beat%8==0:
                note(1,chord_root+24);note(2,chord_root+31);note(3,chord_root)
            elif beat%8==4:note(3,chord_root+7)
            if beat%4==2:note(4,root+degree(m-2))
            if beat%4==3:key(4,False)
        if frame%step==step-3:key(0,False)
        if frame%(step*4)==step*3:key(3,False)
        data.append(0x62)
    data.append(0x66);struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def assets():return {'garden.vgm':music('garden')}
