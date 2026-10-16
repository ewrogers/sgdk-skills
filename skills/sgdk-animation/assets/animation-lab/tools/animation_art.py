"""Original native indexed art; no image libraries or downloaded assets."""
import struct
import zlib

HERO=[(0,0,0),(1,1,2),(2,1,3),(3,2,5),(5,3,7),(1,4,5),(2,6,6),(7,5,2),(7,7,7),(6,2,3),(3,1,2),(4,4,5),(0,0,0),(0,0,0),(0,0,0),(0,0,0)]
DUMMY=[(0,0,0),(1,1,2),(2,2,3),(4,3,2),(6,4,2),(7,6,3),(7,7,6),(7,2,2)]+[(0,0,0)]*8
UI=[(0,0,1),(1,1,2),(2,2,3),(3,3,4),(1,5,6),(7,5,2),(7,2,3),(7,7,7)]+[(0,0,0)]*7+[(7,7,7)]
WIRE=[(0,0,0),(1,1,2),(7,6,2),(2,7,7),(7,3,5),(7,7,7)]+[(0,0,0)]*10

def png(width,height,pixels,palette):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xFFFFFFFF)
    colors=bytes(c*36 for rgb in palette for c in rgb)
    rows=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width]) for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+chunk(b'PLTE',colors)+chunk(b'tRNS',b'\0'+b'\xff'*15)+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')

def rect(p,w,h,x,y,rw,rh,c):
    for yy in range(max(0,y),min(h,y+rh)):
        for xx in range(max(0,x),min(w,x+rw)):p[yy*w+xx]=c

def hero(pose):
    w=h=24;p=[0]*(w*h)
    def r(x,y,a,b,c):rect(p,w,h,x,y,a,b,c)
    if pose>=15:
        r(3,18,18,5,1);r(4,19,8,3,3);r(13,18,7,4,5);r(16,19,3,1,7)
        if pose==15:r(1,15,4,2,7)
        return p
    bob=1 if pose==1 or pose in (3,5) else 0
    if pose==13:bob=3
    if pose==14:bob=6
    leg=2 if pose==2 else -2 if pose==4 else 0
    r(7+leg,17+bob,5,5-bob,1);r(13-leg,17+bob,5,5-bob,1)
    r(7+leg,18+bob,4,3-bob if bob<3 else 1,3);r(13-leg,18+bob,4,3-bob if bob<3 else 1,4)
    r(6+leg,21,6,2,1);r(13-leg,21,6,2,1)
    r(6,9+bob,13,10,1);r(7,10+bob,11,7,3);r(8,10+bob,3,7,4);r(7,17,11,2,7)
    r(7,1+bob,12,9,1);r(8,2+bob,10,6,5);r(8,2+bob,3,5,6)
    r(12,4+bob,6,2,7);r(16,4+bob,2,1,8)
    r(4,11+bob,4,7,1);r(5,12+bob,2,4,5)
    if pose==6:r(15,9,5,5,7);r(17,6,3,5,8)
    elif pose in (7,8):r(17,10,7,4,1);r(17,11,7,2,7)
    elif pose in (9,10):r(17,13,5,4,7)
    else:r(17,11+bob,4,6,1);r(18,12+bob,2,4,5)
    if pose in (11,12):r(8,10+bob,9,2,9);r(1 if pose==11 else 21,6,2,3,8)
    return p

def dummy(frame):
    p=[0]*(24*32)
    def r(x,y,w,h,c):rect(p,24,32,x,y,w,h,c)
    r(10,9,4,21,1);r(3,29,18,3,1);r(5,29,14,2,3)
    r(3,3,18,18,1);r(4,4,16,16,7 if frame else 4)
    r(6,6,12,12,5);r(8,8,8,8,3);r(10,10,4,4,6)
    r(1,10,3,5,1);r(20,10,3,5,1)
    return p

def outline(w,h,x,y,rw,rh,c):
    p=[0]*(w*h)
    for args in [(x,y,rw,1),(x,y+rh-1,rw,1),(x,y,1,rh),(x+rw-1,y,1,rh)]:rect(p,w,h,*args,c)
    return p

def arc(frame):
    p=[0]*512
    for x in range(28):
        y=(x//5+frame*2)%8+3
        rect(p,32,16,x+2,y,3,3,2);rect(p,32,16,x+2,y,2,1,5)
    return p

def sheet(frames,w,h):
    return [v for y in range(h) for frame in frames for v in frame[y*w:(y+1)*w]]

def assets():
    specs={'hero.png':([hero(i) for i in range(17)],24,24,HERO),
        'dummy.png':([dummy(i) for i in range(2)],24,32,DUMMY),
        'arc.png':([arc(i) for i in range(2)],32,16,WIRE),
        'attack_box.png':([outline(32,16,2,2,28,12,4)],32,16,WIRE),
        'body_box.png':([outline(16,24,2,2,12,20,3)],16,24,WIRE),
        'target_box.png':([outline(16,32,2,3,12,26,3)],16,32,WIRE)}
    result={name:png(len(frames)*w,h,sheet(frames,w,h),pal) for name,(frames,w,h,pal) in specs.items()}
    result['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1)) for r,g,b in UI)+'};\n').encode()
    return result
