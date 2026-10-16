"""Original native-resolution pixel art for Lantern Aqueduct. Standard library only."""
import struct
import zlib
from iso_glyphs import rows as glyph_rows
WORLD=[(0,1,2),(1,2,3),(1,3,4),(2,4,5),(3,4,4),(4,5,4),(5,6,5),(7,7,5),(2,4,3),(3,5,3),(4,6,4),(5,4,3),(3,3,3),(6,4,4),(7,5,2),(6,6,6)]
PROPS=[(0,1,2),(1,1,2),(2,2,3),(3,3,4),(2,4,4),(3,5,5),(5,6,5),(7,7,6),(5,4,3),(6,5,4),(7,6,4),(7,5,2),(4,2,4),(6,3,5),(7,5,6),(7,7,7)]
HERO=[(0,1,2),(1,1,2),(2,2,4),(3,3,6),(4,4,7),(1,4,5),(2,6,6),(5,7,7),(4,2,2),(6,4,3),(7,6,4),(7,7,6),(6,3,2),(7,4,1),(7,6,1),(7,7,7)]
UI=[(0,1,2),(1,1,2),(2,2,3),(4,4,5),(6,5,3),(7,6,4),(7,7,5),(7,7,7)]+[(0,1,2)]*7+[(7,7,6)]

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

def diamond(c,x,y,rx,ry,color,edge=None):c.poly([(x,y-ry),(x+rx,y),(x,y+ry),(x-rx,y)],color,edge)
def box(c,cx,cy,rx,ry,h,top=7,left=9,right=8):
    c.poly([(cx-rx,cy-h),(cx,cy+ry-h),(cx,cy+ry),(cx-rx,cy)],left,1)
    c.poly([(cx,cy+ry-h),(cx+rx,cy-h),(cx+rx,cy),(cx,cy+ry)],right,1)
    diamond(c,cx,cy-h,rx,ry,top,1)

def ground():
    c=Canvas(512,256)
    for i in range(48):
        x=(i*137+31)%512;y=(i*47+7)%256
        c.dot(x,y,2)
        if i%7==0:c.line(x-1,y,x+1,y,3)
    # Quiet distant cloud ribbons; no blurred/downscaled texture.
    for x,y in [(60,48),(400,28),(30,212),(416,221)]:
        c.ellipse(x,y,36,3,1);c.ellipse(x-10,y-2,16,3,1)
    # Floating masonry foundation, with a clean silhouette and clustered vines.
    c.poly([(96,144),(256,224),(256,248),(96,168)],11,1)
    c.poly([(256,224),(416,144),(416,168),(256,248)],12,1)
    for yy in range(5,23,6):
        c.line(100,144+yy,252,220+yy,12)
        c.line(260,220+yy,412,144+yy,1)
    for x,y in [(128,168),(160,184),(192,200),(224,216),(288,216),(320,200),(352,184),(384,168)]:
        c.line(x,y,x+10,y+5,1);c.line(x,y,x,y+8,1)
        for k in range(5):c.rect(x+k%2,y+6+k*2,2,2,8 if k%2 else 9)
    for depth in range(19):
        for x in range(10):
            y=depth-x
            if not 0<=y<10:continue
            px=256+16*(x-y);py=64+8*(x+y)+8
            garden=(x<2 and y>5) or (x>7 and y>5) or (y<2 and x not in (4,5))
            canal=x in (4,5)
            base=8 if garden else 2 if canal else 5
            diamond(c,px,py,16,8,base,1)
            if garden:
                diamond(c,px,py,13,6,9)
                for k in range(3):
                    xx=px+((x*3+y*7+k*5)%17)-8;yy=py+((x+y+k*3)%5)-2
                    c.line(xx,yy,xx,yy-2,10)
                if (x+y)%3==0:
                    c.rect(px-3,py-2,2,2,13);c.dot(px+4,py+1,7)
            elif canal:
                diamond(c,px,py,13,6,3)
                c.line(px-7,py,px,py+3,2);c.line(px,py+3,px+7,py,2)
                c.dot(px+4,py-2,6)
            else:
                diamond(c,px,py,13,6,6 if (x+y)%3 else 5)
                c.line(px-10,py-1,px-2,py-5,7)
                if (x*7+y)%5==0:c.line(px+3,py+2,px+6,py+3,4)
    # Tiny gold wayfinding inlays at the two stair approaches.
    for x,y in [(1,5),(8,5),(4,2),(5,8)]:
        diamond(c,256+16*(x-y),72+8*(x+y),4,2,14,1)
    return c

