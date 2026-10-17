"""Original native-resolution Puzzle Lab fixtures; no resampling or external art."""
import struct
import zlib
from puzzle_glyphs import rows
WORLD=[(0,0,1),(1,1,1),(2,2,2),(3,3,3),(4,4,4),(5,4,3),(3,3,4),(4,5,5),(1,2,1),(2,3,2),(3,5,3),(5,6,4),(4,2,1),(6,4,2),(7,5,3),(7,7,6)]
ACTOR=[(0,0,1),(0,0,1),(1,3,2),(2,5,3),(5,7,4),(7,2,2),(7,5,3),(2,2,3),(7,6,2),(5,3,1),(5,3,6),(7,5,7),(3,1,2),(3,4,5),(7,7,6),(7,7,7)]
PROPS=[(0,0,1),(0,0,1),(1,2,3),(2,5,7),(5,7,7),(7,7,7),(7,5,1),(7,2,1),(3,1,2),(5,2,2),(7,4,3),(4,2,5),(6,4,7),(7,6,3),(3,4,5),(7,7,6)]
UI=[(0,0,1),(1,1,1),(2,6,3),(7,2,2),(7,6,2),(7,7,7),(3,3,3),(4,4,4),(4,5,6),(2,2,2),(6,4,2),(7,5,3),(2,3,3),(5,3,6),(7,6,3),(7,7,6)]
def png(w,h,p,palette,transparent=True):
    def chunk(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d)&0xffffffff)
    out=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,3,0,0,0))+chunk(b'PLTE',bytes(c*36 for rgb in palette for c in rgb))
    if transparent:out+=chunk(b'tRNS',b'\0'+b'\xff'*15)
    return out+chunk(b'IDAT',zlib.compress(b''.join(b'\0'+bytes(p[y*w:(y+1)*w]) for y in range(h)),9))+chunk(b'IEND',b'')

