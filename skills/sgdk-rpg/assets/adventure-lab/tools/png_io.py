"""Small bounded PNG reader (8-bit RGB/RGBA/indexed) and exact indexed writer."""
import struct
import zlib

SIGNATURE = b'\x89PNG\r\n\x1a\n'
MAX_PIXELS = 4_194_304


def chunk(kind, payload):
    return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload))


def read_png(path):
    data = path.read_bytes()
    if len(data) > 32_000_000 or data[:8] != SIGNATURE:
        raise ValueError('Expected PNG up to 32 MB')
    offset = 8
    chunks = {}
    ended = False
    while offset < len(data):
        if offset + 12 > len(data): raise ValueError('Truncated PNG chunk')
        size = struct.unpack_from('>I', data, offset)[0]
        end = offset + 12 + size
        if end > len(data): raise ValueError('Truncated PNG payload')
        kind = data[offset+4:offset+8]
        payload = data[offset+8:end-4]
        if zlib.crc32(kind + payload) != struct.unpack_from('>I', data, end-4)[0]:
            raise ValueError('PNG checksum mismatch')
        if not chunks and kind != b'IHDR': raise ValueError('PNG must begin with IHDR')
        if kind in (b'IHDR', b'PLTE', b'tRNS') and kind in chunks:
            raise ValueError('Duplicate PNG metadata')
        if kind not in (b'IHDR', b'PLTE', b'IDAT', b'IEND') and not kind[0] & 32 and kind != b'tRNS':
            raise ValueError('Unsupported critical PNG chunk')
        chunks.setdefault(kind, bytearray()).extend(payload)
        offset = end
        if kind == b'IEND':
            if size or offset != len(data): raise ValueError('Invalid PNG end')
            ended = True
            break
    if not ended or b'IDAT' not in chunks or len(chunks[b'IHDR']) != 13:
        raise ValueError('Missing PNG structure')
    w, h, depth, mode, compression, filtering, interlace = struct.unpack('>IIBBBBB', chunks[b'IHDR'])
    if not w or not h or w*h > MAX_PIXELS: raise ValueError('PNG dimensions exceed safety limit')
    if depth != 8 or mode not in (2,3,6) or interlace or compression or filtering:
        raise ValueError('Use non-interlaced 8-bit RGB, RGBA or indexed PNG; export without resizing')
    bpp = {2:3, 3:1, 6:4}[mode]
    stride = w*bpp
    expected = (stride+1)*h
    decoder = zlib.decompressobj()
    raw = decoder.decompress(chunks[b'IDAT'], expected+1)
    if len(raw) != expected or not decoder.eof or decoder.unused_data:
        raise ValueError('Invalid PNG decompressed size')
    palette = chunks.get(b'PLTE', b'')
    alpha = chunks.get(b'tRNS', b'')
    if mode == 3 and (not palette or len(palette) % 3 or len(palette) > 768 or len(alpha) > len(palette)//3):
        raise ValueError('Invalid indexed palette')
    if mode == 2 and alpha and len(alpha) != 6: raise ValueError('Invalid RGB transparency key')
    if mode == 6 and alpha: raise ValueError('RGBA PNG cannot have tRNS')
    rgb_key = struct.unpack('>HHH', alpha) if mode == 2 and alpha else None
    prior = bytearray(stride)
    pixels = []
    cursor = 0
    for _ in range(h):
        kind = raw[cursor]
        cursor += 1
        row = bytearray(raw[cursor:cursor+stride]); cursor += stride
        if kind > 4: raise ValueError('Invalid PNG filter')
        for x in range(stride):
            a = row[x-bpp] if x >= bpp else 0
            b = prior[x]
            c = prior[x-bpp] if x >= bpp else 0
            estimate = a+b-c
            distances = (abs(estimate-a), abs(estimate-b), abs(estimate-c))
            paeth = (a,b,c)[distances.index(min(distances))]
            predict = (0,a,b,(a+b)//2,paeth)[kind]
            row[x] = (row[x]+predict) & 255
        for x in range(w):
            at = x*bpp
            if mode == 3:
                index = row[at]
                if index*3+3 > len(palette): raise ValueError('Palette index out of range')
                pixels.append((*palette[index*3:index*3+3], alpha[index] if index < len(alpha) else 255))
            else:
                rgb = tuple(row[at:at+3])
                pixels.append((*rgb, row[at+3] if mode == 6 else 0 if rgb_key == rgb else 255))
        prior = row
    return w, h, pixels


def indexed_png(width, height, indices, colors, transparent=False):
    if len(indices) != width*height or not 1 <= len(colors) <= 256:
        raise ValueError('Invalid indexed image shape')
    if any(type(i) is not int or not 0 <= i < len(colors) for i in indices):
        raise ValueError('Invalid output palette index')
    rows = b''.join(b'\0'+bytes(indices[y*width:(y+1)*width]) for y in range(height))
    palette = b''.join(bytes(color) for color in colors)
    return (SIGNATURE + chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,3,0,0,0)) + chunk(b'PLTE',palette)
            + (chunk(b'tRNS',b'\0'+b'\xff'*(len(colors)-1)) if transparent else b'')
            + chunk(b'IDAT',zlib.compress(rows,9)) + chunk(b'IEND',b''))
