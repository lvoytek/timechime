#!/usr/bin/env python3
# /// script
# dependencies = ["Pillow"]
# ///
"""Convert a BMP file to lv_image_dsc_t C source matching sprites.c formatting."""

import argparse
import math
import sys
from pathlib import Path

from PIL import Image

# Characters after the leading tab before the // comment
_COMMENT_COL = 48


def _hex_row(data: bytes) -> str:
    """Format bytes as C array elements with a trailing comma."""
    return ", ".join(f"0x{b:02X}" for b in data) + ","


def _padded(content: str) -> str:
    """Pad content to _COMMENT_COL chars; always leaves at least one trailing space."""
    return content.ljust(_COMMENT_COL) if len(content) < _COMMENT_COL else content + " "


def convert(img: Image.Image, name: str, threshold: int) -> str:
    """Return C source text for a 1-bit lv_image_dsc_t sprite."""
    w, h = img.size
    stride = math.ceil(w / 8)
    gray = img.convert("L")

    row_num_width = len(str(h - 1))

    # BGRA palette: palette[0]=white background, palette[1]=black foreground
    palette = bytes([0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF])
    lines = [f"static const uint8_t {name}_data[] = {{"]
    lines.append(f"\t{_padded(_hex_row(palette))}// palette: white, black")

    for y in range(h):
        acc = 0
        set_px = 0
        for x in range(w):
            if gray.getpixel((x, y)) < threshold:
                acc |= 1 << (stride * 8 - 1 - x)
                set_px += 1
        row_bytes = acc.to_bytes(stride, byteorder="big")
        desc = f"({set_px}px)" if set_px else "empty"
        comment = f"// row {y:{row_num_width}d}: {desc}"
        lines.append(f"\t{_padded(_hex_row(row_bytes))}{comment}")

    lines += [
        "};",
        "",
        f"const lv_image_dsc_t {name} = {{",
        f"\t.header = {{.magic = LV_IMAGE_HEADER_MAGIC,",
        f"\t\t   .cf = LV_COLOR_FORMAT_I1,",
        f"\t\t   .flags = 0,",
        f"\t\t   .w = {w},",
        f"\t\t   .h = {h},",
        f"\t\t   .stride = {stride}}},",
        f"\t.data_size = sizeof({name}_data),",
        f"\t.data = {name}_data,",
        "};",
    ]

    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert a BMP to lv_image_dsc_t C source."
    )
    parser.add_argument("bmp", help="Input BMP (or other Pillow-supported image) file")
    parser.add_argument(
        "-n", "--name",
        help="Sprite variable name (default: sprite_<stem>)",
    )
    parser.add_argument(
        "-o", "--output",
        help="Output file (default: stdout)",
    )
    parser.add_argument(
        "-t", "--threshold",
        type=int,
        default=128,
        metavar="N",
        help="Pixels with grayscale < N become foreground/black (default: 128)",
    )
    args = parser.parse_args()

    bmp_path = Path(args.bmp)
    if not bmp_path.exists():
        sys.exit(f"Error: file not found: {bmp_path}")

    name = args.name or f"sprite_{bmp_path.stem}"

    try:
        img = Image.open(bmp_path)
    except Exception as e:
        sys.exit(f"Error opening image: {e}")

    src = convert(img, name, args.threshold)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(src)
        print(f"Written to {out_path}", file=sys.stderr)
    else:
        sys.stdout.write(src)

    print(f"\nAdd to sprites.h:\n    extern const lv_image_dsc_t {name};", file=sys.stderr)


if __name__ == "__main__":
    main()
