"""Original integration fixture; install the real skills before preparing it."""
import json
from pathlib import Path
import shutil
import struct
import zlib

PALETTE=[(0,0,0),(1,2,2),(2,3,3),(4,5,5),(1,2,5),(2,4,7),(5,6,7),(7,5,1),
         (7,7,3),(3,2,1),(5,3,2),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(7,7,7)]
def png(width,height,pixels):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+bytes(pixels[y*width:(y+1)*width])for y in range(height))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0))+
            chunk(b'PLTE',bytes(round(v*255/7)for color in PALETTE for v in color))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
def assets():
    tiles=[]
    for y in range(8):
        for kind in range(5):
            for x in range(8):
                if kind==0:v=1
                elif kind==1:v=3 if y==0 or x==0 else 2
                elif kind==2:v=9 if x%3 else 7
                elif kind==3:v=8 if 1<=x<=6 and 1<=y<=6 else 1
                else:v=7 if x in (0,7) or y in (0,7) else 1
                tiles.append(v)
    actor=[]
    for y in range(16):
        for x in range(16):
            v=0
            if 4<=x<=11 and 2<=y<=7:v=8 if y<4 else 10
            if 3<=x<=12 and 8<=y<=12:v=5 if x<8 else 4
            if y>=13 and x in (4,5,10,11):v=6
            if y==5 and x in (6,9):v=15
            actor.append(v)
    return {'court.png':png(40,8,tiles),'courier.png':png(16,16,actor)}
def populate(project):
    """Only modify the fresh, SDK-initialized evaluation project we own."""
    project=Path(project)
    if not (project/'src/main.c').is_file() or (project/'inc/model.h').exists():
        raise ValueError('Expected an unused SGDK initializer output')
    source=Path(__file__).parent
    for folder in ('inc','res'): (project/folder).mkdir(exist_ok=True)
    for name in ('main.c','model.c'):shutil.copyfile(source/name,project/'src'/name)
    for name in ('model.h','controls.h'):shutil.copyfile(source/name,project/'inc'/name)
    for name,data in assets().items():(project/'res'/name).write_bytes(data)
    # IMAGE supplies both tileset and palette; five unique tiles retain source order.
    (project/'res/resources.res').write_text('IMAGE court "court.png" NONE NONE\nSPRITE courier "courier.png" 2 2 NONE 0\n')
    (project/'Makefile').write_text('include $(GDK)/makefile.gen\n')
    budget={'schema_version':1,'video_mode':'H40',
            'ram':{'items':[{'name':'SDK, game state and stack','bytes':None}],'complete':False},
            'vram':{'items':[{'name':'Sprite pool reservation','tiles':16},
                             {'name':'Resident court art','tiles':5},
                             {'name':'SDK tables and font','bytes':None}],'complete':False},
            'palette_banks':2,'sprites':{'total':1,'per_scanline':1,'pixels_per_scanline':16},
            'rom':{'bytes_before_padding':None,'mapping':'standard'},
            'dma':{'peak_bytes':None,'budget_bytes':None}}
    (project/'budget.json').write_text(json.dumps(budget,indent=2)+'\n')
    (project/'README.txt').write_text('Courier composition check\nD-pad: move. Start: pause. A: deliver.\nCollect the gold parcel in room one, walk right through the gate, then cross room two through its lower opening and deliver on its gold seal.\n')
    (project/'LICENSE.txt').write_text('Original test code and diagnostic art: MIT. See repository LICENSE for the full license.\n')
    return project
