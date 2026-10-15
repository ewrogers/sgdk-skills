#!/usr/bin/env python3
"""Generate three original FM stealth loops, from uneasy to urgent."""
import argparse
from pathlib import Path
import struct


def music(area):
    step=(30,24,18)[area]
    notes=((644,766,860,766,574,683,766,574),
           (644,683,766,910,860,766,683,574),
           (644,910,766,683,574,860,683,910))[area]
    frames=step*32
    header=bytearray(256); header[:4]=b'Vgm '
    for offset,value in ((8,0x160),(0x18,frames*735),(0x20,frames*735),
                         (0x24,60),(0x2C,7670454),(0x34,204)):
        struct.pack_into('<I',header,offset,value)
    data=bytearray()
    def ym(register,value): data.extend((0x52,register,value))
    ym(0x22,0); ym(0x27,0); ym(0x2B,0)
    for ch in range(6): ym(0x28,ch if ch<3 else ch+1)
    for ch in range(3):
        for slot,multiple,level in ((0,2,42-area*5),(4,1,127),(8,1,28),(12,1,127)):
            for register,value in ((0x30,multiple),(0x40,level),(0x50,24),
                                   (0x60,8),(0x70,4),(0x80,0x58),(0x90,0)):
                ym(register+slot+ch,value)
        ym(0xB0+ch,4); ym(0xB4+ch,0xC0)
    struct.pack_into('<I',header,0x1C,256+len(data)-0x1C)
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step
            for ch,value,block in ((0,notes[beat%8],3),(1,notes[(beat//4)%8],1),
                                   (2,notes[(beat+3)%8],2)):
                ym(0x28,ch)
                if ch<2 or area==2 or (area==1 and beat%2==0):
                    ym(0xA4+ch,(block<<3)|(value>>8)); ym(0xA0+ch,value&255); ym(0x28,0xF0|ch)
        if frame%step==step-5:
            ym(0x28,0); ym(0x28,2)
        data.append(0x62)
    data.append(0x66)
    struct.pack_into('<I',header,4,len(header)+len(data)-4)
    return bytes(header+data)


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('output',type=Path)
    args=parser.parse_args(); files={f'area_{i+1}.vgm':music(i) for i in range(3)}
    if any((args.output/name).exists() for name in files): parser.error('Choose a new output folder')
    args.output.mkdir(parents=True,exist_ok=True)
    for name,data in files.items(): (args.output/name).write_bytes(data)
    print('Created three original FM loops')


if __name__=='__main__': main()
