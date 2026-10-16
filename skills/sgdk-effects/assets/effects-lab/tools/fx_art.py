"""Original native-resolution indexed art; no imaging dependencies."""
import struct
import zlib

PALETTE=[(0,0,0),(36,36,73),(36,73,146),(73,182,219),(255,255,255),
         (255,219,109),(255,146,36),(219,73,36),(109,73,146),(73,73,109),
         (146,146,182),(182,182,219),(36,109,109),(73,219,182),(219,182,109),(255,255,219)]

def png(width, height, pixels):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + bytes(pixels[y * width:(y + 1) * width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0))
            + chunk(b'PLTE', bytes(c for rgb in PALETTE for c in rgb))
            + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def assets():
    shapes=[
        ["00000000","00055000","00054000","05544550","00545000","00050000","00000000","00000000"],
        ["00000000","00000000","00060000","00656000","00060000","00000000","00000000","00000000"],
        ["00000000","00099000","009aa900","09aaab90","09aaab90","009aa900","00099000","00000000"],
        ["00909000","090a0a00","90a0a090","0a0b0a00","9a0a0a90","090a0900","00909000","00000000"],
        ["00000000","00002000","00003000","00030000","00030000","00300000","00400000","00000000"],
        ["00000000","00003000","00343000","00043000","03343300","00030000","00000000","00000000"],
        ["00000000","00020000","00232000","02343200","00232000","00020000","00000000","00000000"],
        ["00000000","00000000","00020000","00232000","00020000","00000000","00000000","00000000"],
    ]
    pixels=[int(shapes[f][y][x],16) for y in range(8) for f in range(8) for x in range(8)]
    hero=[0]*(64*16)
    for f in range(4):
        for y in range(16):
            for x in range(16):
                c=0
                if 4<=x<=11 and 3<=y<=12:
                    c=14 if x in (4,11) or y in (3,12) else 6
                    if 6<=x<=9 and 5<=y<=10:c=15 if (x+y+f)%3 else 5
                if y in (1,2) and 6<=x<=9:c=14
                if y in (13,14) and 6<=x<=9:c=12
                if y==7+(f&1) and (1<=x<=3 or 12<=x<=14):c=13
                if y==8+(f&1) and x in (0,15):c=3
                hero[y*64+f*16+x]=c
    return {'fx.png':png(64,8,pixels),'lantern.png':png(64,16,hero)}
