"""Original native 4bpp tiles for Hardware Lab's readiness page."""
from hardware_glyphs import rows


def tile_words(pixels):
    height, width = len(pixels), len(pixels[0])
    words = []
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(ty, ty + 8):
                words.append(sum(pixels[y][tx + x] << (28 - 4 * x) for x in range(8)))
    return words


def artwork():
    chip = [[1] * 32 for _ in range(32)]
    for pin in (8, 13, 18, 23):
        for p in range(pin, pin + 2):
            for q in range(1, 6):
                chip[p][q] = chip[p][31 - q] = 5
                chip[q][p] = chip[31 - q][p] = 5
    for y in range(6, 26):
        for x in range(6, 26):
            chip[y][x] = 4 if x in (6, 25) or y in (6, 25) else 2
    for y in range(10, 22):
        for x in range(10, 22):
            chip[y][x] = 1
    for x, y in [(11 + i, 16 + i) for i in range(5)] + [(15 + i, 20 - i) for i in range(8)]:
        for dx, dy in ((0, 0), (1, 0), (0, 1)):
            chip[y + dy][x + dx] = 15

    title = [[1] * 144 for _ in range(16)]
    for index, char in enumerate("HARDWARE LAB"):
        for y, bits in enumerate(rows(char)[:7]):
            for x in range(5):
                if bits & (1 << (4 - x)):
                    for dy in (0, 1):
                        for dx in (0, 1):
                            title[y * 2 + dy][index * 12 + x * 2 + dx] = 15

    words = tile_words(chip) + tile_words(title)
    corner = [[2 if x >= 4 or y >= 4 or (x - 4) ** 2 + (y - 4) ** 2 <= 16 else 1
               for x in range(8)] for y in range(8)]
    for flip_x, flip_y in ((False, False), (True, False), (False, True), (True, True)):
        pixels = [list(reversed(row)) if flip_x else row[:] for row in corner]
        if flip_y:
            pixels.reverse()
        words += tile_words(pixels)
    for color in (3, 4, 5):
        words += tile_words([[color if x < 6 and 2 <= y <= 5 else 2 for x in range(8)]
                             for y in range(8)])
    return words


def header():
    words = artwork()
    return ("/* Original 4bpp chip, 2x title, card corners and heartbeat segments. */\n"
            "#define HL_READY_TILE_COUNT 59\n"
            "#define HL_READY_TITLE_OFFSET 16\n"
            "#define HL_READY_CORNER_OFFSET 52\n"
            "#define HL_READY_METER_OFFSET 56\n"
            "static const u32 hl_ready_tiles[HL_READY_TILE_COUNT * 8] = {\n" +
            ",\n".join(f"0x{word:08x}" for word in words) + "\n};\n")
