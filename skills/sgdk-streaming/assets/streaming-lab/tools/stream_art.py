"""Original 32x32 indexed scenery and a 16x16 courier, drawn at native size."""
import struct
import zlib

# RGB3 values, snapped before authoring rather than quantizing finished art.
COLORS=[
 [(0,0,0),(1,2,2),(1,3,3),(2,4,3),(3,5,3),(4,6,4),(1,2,1),(2,3,1),(3,4,2),(4,3,2),(5,4,3),(6,5,3),(7,6,4),(7,7,6),(3,5,5),(2,3,4)],
 [(0,0,0),(1,1,2),(2,2,3),(3,3,4),(4,4,5),(5,5,5),(2,1,1),(3,2,1),(4,3,2),(5,3,1),(6,4,1),(7,5,2),(7,6,3),(7,7,5),(3,4,5),(4,2,2)],
 [(0,0,0),(1,2,3),(1,3,4),(2,4,5),(3,5,6),(4,6,6),(1,2,2),(2,3,3),(3,4,4),(4,3,2),(5,4,3),(6,5,3),(7,6,4),(7,7,6),(3,5,4),(2,3,5)],
 [(0,0,0),(1,1,2),(2,2,3),(3,3,4),(4,4,5),(5,5,6),(1,1,2),(2,2,3),(3,3,4),(3,2,3),(4,3,4),(5,4,5),(6,5,7),(7,7,7),(3,4,5),(2,2,4)],
]
HERO=[(0,0,0),(1,1,2),(2,2,3),(3,4,5),(4,6,7),(6,7,7),(3,2,1),(5,3,2),
      (7,5,3),(7,6,4),(2,3,2),(3,5,3),(4,6,4),(7,7,6),(5,4,2),(6,5,3)]

def png(w,h,pixels,palette):
    def chunk(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d))
    levels=[0,36,73,109,146,182,219,255]
    rgb=bytes(levels[c] for color in palette for c in color)
    data=b''.join(b'\0'+bytes(pixels[y*w:(y+1)*w]) for y in range(h))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,3,0,0,0))+chunk(b'PLTE',rgb)+chunk(b'IDAT',zlib.compress(data,9))+chunk(b'IEND',b'')

def tiles(pixels,w,h,column_first=False):
    coordinates=[(x,y) for x in range(w//8) for y in range(h//8)] if column_first else [(x,y) for y in range(h//8) for x in range(w//8)]
    out=bytearray()
    for tx,ty in coordinates:
        for y in range(8):
            row=pixels[(ty*8+y)*w+tx*8:(ty*8+y)*w+tx*8+8]
            out.extend((row[x]<<4)|row[x+1] for x in range(0,8,2))
    return bytes(out)

def page(zone,kind):
    p=[1]*1024
    def rect(x,y,w,h,c):
        for yy in range(max(0,y),min(32,y+h)):
            for xx in range(max(0,x),min(32,x+w)):p[yy*32+xx]=c
    if kind==0: # Walkway: clean stone edges and restrained seams.
        rect(0,0,32,3,11);rect(0,3,32,2,9);rect(0,5,32,27,7)
        for y in range(8,32,8):
            rect(0,y,32,1,6)
            for x in range(((y//8&1)*8+zone*3)%16,32,16):rect(x,y+1,1,7,6);rect(x+2,y+2,7,1,8)
    elif kind==1: # A few stars / wisps, not a noisy wallpaper.
        rect(0,0,32,32,1)
        if zone in (0,3):rect(7+zone*2,7,1,1,12);rect(24-zone,13+zone,1,1,5)
        else:rect(4+zone,12,14-zone,1,3);rect(8,10+zone,8+zone,1,3)
    elif kind==2: # Distant hedge, pipes, sea wall or ruined fence.
        rect(0,0,32,32,1);rect(0,17,32,15,6)
        for x in range(0,32,8):rect(x,9+(x%3),6-zone,23,7);rect(x,9+(x%3),6-zone,2,8)
        rect(0,28,32,2,9)
    elif kind==3: # Lower landmark.
        rect(0,0,32,32,1)
        if zone==0:
            rect(12,0,9,32,6);rect(14,0,5,32,9);rect(14,0,1,32,10)
            rect(7,28,19,4,6)
        else:
            rect(3,0,26,32,6);rect(5,0,22,32,8);rect(6,0,2,32,10)
            rect(11,8,11,21,6);rect(13,10,7,17,zone==3 and 12 or 11)
            rect(13,14+zone,7,2,7);rect(15+zone%2,10,1,17,7)
            for y in (4,28):rect(3,y,26,2,9)
    elif kind==4: # Tree canopy / roof / crane top / broken arch.
        rect(0,0,32,32,1)
        if zone==0:
            for y in range(4,30):
                inset=abs(16-y)//2+2
                rect(inset,y,32-2*inset,1,6 if y>23 else 3)
                if y<18:rect(inset+2,y,32-2*inset-4,1,4)
            rect(9,9,13,2,5)
        else:
            for y in range(6,22):rect(max(0,16-y),y,min(32,2*y),1,6 if y%4==0 else 9)
            rect(2,22,28,4,10);rect(5,26,22,6,8)
            if zone==2:rect(18,2,3,20,10);rect(7,2,21,2,11)
            if zone==3:rect(18,4,5,13,2)
    else: # Small lamp and stone plinth.
        rect(0,0,32,32,1);rect(0,27,32,5,7)
        rect(14,9,3,19,6);rect(11,8,9,2,6);rect(12,10,7,9,10)
        rect(13+zone%3,11,3,6,13);rect(9+zone,19,11-zone,2,6);rect(10,28,12,3,9)
    return p

def hero(frame):
    rows=["0000000000000000","0000001111000000","000001eeef100000","00001eefff110000",
          "0000118888100000","0000018988100000","0000018888100000","0000001661000000",
          "0000114444110000","0001645444510000","0001684444810000","0000114444110000",
          "0000016666100000","0000016626100000","0000012002100000","0000000000000000"]
    p=[int(c,16) for row in rows for c in row]
    if frame:
        for x in range(5,11):p[14*16+x]=0
        for x in (4,5,10,11):p[14*16+x]=2
    return p

def assets():
    out={};raw=bytearray()
    for z in range(4):
        sheet=[0]*(192*32)
        for k in range(6):
            pixels=page(z,k);raw.extend(tiles(pixels,32,32))
            for y in range(32):sheet[y*192+k*32:y*192+k*32+32]=pixels[y*32:y*32+32]
        out[f'area-{z}.png']=png(192,32,sheet,COLORS[z])
    out['pages.bin']=bytes(raw)
    out['hero.bin']=b''.join(tiles(hero(f),16,16,True) for f in range(2))
    out['hero.png']=png(16,16,hero(0),HERO)
    def words(colors):return ','.join('0x%04X'%((b<<9)|(g<<5)|(r<<1)) for r,g,b in colors)
    out['palettes.h']=('/* Native RGB3 authoring palettes; ROM-resident metadata. */\n'
        'static const u16 area_colors[4][16]={\n'+',\n'.join('{'+words(c)+'}' for c in COLORS)+'};\n'
        'static const u16 hero_colors[16]={'+words(HERO)+'};\n').encode()
    return out
