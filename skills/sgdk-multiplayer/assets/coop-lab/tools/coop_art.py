"""Original native-resolution art for Bellkeepers. Standard library only."""
import ast
from pathlib import Path
import struct
import zlib
from coop_glyphs import rows
WORLD=[(0,0,1),(1,1,2),(2,2,3),(3,3,4),(4,4,5),(2,3,3),(3,4,4),(4,5,4),(5,5,4),(1,3,3),(2,4,4),(3,5,5),(5,4,2),(7,5,2),(7,6,4),(7,7,6)]
HERO=[(0,0,1),(1,1,2),(2,2,3),(2,6,7),(7,3,3),(5,4,7),(7,6,2),(7,6,4),(5,3,2),(3,2,2),(1,3,3),(6,7,7),(3,4,5),(5,6,6),(7,7,5),(7,7,7)]
PROPS=[(0,0,1),(1,1,2),(3,2,3),(5,2,3),(7,3,3),(7,5,2),(7,7,4),(7,7,7),(2,3,3),(3,5,4),(4,6,5),(1,3,4),(2,5,6),(4,6,7),(5,4,7),(7,6,7)]
UI=[(0,0,1),(1,1,2),(2,6,7),(7,3,3),(5,4,7),(7,6,2),(3,3,4),(5,5,6),(7,6,4),(7,4,3),(3,6,4),(4,6,7),(2,3,4),(5,4,7),(7,7,5),(7,7,6)]
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

def world_rows():
    return [ast.literal_eval(line.rstrip(',')) for line in (Path(__file__).resolve().parents[1]/'inc/world_rows.inc').read_text().splitlines()]

def world():
    c=Canvas(512,384)
    for y,row in enumerate(world_rows()):
        for x,ch in enumerate(row):
            px,py=x*16,y*16
            if ch=='#':
                c.rect(px,py,16,16,1);c.rect(px+1,py+1,14,12,5);c.line(px+1,py+1,px+14,py+1,7)
                c.rect(px+2,py+3,12,6,9);c.rect(px+4,py+2,3,4,10);c.rect(px+8,py+5,4,4,11)
                c.line(px+1,py+13,px+14,py+13,3);c.line(px+3,py+15,px+14,py+15,2)
                if (x+y)%4==0:c.dot(px+9,py+3,13);c.dot(px+10,py+3,14)
            else:
                base=2 if (x+y)%3 else 3;c.rect(px,py,16,16,base)
                c.line(px,py+15,px+15,py+15,1);c.line(px+15,py,px+15,py+15,1)
                c.line(px+1,py+1,px+12,py+1,4);c.line(px+1,py+2,px+1,py+12,3)
                c.dot(px+5,py+9,3);c.dot(px+10,py+6,4 if (x+y)%2 else 2)
                if y in (3,20) and x%3==0:c.poly([(px+8,py+3),(px+11,py+7),(px+8,py+11),(px+5,py+7)],12);c.dot(px+8,py+5,14)
    return c

def hero(variant,frame):
    c=Canvas(16,24);face=frame//3;step=frame%3;bob=1 if step else 0;accent=3+variant
    c.ellipse(8,22,6,1,2)
    for x,dy in [(4,1 if step==1 else 0),(9,1 if step==2 else 0)]:c.rect(x,18,3,4+dy,1);c.rect(x,19,2,2+dy,9)
    c.poly([(4,9+bob),(11,9+bob),(13,20),(2,20)],1)
    c.poly([(5,10+bob),(10,10+bob),(11,18),(4,18)],accent)
    c.line(5,11+bob,4,17,11);c.line(11,12+bob,11,18,2);c.rect(4,17,8,2,12);c.dot(8,17,14)
    c.ellipse(8,6+bob,6,6,1);c.ellipse(8,5+bob,5,4,accent)
    if face==2:
        c.line(5,6+bob,11,6+bob,11);c.rect(6,8+bob,4,2,2)
    else:
        fx=4 if face==3 else 7 if face==1 else 5
        c.rect(fx,6+bob,5,4,8);c.rect(fx+1,6+bob,3,3,7)
        c.dot(fx+(3 if face==1 else 0),7+bob,1)
        if face==0:c.dot(fx+3,7+bob,1)
    c.rect(2 if face==3 else 12,12+bob,2,4,1);c.dot(2 if face==3 else 13,13+bob,14)
    return c

