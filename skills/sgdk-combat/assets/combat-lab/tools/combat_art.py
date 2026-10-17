"""Original native-resolution Combat Lab fixtures; no resampling or external art."""
import struct
import zlib
from combat_glyphs import rows
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

def floor():
    c=Canvas(320,224);c.rect(0,0,320,224,1)
    for y in range(48,224,16):
        shift=16 if (y//16)%2 else 0
        for x in range(-shift,320,32):
            c.rect(x,y,31,15,2);c.line(x+1,y+1,x+29,y+1,3)
            if (x//32+y//16)%5==0:c.line(x+23,y+10,x+26,y+11,3)
    for x in range(0,320,16):
        for y in (45,218):
            c.ellipse(x+8,y,10,6,8);c.ellipse(x+6,y-2,7,4,9)
            c.line(x+3,y-4,x+8,y-4,10);c.dot(x+8,y-5,11)
    # An inset practice ring with deliberately quiet flooring.
    c.line(18,60,301,60,12);c.line(18,211,301,211,12)
    c.line(14,60,14,211,5);c.line(305,60,305,211,5)
    c.ellipse(160,145,44,24,3);c.ellipse(160,145,41,22,2)
    c.poly([(160,126),(179,145),(160,164),(141,145)],5,12)
    c.poly([(160,133),(172,145),(160,157),(148,145)],2)
    for x in [76,220]:
        c.rect(x-2,99,28,28,1);c.rect(x,99,24,23,5);c.line(x,99,x+23,99,14)
        c.rect(x+3,102,18,17,12);c.rect(x+6,104,12,12,13)
        c.poly([(x+12,105),(x+17,110),(x+12,115),(x+7,110)],14)
    for x in (24,296):
        c.rect(x-3,70,7,19,1);c.rect(x-2,72,5,12,12);c.ellipse(x,72,3,5,13)
        c.ellipse(x,70,2,4,14);c.dot(x,68,15);c.rect(x-4,85,9,3,5)
    return c

def fighter(kind,frame):
    c=Canvas(24,32);face=frame//4 if frame<16 else frame-16;pose=frame%4 if frame<16 else 4
    accent,shine,shadow=[(3,4,2),(5,6,12),(9,8,2),(13,4,7),(10,11,12)][kind]
    c.ellipse(12,30,8,1,7)
    # Boots and broad shaded tunic/armor, all authored directly at 24x32.
    c.rect(7,23,4,6,1);c.rect(14,23,4,6,1)
    c.rect(8,24,2,4,7);c.rect(15,24,2,4,7)
    c.line(7,29,11,29,shadow);c.line(14,29,18,29,shadow)
    c.poly([(7,12),(17,12),(20,24),(4,24)],1)
    c.poly([(8,13),(16,13),(18,22),(6,22)],accent)
    c.line(8,14,7,20,shine);c.line(17,15,18,22,shadow)
    c.rect(7,23,11,2,shadow);c.rect(11,23,3,2,8)
    if kind==4:
        c.poly([(12,0),(20,11),(17,14),(6,14),(4,10)],1)
        c.poly([(12,2),(17,10),(7,10)],accent);c.line(9,10,15,10,shine)
        c.rect(8,11,8,3,shadow)
    else:
        c.ellipse(12,7,7,7,1);c.ellipse(11,6,5,5,accent)
        c.line(9,2,12,2,shine);c.line(7,4,7,7,shine)
        if kind==2:
            c.poly([(5,6),(2,2),(8,4)],8,1);c.poly([(17,4),(21,2),(19,7)],8,1)
        if kind==3:c.rect(8,7,9,3,1);c.rect(9,8,7,1,8)
        elif face!=2:
            fx=5 if face==3 else 12 if face==1 else 8
            c.rect(fx,8,6,5,6);c.line(fx+1,8,fx+4,8,14)
            c.dot(fx+(1 if face==3 else 4),10,1)
            if face==0:c.dot(fx+1,10,1)
    if kind<2:
        c.line(8,16,7,20,shine);c.rect(9,19,2,3,shadow)
        c.line(14,17,16,21,shadow);c.dot(12,21,shine)
        if face!=2:
            fx=5 if face==3 else 12 if face==1 else 8
            c.rect(fx-1,8,8,3,9);c.line(fx,8,fx+4,8,6)
            c.rect(fx,11,6,3,6);c.dot(fx+(0 if face==3 else 4),11,1)
            if face==0:c.dot(fx+1,11,1)
            c.dot(fx+5,12,14);c.rect(fx-2 if face==3 else fx+6,10,2,2,6)
        else:
            c.rect(7,8,10,4,9);c.line(8,8,13,8,6)
            c.poly([(7,12),(16,12),(18,21),(6,21)],shadow)
            c.line(7,13,8,20,accent)
    # Shield presentation follows world facing, not a camera-facing billboard.
    if face==0:
        sx,sy=(6,15) if pose==4 else (2,17)
        c.poly([(sx,sy),(sx+10,sy-1),(sx+12,sy+8),(sx+6,sy+13),(sx-1,sy+8)],1)
        c.poly([(sx+1,sy+1),(sx+9,sy),(sx+10,sy+7),(sx+6,sy+11),(sx+1,sy+7)],13)
        c.line(sx+2,sy+2,sx+8,sy+1,15 if pose==4 else shine)
        c.poly([(sx+6,sy+3),(sx+8,sy+6),(sx+6,sy+8),(sx+4,sy+6)],3)
    elif face==2:
        c.poly([(7,0),(17,0),(19,5),(16,9),(8,9),(5,5)],1)
        c.rect(8,1,8,5,13);c.line(8,1,15,1,15 if pose==4 else shine)
        c.line(7,6,17,6,7)
    else:
        left=face==3;sx=0 if left else 18
        c.poly([(sx+2,12),(sx+5,15),(sx+5,24),(sx+2,27),(sx,22),(sx,16)],1)
        c.rect(sx+1,15,3,8,13);c.line(sx+(1 if left else 3),15,sx+(1 if left else 3),22,15 if pose==4 else shine)
        c.dot(sx+2,18,3)
    # Four poses: idle, anticipated swing, committed swing, follow-through/hurt.
    wx=4 if face==3 else 20
    blade=5 if kind==3 else 15
    if pose==1:
        c.line(wx,16,wx-3 if face==3 else wx+1,2,1)
        c.line(wx,14,wx-2 if face==3 else wx,3,blade);c.line(wx-2,14,wx+2,14,8)
        if kind==4:c.ellipse(12,18,4,4,11);c.ellipse(12,18,2,2,15)
    elif pose==2:
        if face in (1,3):
            c.rect(2 if face==3 else 17,15,5,3,6)
        else:c.rect(13,6 if face==2 else 22,3,8,6)
    elif pose==3:
        c.line(wx,18,wx-5 if face==3 else wx-1,26,13)
        c.line(8,18,15,20,shadow)
    elif pose==4:
        pass
    else:
        c.line(wx,15,wx,25,1);c.line(wx,16,wx,24,13)
    return c

def bolt(team,frame):
    c=Canvas(8,8);col=3 if team==0 else 9;light=4 if team==0 else 10
    c.poly([(4,0),(7,4),(4,7),(0,4)],col,1)
    c.poly([(4,1),(6,4),(4,6),(1,4)],light);c.rect(3,2+frame%2,2,3,5)
    return c

def spark(kind,frame):
    c=Canvas(16,16);col=3 if kind else 6;r=(3,5,6,7)[frame]
    for dx,dy in [(r,0),(-r,0),(0,r),(0,-r)]:
        x,y=8+dx,8+dy;c.line(8+dx//2,8+dy//2,x,y,col);c.dot(x,y,5)
    if frame<2:c.ellipse(8,8,3-frame,3-frame,5)
    for sx,sy in [(-1,-1),(1,-1),(-1,1),(1,1)]:c.dot(8+sx*(r-1),8+sy*(r-1),col)
    return c

def sword(animation,frame):
    c=Canvas(24,24);heavy=animation>=4;facing=animation%4
    tips=[(8,1),(22,7),(22,17),(8,22)];tx,ty=tips[frame]
    trail=7 if heavy else 3;middle=10 if heavy else 4
    # A solid edge, stepped trailing arcs and a bright leading blade.
    if frame:
        ox,oy=tips[frame-1]
        c.poly([(1,12),(ox,oy),(tx,ty)],trail)
        c.line(ox,oy,tx,ty,middle)
    c.line(1,12,tx,ty,1);c.line(2,12,tx,ty,5)
    c.line(2,13,tx,min(23,ty+1),middle)
    c.line(0,11,3,14,6);c.dot(tx,ty,5)
    # Exact quarter-turns: no scaling, interpolation or softened edges.
    for _ in range((5-facing)%4):
        rotated=Canvas(24,24)
        for y in range(24):
            for x in range(24):rotated.dot(23-y,x,c.p[y*24+x])
        c=rotated
    return c

def title():
    c=floor()
    for x in range(34,289,36):
        c.rect(x,47,10,12,12);c.line(x,47,x+9,47,14)
    # Crossed ceremonial swords above the title; broad clustered silhouettes.
    c.line(126,64,191,110,1);c.line(128,64,191,108,11);c.line(129,64,192,107,15)
    c.line(192,64,127,110,1);c.line(190,64,127,108,11);c.line(189,64,126,107,15)
    for x,y in [(128,108),(190,108)]:c.line(x-6,y-3,x+5,y+3,14);c.line(x,y,x+4,y+6,12)
    return c

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

def assets():
    out={}
    for name,c,pal,opaque in [('court',floor(),WORLD,True),('title',title(),WORLD,True),
        ('fighters',sheet([fighter(k,f) for k in range(5) for f in range(20)],20),ACTOR,False),
        ('bolts',sheet([bolt(t,f) for t in range(2) for f in range(4)],4),PROPS,False),
        ('swords',sheet([sword(a,f) for a in range(8) for f in range(4)],4),PROPS,False),
        ('sparks',sheet([spark(t,f) for t in range(2) for f in range(4)],4),PROPS,False)]:
        out[name+'.png']=png(c.w,c.h,c.p,pal,not opaque)
    out['font.h']=('static const u32 font_tiles[768]={'+','.join(hex(x) for x in font_words())+'};\n').encode()
    out['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1))for r,g,b in UI)+'};\n').encode()
    return out