def tower():
    c=Canvas(32,64);box(c,16,55,15,7,36,6,9,8)
    for yy in (32,40,48):
        c.line(2,yy,15,yy+6,8);c.line(17,yy+6,29,yy,3)
    # Glazed hip roof, bronze ribs and a gold finial.
    c.poly([(1,19),(16,6),(16,26)],5,1);c.poly([(16,6),(31,19),(16,26)],4,1)
    c.line(16,6,16,25,11);c.line(3,19,15,24,6);c.line(18,24,28,19,5)
    c.rect(15,2,2,5,11);c.dot(15,1,15)
    for x,y in [(8,36),(23,35)]:
        c.rect(x-2,y-2,5,10,1);c.rect(x-1,y-1,3,7,11);c.rect(x,y,1,5,15)
        c.line(x-2,y+8,x+3,y+6,10)
    # Violet pennant with gold piping on the near face.
    c.poly([(3,29),(11,33),(11,46),(7,43),(3,42)],12,1)
    c.line(4,30,10,33,11);c.line(4,32,4,40,13)
    c.dot(7,36,14);c.dot(7,39,11)
    c.line(16,28,16,57,7)
    return c

def tree():
    c=Canvas(32,48);diamond(c,16,40,13,6,4,1)
    c.poly([(13,39),(15,20),(19,19),(18,39)],8,1)
    c.line(16,31,7,23,1);c.line(17,28,25,22,1)
    for x,y,rx,ry in [(8,21,7,7),(23,22,7,7),(15,11,9,8),(7,13,5,6),(24,13,5,6),(16,23,10,7)]:
        c.ellipse(x,y,rx,ry,1);c.ellipse(x,y-1,rx-1,ry-1,12)
        c.ellipse(x-1,y-2,max(1,rx-3),max(1,ry-3),13)
    for x,y in [(5,12),(12,6),(20,11),(24,20),(14,20),(8,24),(18,26)]:
        c.rect(x,y,2,2,14);c.dot(x,y-1,15)
    return c

def deck():
    c=Canvas(32,24);box(c,16,16,15,7,4,7,9,8)
    diamond(c,16,12,12,5,6)
    c.line(4,12,15,7,10);c.line(16,17,27,12,8)
    diamond(c,16,12,3,1,11)
    c.dot(16,11,15)
    return c

def post():
    c=Canvas(16,40);box(c,8,35,6,3,28,7,9,8)
    box(c,8,35,7,3,3,6,9,8);box(c,8,11,7,3,4,7,10,9)
    c.line(6,14,6,31,7);c.line(9,15,9,32,3)
    return c

