#!/usr/bin/env python3

from PIL import Image
from contextlib import redirect_stdout
import sys


if __name__ == '__main__':
    image = Image.open(sys.argv[1])
    image = image.convert('P')
    pix = image.load()

    gradient = []
    prevCol = -1

    for y in range(1, image.size[1]):
        col = pix[0, y]
        if col == prevCol:
            continue
        gradient.append((y, col))
        prevCol = col

    with open(sys.argv[2], 'w') as f:
        with redirect_stdout(f):
            print('#define anemone_gradient_length %d\n' % len(gradient))

            print('static const u_char anemone_gradient_y[] = {')
            for y, _ in gradient:
                print('  %d,' % y)
            print('};\n')

            print('static const u_char anemone_gradient_color[] = {')
            for _, col in gradient:
                print('  %d,' % col)
            print('};\n')
