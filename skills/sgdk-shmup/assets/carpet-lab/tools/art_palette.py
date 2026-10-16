"""Fixed VDP-compatible color plans used for Carpet Lab's native art."""
import struct
import zlib

ACTORS=['000000','240024','492449','6d2449','b64949','db926d','ffdb92','ffffdb',
        '242449','24496d','246d92','49b6b6','92dbdb','6d4924','b69249','ffdb49']
FX=['000000','240024','492449','922449','db4949','ff926d','ffdb92','ffffdb',
    '244949','24926d','92db92','ffff49','244992','4992db','92dbff','ffffff']
FAR=[
 ['000000','242449','492449','6d496d','b66d6d','db926d','ffb66d','ffdb92','ffffdb','24496d','496d92','6d92b6','92b6db','b6dbff','dbdbff','ffffff'],
 ['000000','242449','49246d','6d4992','926db6','b692db','dbb6ff','ffdbff','ffffdb','244949','246d6d','499292','6db6b6','92dbb6','b6dbdb','ffffff'],
 ['000000','242449','49496d','6d6d92','9292b6','b6b6db','dbdbff','ffffff','49246d','6d4992','926db6','b692db','dbb6ff','b69249','dbb66d','ffdb92']]
NEAR=[
 ['000000','240024','492424','6d4924','926d49','b6926d','dbb692','ffdbb6','ffffdb','244949','246d6d','49926d','92b692','b6dbb6','b64949','db926d'],
 ['000000','240024','492449','6d496d','926d92','b692b6','dbb6db','ffdbff','ffffdb','244949','246d49','49926d','6db692','92dbb6','b69249','ffdb92'],
 ['000000','240024','242449','49496d','6d6d92','9292b6','b6b6db','dbdbff','ffffdb','492449','6d496d','926d92','b69249','dbb66d','ffdb92','ffffff']]

def palette_png(colors):
    def chunk(kind,data): return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',16,1,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b'\0'+b''.join(bytes.fromhex(c) for c in colors)))+chunk(b'IEND',b'')