class Canvas:
    def __init__(self,w,h):self.w=w;self.h=h;self.p=[0]*(w*h)
    def dot(self,x,y,c):
        if 0<=x<self.w and 0<=y<self.h:self.p[y*self.w+x]=c
    def rect(self,x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w):self.dot(xx,yy,c)
    def line(self,x0,y0,x1,y1,c):
        dx=abs(x1-x0);sx=1 if x0<x1 else -1;dy=-abs(y1-y0);sy=1 if y0<y1 else -1;err=dx+dy
        while True:
            self.dot(x0,y0,c)
            if x0==x1 and y0==y1:break
            e=2*err
            if e>=dy:err+=dy;x0+=sx
            if e<=dx:err+=dx;y0+=sy
    def poly(self,pts,c,edge=None):
        for y in range(min(p[1] for p in pts),max(p[1] for p in pts)+1):
            xs=[]
            for i,(x0,y0) in enumerate(pts):
                x1,y1=pts[(i+1)%len(pts)]
                if min(y0,y1)<=y<max(y0,y1):xs.append(x0+(y-y0)*(x1-x0)//(y1-y0))
            xs.sort()
            for x0,x1 in zip(xs[::2],xs[1::2]):self.rect(x0,y,x1-x0+1,1,c)
        if edge is not None:
            for i,p in enumerate(pts):self.line(*p,*pts[(i+1)%len(pts)],edge)
    def ellipse(self,cx,cy,rx,ry,c):
        for y in range(-ry,ry+1):
            for x in range(-rx,rx+1):
                if x*x*ry*ry+y*y*rx*rx<=rx*rx*ry*ry:self.dot(cx+x,cy+y,c)

def sheet(frames,columns):
    w,h=frames[0].w,frames[0].h;c=Canvas(w*columns,h*(len(frames)//columns))
    for i,f in enumerate(frames):
        for y in range(h):
            for x in range(w):c.dot((i%columns)*w+x,(i//columns)*h+y,f.p[y*w+x])
    return c

def font_words():
    words=[]
    for code in range(32,128):
        for bits in rows(chr(code)):
            pixels=[1]+[15 if bits&(1<<(4-x)) else 1 for x in range(5)]+[1,1]
            words.append(sum(p<<(28-4*i) for i,p in enumerate(pixels)))
    return words


def tile(kind):
    c=Canvas(16,16);c.rect(0,0,16,16,2);c.line(0,0,15,0,3);c.line(0,0,0,15,3)
    c.line(1,15,15,15,1);c.line(15,1,15,15,1)
    if kind==1:
        c.rect(0,0,16,16,1);c.rect(1,1,14,12,6);c.rect(2,2,12,8,7)
        c.line(2,2,13,2,11);c.line(2,3,2,9,10);c.rect(4,5,8,3,6);c.line(3,12,13,12,3)
    if kind in (2,10):
        c.poly([(8,2),(13,7),(8,12),(3,7)],12,1);c.poly([(8,4),(11,7),(8,10),(5,7)],14 if kind==10 else 5)
        c.dot(8,6,15);c.line(5,13,10,13,14 if kind==10 else 12)
    if kind in (3,11):
        c.rect(3,3,10,10,1);c.rect(4,4,8,8,8 if kind==11 else 6);c.rect(6,6,4,4,15 if kind==11 else 10)
    if kind in (4,9):
        c.rect(0,0,3,16,12);c.rect(13,0,3,16,12);c.line(0,0,15,0,14)
        if kind==4:
            for x in (4,7,10):c.rect(x,2,2,13,13);c.dot(x,2,15)
            c.line(3,7,12,7,14)
        else:c.rect(4,1,8,2,10)
    if kind==5:
        c.rect(1,1,14,14,1);c.rect(3,3,10,10,0);c.line(2,2,13,2,5);c.line(2,2,2,13,5)
    if kind in (6,7):
        c.rect(2,2,12,12,1);c.rect(4,4,8,8,9 if kind==7 else 5)
        c.poly([(8,3),(12,8),(8,12),(4,8)],11 if kind==7 else 6)
        c.line(6,8,10,8,15 if kind==7 else 12);c.dot(9,7,15 if kind==7 else 12);c.dot(9,9,15 if kind==7 else 12)
    if kind==8:
        for x in range(1,15,4):c.rect(x,1,3,14,12);c.line(x,2,x,13,14)
        c.line(1,4,14,4,5);c.line(1,12,14,12,5)
    return c

def garden():
    c=Canvas(320,224);c.rect(0,0,320,224,1)
    for y in range(32,216,8):
        for x in range(0,320,16):
            c.rect(x,y,15,7,2);c.line(x,y,x+14,y,3)
    for x in (24,280):
        c.rect(x,42,16,153,1);c.rect(x+2,45,12,145,6);c.line(x+3,45,x+3,188,7)
        for y in (45,180):c.rect(x-4,y,24,9,5);c.line(x-3,y,x+18,y,14)
        for y in (72,124):
            c.rect(x+4,y,8,16,1);c.rect(x+5,y+1,6,12,9);c.rect(x+6,y+2,4,8,10);c.line(x+7,y+3,x+7,y+7,15)
    c.rect(60,44,200,152,1);c.line(61,45,258,45,14);c.line(61,45,61,194,12)
    c.line(62,194,258,194,5);c.line(258,46,258,194,5)
    return c

def hero(frame):
    c=Canvas(16,24);face=frame//2;step=frame%2
    c.ellipse(8,22,6,1,1);c.rect(4,18+step,3,4,5);c.rect(10,18-step,3,4,5)
    c.poly([(4,10),(11,10),(13,19),(3,19)],9,1);c.line(5,12,4,17,10)
    c.rect(5,18,7,2,12);c.rect(8,18,2,2,14)
    c.ellipse(8,6,6,6,1);c.ellipse(8,5,5,4,12);c.line(5,2,9,2,14)
    if face!=2:
        x=3 if face==0 else 7 if face==1 else 5
        c.rect(x,6,6,4,13);c.line(x+1,6,x+4,6,14);c.dot(x+(1 if face==0 else 4),8,1)
        if face==3:c.dot(x+1,8,1)
    else:c.rect(4,7,9,3,5)
    c.rect(2,12,2,5,13);c.rect(12,12,2,5,13)
    return c

def crate(frame):
    c=Canvas(16,16);c.rect(1,2,14,13,1);c.rect(2,3,12,10,12);c.line(2,3,13,3,14)
    c.rect(4,5,8,6,5);c.rect(5,5,6,5,9);c.rect(6,6,4,3,10 if not frame else 11);c.dot(7,6,15)
    c.line(3,12,12,12,14);c.rect(3,14,3,2,5);c.rect(10,14,3,2,5)
    return c

def frame_words():
    words=[]
    for tile in range(8):
        c=Canvas(8,8);c.rect(0,0,8,8,1)
        if tile in (0,1,2):c.line(0 if tile!=0 else 3,3,7 if tile!=2 else 4,3,14)
        if tile in (5,6,7):c.line(0 if tile!=5 else 3,4,7 if tile!=7 else 4,4,10)
        if tile in (0,3,5):c.line(3,0 if tile!=0 else 3,3,7 if tile!=5 else 4,14)
        if tile in (2,4,7):c.line(4,0 if tile!=2 else 3,4,7 if tile!=7 else 4,10)
        for y in range(8):words.append(sum(c.p[y*8+x]<<(28-x*4) for x in range(8)))
    return words

def block_words():
    words=[]
    for kind in range(9):
        c=Canvas(8,8);c.rect(0,0,8,8,1)
        if kind in range(1,8):
            color=[2,3,4,8,12,13,14][kind-1]
            c.rect(1,1,6,6,color);c.line(1,1,6,1,15);c.line(1,1,1,5,15)
            c.line(2,6,6,6,6);c.dot(4,3,15)
        elif kind==8:
            c.line(1,1,6,1,7);c.line(1,1,1,6,7);c.line(1,6,6,6,7);c.line(6,1,6,6,7)
        for y in range(8):words.append(sum(c.p[y*8+x]<<(28-x*4) for x in range(8)))
    return words

def assets():
    global WORLD,ACTOR,UI
    WORLD=[(0,0,1),(1,1,2),(2,2,3),(3,3,4),(4,4,5),(2,3,3),(3,4,4),(4,5,5),(2,4,3),(2,5,4),(4,7,5),(6,7,6),(4,3,2),(6,4,2),(7,6,3),(7,7,6)]
    ACTOR=WORLD[:];UI=[(0,0,1),(1,1,2),(2,5,4),(7,3,2),(7,6,3),(7,7,7),(3,3,4),(4,4,5),(4,5,6),(2,2,3),(6,4,2),(7,5,3),(2,3,3),(5,3,6),(7,6,3),(7,7,6)]
    out={}
    for name,c,pal,opaque in [('garden',garden(),WORLD,True),('atlas',sheet([tile(i) for i in range(12)],12),WORLD,True),('hero',sheet([hero(i) for i in range(8)],8),ACTOR,False),('crate',sheet([crate(i) for i in range(2)],2),WORLD,False)]:out[name+'.png']=png(c.w,c.h,c.p,pal,not opaque)
    out['font.h']=('static const u32 font_tiles[768]={'+','.join(hex(x) for x in font_words())+'};\n').encode()
    out['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1))for r,g,b in UI)+'};\nstatic const u32 frame_tiles[64]={'+','.join(hex(x)for x in frame_words())+'};\n').encode()
    out['block_tiles.h']=('static const u32 block_tiles[72]={'+','.join(hex(v)for v in block_words())+'};\n').encode()
    return out
