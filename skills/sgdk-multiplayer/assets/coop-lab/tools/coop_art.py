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
    c=Canvas(512,384);c.rect(0,0,512,384,2)
    # Broad, low-contrast flagstones keep small moving characters readable.
    for y in range(0,384,16):
        shift=16 if (y//16)%2 else 0
        for x in range(-shift,512,32):
            c.rect(x,y,31,15,2);c.line(x+2,y+1,x+28,y+1,3)
            c.line(x+1,y+2,x+1,y+12,3);c.line(x+2,y+14,x+30,y+14,1)
            if (x//32+y//16)%7==0:c.line(x+23,y+10,x+25,y+11,3)
    grid=world_rows()
    for y,row in enumerate(grid):
        for x,ch in enumerate(row):
            if ch!='#':continue
            px,py=x*16,y*16
            c.rect(px,py,16,16,5)
            # Stone edging follows the actual collision boundary, not every cell.
            top=y==0 or grid[y-1][x]!='#';bottom=y==23 or grid[y+1][x]!='#'
            left=x==0 or row[x-1]!='#';right=x==31 or row[x+1]!='#'
            if top:c.rect(px,py,16,3,7);c.line(px,py,px+15,py,8)
            if bottom:c.rect(px,py+11,16,5,1);c.line(px,py+11,px+15,py+11,6)
            if left:c.rect(px,py,3,12,6);c.line(px,py,px,py+11,7)
            if right:c.rect(px+13,py,3,13,1)
            c.ellipse(px+7,py+6,5,4,9);c.ellipse(px+6,py+4,4,3,10)
            c.line(px+4,py+3,px+7,py+3,11)
            if (x*3+y)%5==0:
                for dx,dy in [(0,-1),(-1,0),(1,0),(0,1)]:c.dot(px+8+dx,py+5+dy,13)
                c.dot(px+8,py+5,15)
    # Inlaid compass dais beneath each lantern; these remain walkable.
    for x,y in [(40,40),(472,40),(472,344),(40,344)]:
        c.ellipse(x,y,14,9,1);c.ellipse(x,y-1,13,8,5);c.ellipse(x,y-2,10,5,3)
        c.line(x-9,y-2,x+9,y-2,12);c.line(x,y-7,x,y+3,12)
    return c

def hero(variant,frame):
    c=Canvas(16,24);face=frame//3;step=frame%3;bob=1 if step==1 else 0;accent=3+variant
    c.ellipse(8,22,5,1,2)
    c.rect(4,18,3,4 if step!=2 else 3,1);c.rect(9,18,3,4 if step!=1 else 3,1)
    c.line(4,21 if step!=2 else 20,6,21 if step!=2 else 20,9)
    c.line(9,21 if step!=1 else 20,11,21 if step!=1 else 20,9)
    c.poly([(4,10+bob),(11,10+bob),(13,19),(2,19)],1)
    c.poly([(5,10+bob),(10,10+bob),(11,17),(4,17)],accent)
    c.line(4,12+bob,4,17,11);c.line(11,12+bob,11,17,2)
    c.rect(4,17,8,2,9);c.rect(7,17,2,2,14)
    c.poly([(4,1+bob),(10,1+bob),(13,5+bob),(12,10+bob),(3,10+bob),(2,5+bob)],1)
    c.poly([(5,2+bob),(9,2+bob),(11,5+bob),(11,8+bob),(4,8+bob),(3,5+bob)],accent)
    c.line(5,2+bob,8,2+bob,11)
    if face==2:c.line(5,8+bob,10,8+bob,2)
    else:
        fx=3 if face==3 else 8 if face==1 else 5
        c.rect(fx,6+bob,4,4,8);c.rect(fx,6+bob,4,2,7)
        c.dot(fx+(0 if face==3 else 3),7+bob,1)
        if face==0:c.dot(fx,7+bob,1)
    # A bright hand lantern gives every keeper a readable silhouette.
    lx=1 if face==3 else 12
    c.rect(lx,12+bob,3,6,1);c.rect(lx+1,13+bob,1,3,14);c.dot(lx+1,13+bob,15)
    c.line(lx,12+bob,lx+2,12+bob,6)
    return c

def moth(frame):
    c=Canvas(16,16);dazed=frame>=4;spread=1 if frame%2 else 0
    if dazed:
        c.poly([(2,7),(5,8),(6,12),(2,11)],5,1);c.poly([(13,7),(10,8),(9,12),(13,11)],5,1)
    else:
        c.poly([(7,7),(2,2+spread),(0,5),(1,10),(6,12)],5,1)
        c.poly([(8,7),(13,2+spread),(15,5),(14,10),(9,12)],5,1)
        c.line(2,5,5,8,6);c.line(13,5,10,8,6)
    c.ellipse(8,10,3,4,1);c.rect(6,7,4,5,3)
    if dazed:
        c.line(6,8,7,9,7);c.line(7,8,6,9,7);c.line(9,8,10,9,7);c.line(10,8,9,9,7)
        for x,y in ([(3,2),(11,3)] if frame==4 else [(5,3),(13,1)]):
            c.line(x-1,y,x+1,y,6);c.line(x,y-1,x,y+1,7)
    else:c.dot(6,8,7);c.dot(9,8,7)
    c.rect(7,12,2,2,5)
    return c

def pulse(frame):
    c=Canvas(32,32);r=(7,11,14,15)[frame]
    # Broad white/cyan arcs, with gaps that keep the keeper readable.
    for dx,dy in [(r,0),(-r,0),(0,r),(0,-r)]:
        x,y=16+dx,16+dy
        if dx:
            c.rect(x-(1 if dx>0 else 0),y-4,2,9,12)
            c.line(x-(1 if dx>0 else -1),y-3,x-(1 if dx>0 else -1),y+3,7)
            c.line(x-(2 if dx>0 else -2),y-2,x-(2 if dx>0 else -2),y+2,13)
        else:
            c.rect(x-4,y-(1 if dy>0 else 0),9,2,12)
            c.line(x-3,y-(1 if dy>0 else -1),x+3,y-(1 if dy>0 else -1),7)
            c.line(x-2,y-(2 if dy>0 else -2),x+2,y-(2 if dy>0 else -2),13)
    for sx,sy in [(-1,-1),(1,-1),(-1,1),(1,1)]:
        x,y=16+sx*(r*3//4),16+sy*(r*3//4)
        c.line(x-1,y,x+1,y,7);c.line(x,y-1,x,y+1,7)
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
        ('moth',sheets([moth(f) for f in range(6)],6),PROPS,False),('mote',sheets([mote(f) for f in range(4)],4),PROPS,False),
        ('pulse',sheets([pulse(f) for f in range(4)],4),PROPS,False),
        ('beacon',sheets([beacon(f) for f in range(4)],4),PROPS,False)]:out[name+'.png']=png(c.w,c.h,c.p,pal,not opaque)
    out['font.h']=('static const u32 font_tiles[768]={'+','.join(hex(v) for v in font_words())+'};\n').encode()
    out['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1))for r,g,b in UI)+'};\n').encode()
    return out
