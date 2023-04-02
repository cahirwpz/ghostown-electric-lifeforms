from math import ceil, log
from PIL import Image
from collections import namedtuple
from itertools import product
import argparse

# Converts image with electron positions to C file
# electron head needs to have palette index 3
# electron tail needs to have palette index 5
# background can have palette index 0 or 1

HEAD_COLOR_IDX = 3
TAIL_COLOR_IDX = 5

header = "#include <gfx.h>\n\n\
#ifndef ELECTRONS_T\n\
#define ELECTRONS_T\n\
typedef struct ElectronT {\n\
  Point2D head;\n\
  Point2D tail;\n\
} ElectronT;\n\
\n\
typedef struct ElectronArrayT {\n\
  const int num_electrons;\n\
  const ElectronT points[0];\n\
} ElectronArrayT;\n\
#endif\n\n"

Electron = namedtuple('Electron', ['head', 'tail'])


def pixels_around(x, y):
    for i in range(-1, 2):
        for j in range(-1, 2):
            yield x+i, y+j


def get_positions(infile):
    with Image.open(infile) as im:
        if im.mode != 'P':
            raise SystemExit('Only palette images supported.')

        width, height = im.size

        px = im.load()
        electrons = []
        for hx, hy in product(range(width), range(height)):
            if px[hx, hy] == HEAD_COLOR_IDX:
                for tx, ty in pixels_around(hx, hy):
                    if px[tx, ty] == TAIL_COLOR_IDX:
                        electrons.append(Electron((hx, hy), (tx, ty)))
                        break

        return electrons


def generate_array(electrons, name):
    res = ""
    res += "static const __data ElectronArrayT {} = {{\n".format(name)
    res += "  .num_electrons = {},\n".format(len(electrons))
    res += "  .points = {\n"
    for head, tail in electrons:
        hx, hy = head
        tx, ty = tail
        res += "    {{ .head = {{ .x = {}, .y = {} }},".format(hx, hy)
        res += ".tail = {{ .x = {}, .y = {} }} }},\n".format(tx, ty)
    res += "  }\n"
    res += "};\n\n"
    return res


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Converts 2-bit PNG with electron positions to C file."
    )
    parser.add_argument("infile", help="Input PNG file")
    parser.add_argument("outfile", help="Output C file")
    parser.add_argument("--name", default="electrons",
                        help="Name of the array with electron coordinates")
    args = parser.parse_args()

    electrons = get_positions(args.infile)

    with open(args.outfile, "w") as out:
        out.write(header)
        out.write(generate_array(electrons, args.name))
