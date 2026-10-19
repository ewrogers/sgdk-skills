"""Original native tile icons and titles for the five diagnostic pages."""
from hardware_glyphs import rows
from readiness_art import tile_words

TITLES = ('VIDEO LAB', 'SPRITE LAB', 'DMA / AUDIO', 'INPUT LAB', 'SAVE LAB')


def icons():
    result = []
    for page in range(5):
        p = [[1] * 32 for _ in range(32)]
        def box(x, y, w, h, color):
            for yy in range(y, y+h):
                for xx in range(x, x+w):
                    p[yy][xx] = color
        if page == 0:
            box(2, 3, 28, 22, 5); box(4, 5, 24, 18, 2)
            for x, color in ((6, 4), (13, 15), (20, 5)):
                box(x, 7, 6, 14, color)
            box(13, 25, 6, 3, 3); box(8, 28, 16, 2, 5)
        elif page == 1:
            for x, y, color in ((12, 3, 3), (7, 8, 5), (2, 13, 4)):
                box(x, y, 18, 17, color); box(x+2, y+2, 14, 13, 2)
                box(x+5, y+5, 3, 3, 15); box(x+11, y+5, 3, 3, 15)
                box(x+6, y+10, 7, 2, color)
        elif page == 2:
            box(1, 5, 30, 23, 3); box(3, 7, 26, 19, 2)
            for x in range(5, 28):
                p[22][x] = 3
                p[10 + (abs((x-5) % 12 - 6) * 2)][x] = 4
                p[11 + (abs((x-5) % 12 - 6) * 2)][x] = 4
            box(8, 1, 3, 4, 5); box(21, 1, 3, 4, 5)
        elif page == 3:
            box(4, 9, 24, 15, 5); box(2, 13, 28, 14, 5)
            box(5, 12, 22, 12, 2); box(8, 14, 3, 8, 15)
            box(5, 17, 9, 2, 15)
            for x, y in ((20, 17), (24, 14), (24, 21)):
                box(x, y, 3, 3, 4)
            box(13, 4, 3, 5, 3); box(13, 3, 10, 2, 3)
        else:
            box(5, 2, 22, 27, 5); box(8, 5, 16, 19, 2)
            box(11, 8, 10, 9, 3); box(14, 8, 5, 6, 4)
            for x in (9, 13, 17, 21): box(x, 26, 2, 4, 15)
            box(11, 20, 10, 2, 15)
        result += tile_words(p)
    return result


def artwork():
    words = icons()
    offsets, widths = [], []
    for title in TITLES:
        width = (len(title)*12+7)//8
        offsets.append(len(words)//8); widths.append(width)
        p = [[1] * (width*8) for _ in range(16)]
        for i, char in enumerate(title):
            for y, bits in enumerate(rows(char)[:7]):
                for x in range(5):
                    if bits & (1 << (4-x)):
                        for dy in (0, 1):
                            for dx in (0, 1): p[y*2+dy][i*12+x*2+dx] = 15
        words += tile_words(p)
    return words, offsets, widths


def header():
    words, offsets, widths = artwork()
    return ('/* Original diagnostic icons and 2x titles. */\n'
            f'#define HL_LAB_TILE_COUNT {len(words)//8}\n'
            'static const u16 hl_lab_title_offsets[5] = {' + ','.join(map(str, offsets)) + '};\n'
            'static const u16 hl_lab_title_widths[5] = {' + ','.join(map(str, widths)) + '};\n'
            'static const u32 hl_lab_tiles[HL_LAB_TILE_COUNT * 8] = {\n' +
            ',\n'.join(f'0x{word:08x}' for word in words) + '\n};\n')