def ramp(reverse=False):
    c=Canvas(32,48)
    # Four 8-pixel risers. Each step spans a quarter of one map cell.
    def p(x,y,z):return (16+(x-y)//16,36+(x+y)//32-z)
    for step in range(4):
        # Far-to-near along x. Mirrors are authored here, not a projection-breaking HFlip.
        lo=-128+step*64;hi=lo+64;z=(4-step if reverse else step+1)*8
        a=p(lo,-128,z);b=p(hi,-128,z);d=p(lo,128,z);e=p(hi,128,z)
        c.poly([d,e,p(hi,128,0),p(lo,128,0)],9,1)
        c.poly([b,e,p(hi,128,0),p(hi,-128,0)],8,1)
        c.poly([a,b,e,d],7,1);c.line(*d,*e,10)
    return c

def fountain(frame):
    c=Canvas(32,32);box(c,16,24,15,7,5,6,9,8);diamond(c,16,19,12,5,4,1)
    diamond(c,16,19,9,3,5);c.line(10,20,15,22,7)
    c.rect(15,9,3,11,3);c.line(16,8,16,17,15);c.line(13,7,16,5,7);c.line(16,5,20,10,6)
    for k in range(4):
        x=8+k*5;y=14+(k+frame)%4;c.dot(x,y,15);c.dot(x+1,y+2,5)
    return c

def hero(frame):
    c=Canvas(24,32);face=frame//4;step=frame%4;bob=1 if step in (1,3) else 0
    # Soft ground shadow is part of the foot-anchored sprite.
    c.ellipse(12,29,7,2,2)
    for x,offset in [(8,1 if step==1 else -1 if step==3 else 0),(13,-1 if step==1 else 1 if step==3 else 0)]:
        c.rect(x,24,4,5+offset,1);c.rect(x,25,3,3+offset,10)
    c.poly([(7,13+bob),(16,13+bob),(19,26),(5,26)],1)
    c.poly([(8,14+bob),(15,14+bob),(17,24),(7,24)],3)
    c.line(8,16+bob,8,23,5);c.line(16,16+bob,17,23,4)
    c.rect(8,22,9,2,14);c.rect(11,23,2,2,11)
    c.ellipse(12,8+bob,7,7,1);c.ellipse(12,7+bob,6,5,3)
    if face==0:
        c.line(8,9+bob,16,9+bob,4);c.rect(9,5+bob,2,3,5)
    else:
        fx=9 if face==3 else 11 if face==1 else 9
        c.rect(fx,8+bob,6,5,9);c.rect(fx+1,8+bob,4,4,10)
        c.rect(fx+(3 if face==1 else 0),9+bob,1,1,1)
        if face==2:c.rect(fx+4,9+bob,1,1,1)
        c.line(7,6+bob,17,6+bob,5)
    # A warm hanging lantern, framed in bronze rather than a stray hand pixel.
    lx=3 if face==3 else 19;c.line(lx,17+bob,lx,21+bob,1)
    c.rect(lx-2,21+bob,5,6,1);c.rect(lx-1,22+bob,3,4,13);c.rect(lx,22+bob,1,3,15)
    c.line(lx-2,27+bob,lx+2,27+bob,14)
    return c

def marker():
    c=Canvas(16,8);diamond(c,8,4,7,3,0,11);c.dot(8,4,15);return c

def sheet(frames):
    w,h=frames[0].w,frames[0].h
    return len(frames)*w,h,[v for y in range(h) for f in frames for v in f.p[y*w:(y+1)*w]]

def assets():
    c=ground();out={'ground.png':png(c.w,c.h,c.p,WORLD,False)}
    specs={'tower':([tower()],PROPS),'tree':([tree()],PROPS),'deck':([deck()],PROPS),'post':([post()],PROPS),
           'ramp_l':([ramp()],PROPS),'ramp_r':([ramp(True)],PROPS),'fountain':([fountain(i) for i in range(3)],PROPS),
           'hero':([hero(i) for i in range(16)],HERO),'marker':([marker()],HERO)}
    for name,(frames,pal) in specs.items():w,h,p=sheet(frames);out[name+'.png']=png(w,h,p,pal)
    out['ui.h']=('static const u16 ui_colors[16]={'+','.join(hex((b<<9)|(g<<5)|(r<<1)) for r,g,b in UI)+'};\n').encode()
    out['font.h']=('static const u32 font_tiles[768]={'+','.join(hex(v) for v in font_words())+'};\n').encode()
    return out

def font_words():
    font=bytearray()
    for code in range(32,128):
        for bits in glyph_rows(chr(code)):
            p=[1]+[15 if bits&(1<<(4-x)) else 1 for x in range(5)]+[1,1]
            font.extend((p[i]<<4)|p[i+1] for i in range(0,8,2))
    return [int.from_bytes(font[i:i+4],'big') for i in range(0,len(font),4)]

if __name__=='__main__':
    import argparse
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);args=p.parse_args()
    if args.output.exists():p.error('Choose a new output directory')
    args.output.mkdir(parents=True)
    for n,b in assets().items():(args.output/n).write_bytes(b)
