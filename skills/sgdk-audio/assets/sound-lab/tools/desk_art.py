"""Original native-resolution Sound Desk art and readable 5x7 font."""
import struct
import zlib
from desk_glyphs import rows
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

PALETTE=[(0,0,1),(1,1,2),(2,2,3),(3,3,4),(4,4,5),(6,6,6),(7,7,7),(7,6,3),(6,4,2),(4,2,1),(2,5,5),(3,7,6),(2,3,4),(3,4,6),(6,3,4),(7,4,4)]

def desk():
    c=Canvas(320,224);c.rect(0,0,320,224,1)
    for y in range(0,224,8):c.line(0,y,319,y,2)
    # A small recording console, built at native resolution, with no resampling.
    c.rect(8,48,104,133,0);c.rect(10,50,100,128,3);c.line(10,50,109,50,5)
    c.rect(15,55,90,76,1);c.line(15,55,104,55,8)
    for x in (33,85):
        c.ellipse(x,85,20,23,0);c.ellipse(x,85,17,20,2);c.ellipse(x,85,13,15,0)
        c.ellipse(x,85,9,10,3);c.ellipse(x,85,5,6,1);c.dot(x-3,81,5)
        c.rect(x-14,112,28,8,0)
        for i in range(5):c.line(x-10+i*5,114,x-10+i*5,117,8)
    c.rect(48,65,24,41,8);c.rect(51,68,18,35,1)
    for y in range(72,100,6):c.line(54,y,65,y,10 if y<88 else 7)
    c.rect(17,139,86,29,1)
    for x in range(24,100,12):
        c.line(x,143,x,163,4);c.rect(x-3,149+(x%3)*3,7,5,8);c.line(x-2,150+(x%3)*3,x+2,150+(x%3)*3,7)
    c.rect(35,183,58,4,0);c.rect(47,187,34,3,2)
    return c

def font_words():
    words=[]
    for code in range(32,128):
        for bits in rows(chr(code)):
            pixels=[1]+[6 if bits&(1<<(4-x))else 1 for x in range(5)]+[1,1]
            words.append(sum(p<<(28-4*i)for i,p in enumerate(pixels)))
    return words

def assets():
    c=desk();out={'desk.png':png(c.w,c.h,c.p,PALETTE,False)}
    words=[]
    # Blank, eight meter fills and two decorative line tiles, all in one bank.
    for fill in range(9):
        for y in range(8):words.append(sum((10 if x<fill and 2<=y<=5 else 2 if 1<=y<=6 else 1)<<(28-x*4)for x in range(8)))
    words += [0x11111111,0x11111111,0x77777777,0x88888888,0x11111111,0x11111111,0x11111111,0x11111111]
    words += [0x11111111]*8
    out['desk_font.h']=('static const u32 desk_font[768]={'+','.join(hex(v)for v in font_words())+'};\n').encode()
    out['desk_ui.h']=('static const u16 desk_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1))for r,g,b in PALETTE)+'};\nstatic const u32 desk_meters[88]={'+','.join(hex(v)for v in words)+'};\n').encode()
    return out
