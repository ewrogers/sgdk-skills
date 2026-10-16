"""Original native indexed art; no image libraries or downloaded assets."""
import struct
import zlib

COLORS=[(0,0,1),(1,1,2),(2,2,3),(3,3,4),(1,6,7),(7,5,2),(7,2,3),(7,7,7)]+[(0,0,0)]*7+[(7,7,7)]

def png(width,height,pixels,palette):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xFFFFFFFF)
    colors=bytes(c*36 for rgb in palette for c in rgb)
    rows=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width]) for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+chunk(b'PLTE',colors)+chunk(b'tRNS',b'\0'+b'\xff'*15)+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')

def rect(p,w,h,x,y,rw,rh,c):
    for yy in range(max(0,y),min(h,y+rh)):
        for xx in range(max(0,x),min(w,x+rw)):p[yy*w+xx]=c

def assets():
    result={}
    for name,w,h in [('actor',16,16),('shot',8,8),('wall',8,32),('dot',8,8),('cursor',8,8)]:
        p=[0]*(w*h)
        if name=='actor':
            rect(p,w,h,2,2,12,12,1);rect(p,w,h,3,3,10,10,4)
            rect(p,w,h,5,5,6,3,7);rect(p,w,h,5,10,6,2,1)
        elif name=='shot':rect(p,w,h,0,0,4,4,5);rect(p,w,h,1,1,2,2,7)
        elif name=='wall':
            rect(p,w,h,0,0,4,32,6);rect(p,w,h,1,0,1,32,7)
        elif name=='dot':rect(p,w,h,0,0,2,2,5)
        else:
            rect(p,w,h,3,0,2,8,7);rect(p,w,h,0,3,8,2,7);rect(p,w,h,3,3,2,2,6)
        result[name+'.png']=png(w,h,p,COLORS)
    result['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1)) for r,g,b in COLORS)+'};\n').encode()
    return result
