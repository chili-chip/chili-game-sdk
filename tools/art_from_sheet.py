"""Put a PNG sprite sheet into a .rpg game file as "art", in Sweetie 16.

Usage: python tools/art_from_sheet.py GAME.rpg SHEET.png OUT.rpg

Each 8x8 cell of the sheet, left to right and top to bottom, becomes one
picture, so sprite numbers stay the same. Each pixel becomes the nearest
Sweetie 16 colour; pixels under half alpha become clear. The game's
"sprites" line is replaced by the "art" block, and "background" moves to the
nearest Sweetie 16 colour. The rest of the file is kept as it is.

Needs Pillow (pip install pillow).
"""

import re
import sys
from pathlib import Path

from PIL import Image

SWEETIE16 = [
    0x1A1C2C, 0x5D275D, 0xB13E53, 0xEF7D57, 0xFFCD75, 0xA7F070, 0x38B764, 0x257179,
    0x29366F, 0x3B5DC9, 0x41A6F6, 0x73EFF7, 0xF4F4F4, 0x94B0C2, 0x566C86, 0x333C57,
]
RGB = [((c >> 16) & 255, (c >> 8) & 255, c & 255) for c in SWEETIE16]


def nearest(r: int, g: int, b: int) -> int:
    # Weighted RGB distance: close enough for picking from 16 colours.
    def dist(c):
        return 2 * (r - c[0]) ** 2 + 4 * (g - c[1]) ** 2 + 3 * (b - c[2]) ** 2
    return min(range(16), key=lambda i: dist(RGB[i]))


def pictures(sheet: Image.Image) -> list[list[str]]:
    sheet = sheet.convert("RGBA")
    cols, rows = sheet.width // 8, sheet.height // 8
    out = []
    for n in range(min(cols * rows, 256)):
        x0, y0 = (n % cols) * 8, (n // cols) * 8
        pic = []
        for y in range(8):
            row = ""
            for x in range(8):
                r, g, b, a = sheet.getpixel((x0 + x, y0 + y))
                row += "." if a < 128 else "0123456789abcdef"[nearest(r, g, b)]
            pic.append(row)
        out.append(pic)
    # Drop clear pictures at the end.
    while out and all(row == "........" for row in out[-1]):
        out.pop()
    return out


def art_block(pics: list[list[str]], indent: str) -> str:
    lines = [f'{indent}"art": {{', f'{indent}  "palette": "sweetie16",', f'{indent}  "pictures": [']
    for i, pic in enumerate(pics):
        comma = "," if i < len(pics) - 1 else ""
        lines.append(f'{indent}    [' + ", ".join(f'"{row}"' for row in pic) + f"]{comma}")
    lines += [f"{indent}  ]", f"{indent}}},"]
    return "\n".join(lines)


def main(game_path: str, sheet_path: str, out_path: str) -> int:
    text = Path(game_path).read_text()
    pics = pictures(Image.open(sheet_path))
    m = re.search(r'^(\s*)"sprites":\s*"[^"]*",\s*$', text, re.M)
    if not m:
        print(f'{game_path}: no "sprites" line to replace')
        return 1
    text = text[: m.start()] + art_block(pics, m.group(1)) + text[m.end():]

    def background(bg):
        c = int(bg.group(1), 16)
        i = nearest(c >> 16, (c >> 8) & 255, c & 255)
        return f'"background": "#{SWEETIE16[i]:06x}"'

    text = re.sub(r'"background":\s*"#([0-9a-fA-F]{6})"', background, text, count=1)
    Path(out_path).write_text(text)
    print(f"{out_path}: {len(pics)} pictures")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(*sys.argv[1:]))