def moth(frame):
    c=Canvas(16,16);spread=2 if frame%2 else 0
    for mirror in (False,True):
        points=[(8,7),(2-spread,2),(1,8),(5,11),(8,10)]
        if mirror:points=[(15-x,y) for x,y in points]
        c.poly(points,2,1)
    c.ellipse(8,8,4,6,1);c.ellipse(8,7,3,4,3);c.rect(6,5,2,2,6);c.rect(9,5,2,2,6)
    c.line(7,1,5,0,4);c.line(9,1,11,0,4);c.rect(7,12,2,2,5)
    return c

def mote(frame):
    c=Canvas(8,8);rx=(2,3,2,1)[frame];c.poly([(4,0),(4+rx,3),(4,7),(4-rx,3)],5,1)
    c.poly([(4,1),(4+max(1,rx-1),3),(4,6),(4-max(1,rx-1),3)],6);c.dot(4,2,7);return c

def beacon(frame):
    c=Canvas(16,24);c.ellipse(8,22,6,1,1);c.rect(5,17,6,5,2);c.rect(4,21,8,2,1)
    c.line(8,3,8,18,5);c.poly([(8,0),(14,8),(13,16),(3,16),(2,8)],1)
    c.rect(4,7,8,8,8 if frame==0 else 5);c.rect(6,8,4,6,9 if frame==0 else 6)
    if frame:c.rect(7,9-frame%2,2,5,7);c.dot(5,7,7)
    c.line(3,7,13,7,5);c.line(4,16,12,16,5);c.line(8,7,8,16,1)
    return c

def title():
    c=Canvas(320,224);c.rect(0,0,320,224,1)
    for i in range(36):c.dot((i*73+9)%320,(i*29+7)%144,5 if i%3 else 8)
    c.ellipse(251,42,20,20,7);c.ellipse(257,38,18,19,1)
    # A quiet garden gate, cut from broad pixel clusters with no resampling.
    for side in (0,1):
        x=side*258
        c.rect(x,112,62,112,2);c.rect(x+5,110,50,7,4)
        for yy in range(124,224,16):
            c.line(x+2,yy,x+59,yy,1)
            c.line(x+(16 if yy%32 else 40),yy-15,x+(16 if yy%32 else 40),yy-1,1)
        for k in range(7):c.ellipse(x+8+k*7,99+(k%2)*4,13,17,9);c.ellipse(x+7+k*7,95+(k%2)*4,8,9,10)
    c.poly([(52,224),(101,160),(219,160),(268,224)],3)
    for y in range(168,224,12):c.line(52+(224-y),y,268-(224-y),y,4)
    for x in range(64,258,32):c.line(160,160,x,223,2)
    for x in (84,236):
        c.rect(x-3,126,7,54,1);c.rect(x-2,127,2,50,12)
        c.rect(x-11,113,23,26,1);c.rect(x-8,116,17,18,12);c.rect(x-5,119,11,13,13);c.rect(x-2,119,5,10,15)
        c.line(x,107,x,113,12);c.line(x-10,114,x+10,114,14);c.line(x-10,137,x+10,137,14)
    return c

def sheets(frames,columns):
    w,h=frames[0].w,frames[0].h;rows_count=len(frames)//columns;out=Canvas(w*columns,h*rows_count)
    for i,f in enumerate(frames):
        for y in range(h):
            for x in range(w):out.dot((i%columns)*w+x,(i//columns)*h+y,f.p[y*w+x])
    return out

def font_words():
    out=[]
    for code in range(32,128):
        glyph=rows(chr(code))
        if chr(code)=='*':glyph=[0b01010,0b11111,0b11111,0b01110,0b00100,0,0,0]
        for bits in glyph:
            pixels=[1]+[15 if bits&(1<<(4-x)) else 1 for x in range(5)]+[1,1]
            out.append(sum(v<<(28-4*i) for i,v in enumerate(pixels)))
    return out

def assets():
    out={}
    for name,c,pal,opaque in [('world',world(),WORLD,True),('title',title(),WORLD,True),
        ('hero',sheets([hero(v,f) for v in range(4) for f in range(12)],12),HERO,False),
        ('moth',sheets([moth(f) for f in range(4)],4),PROPS,False),('mote',sheets([mote(f) for f in range(4)],4),PROPS,False),
        ('beacon',sheets([beacon(f) for f in range(4)],4),PROPS,False)]:out[name+'.png']=png(c.w,c.h,c.p,pal,not opaque)
    out['font.h']=('static const u32 font_tiles[768]={'+','.join(hex(v) for v in font_words())+'};\n').encode()
    out['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1))for r,g,b in UI)+'};\n').encode()
    return out
