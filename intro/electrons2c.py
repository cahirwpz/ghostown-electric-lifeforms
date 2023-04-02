#!/usr/bin/env python3

from math import ceil, log
from PIL import Image
from collections import namedtuple
from contextlib import redirect_stdout
from itertools import product
import argparse

# Converts image with electron positions to C file
# electron head needs to have palette index 3
# electron tail needs to have palette index 5
# background can have palette index 0 or 1

HEAD_COLOR_IDX = 3
TAIL_COLOR_IDX = 5

Electron = namedtuple('Electron', ['head', 'tail'])


def pixels_around(x, y):
    for i in range(-1, 2):
        for j in range(-1, 2):
            yield x+i, y+j


def get_positions(infile):
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
    print('#ifndef ELECTRONS_T')
    print('#define ELECTRONS_T')
    print('typedef struct ElectronArray {')
    print('  short num_electrons;')
    print('  short points[0];')
    print('} ElectronArrayT;')
    print('#endif\n')

    print('static const __data ElectronArrayT %s = {' % name)
    print('  .num_electrons = %d,' % len(electrons))
    print('  .points = {')
    for head, tail in electrons:
        hx, hy = head
        tx, ty = tail
        print(f'    ELPOS({hx}, {hy}), ELPOS({tx-hx}, {ty-hy}),')
    print('  }')
    print('};')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Converts 2-bit PNG with electron positions to C file."
    )
    parser.add_argument("infile", help="Input PNG file")
    parser.add_argument("outfile", help="Output C file")
    parser.add_argument("--name", default="electrons",
                        help="Name of the array with electron coordinates")
    args = parser.parse_args()

    with Image.open(args.infile) as im:
        if im.mode != 'P':
            raise SystemExit('Only palette images supported.')
        electrons = get_positions(im)

    with open(args.outfile, "w") as f:
        with redirect_stdout(f):
            generate_array(electrons, args.name)
