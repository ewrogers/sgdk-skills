"""Original 64x48 native pixel ferry, decorative only."""
import struct
import zlib


def ferry():
    width,height=64,48
    pixels=bytearray(width*height)
    def rect(x,y,w,h,c):
        for py in range(y,y+h):
            for px in range(x,x+w): pixels[py*width+px]=c
    rect(0,33,64,15,2)
    for x,y,w in [(2,35,12),(22,38,18),(45,34,14),(8,44,22),(40,43,15)]:rect(x,y,w,1,3)
    rect(6,30,51,3,5); rect(10,33,43,3,6);rect(15,36,34,2,7)
    rect(20,19,27,11,8);rect(18,17,31,3,5)
    rect(22,21,6,6,9);rect(31,21,6,6,9);rect(40,21,5,6,9)
    rect(21,28,26,2,7);rect(35,10,6,7,6);rect(34,9,8,2,7)
    rect(10,23,2,7,7);rect(11,24,7,1,7);rect(16,23,2,7,7)
    rect(48,24,7,1,7);rect(54,23,2,7,7)
    rect(36,5,3,2,3);rect(39,2,5,2,3);rect(46,1,3,2,3)
    rect(6,7,2,2,10);rect(57,12,2,2,10);rect(12,15,1,1,10)
    palette=[(0,0,0),(0,36,72),(0,72,108),(36,108,144),(72,144,180),
             (72,36,72),(144,72,72),(216,144,72),(180,180,144),(252,216,144),
             (252,252,216)]+[(0,0,0)]*5
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    scanlines=b''.join(b'\0'+pixels[y*width:(y+1)*width] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+
            chunk(b'PLTE',bytes(c for rgb in palette for c in rgb))+chunk(b'IDAT',zlib.compress(scanlines,9))+chunk(b'IEND',b''))


VDP_PALETTE=[0x0000,0x0420,0x0640,0x0862,0x0A84,0x0424,0x0448,0x048C,
             0x08AA,0x08CE,0x0CEE,0,0,0,0,0]
