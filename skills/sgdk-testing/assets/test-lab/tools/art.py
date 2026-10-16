"""Original native-resolution Test Lab art, generated without image libraries."""
import struct
import zlib


def png(width, height, pixels, palette):
    def chunk(name, data):
        return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name + data))
    rows = b''.join(b'\0' + bytes(pixels[y * width:(y + 1) * width]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)) +
            chunk(b'PLTE', bytes(c for rgb in palette for c in rgb)) + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def assets():
    palette = [(0, 0, 0), (0, 36, 72), (0, 72, 144), (36, 144, 216),
               (108, 216, 252), (252, 252, 216), (252, 180, 36), (180, 72, 0),
               (36, 72, 108), (72, 144, 144), (108, 216, 144), (216, 252, 216),
               (144, 36, 72), (216, 72, 108), (252, 144, 144), (252, 252, 252)]
    player = []
    pattern = [
        '0000000000000000', '0000011111100000', '0000122222210000', '0001233333321000',
        '0001244444421000', '0001255555521000', '0001222222221000', '0000111111110000',
        '0001772772771000', '0017662662667100', '0011662662661100', '0001222222221000',
        '0000122222210000', '0000188008810000', '0000188008810000', '0000111001110000']
    for y in range(16):
        for frame in range(2):
            for x in range(16):
                value = int(pattern[y][x])
                if frame and y == 14 and value: value = 0
                if frame and y == 15 and x in (4, 5, 9, 10): value = 3
                player.append(value)
    coin = []
    bolt = []
    for y in range(8):
        for frame in range(3):
            for x in range(8):
                d = abs(x - 3) + abs(y - 3)
                coin.append(0 if d > 3 else (7 if d == 3 else (15 if x == 2 + frame else 6)))
                bolt.append(0 if not (2 <= y <= 5 and 1 <= x <= 6) else (15 if y in (3, 4) else 4))
    return {'courier.png': png(32, 16, player, palette),
            'mote.png': png(24, 8, coin, palette), 'bolt.png': png(24, 8, bolt, palette)}
