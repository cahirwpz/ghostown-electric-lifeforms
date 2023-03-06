#!/usr/bin/env python3 -B

from array import array
from uvmap import UVMap, FancyEye
import sys


if __name__ == '__main__':
    uvmap = UVMap(160, 100)
    uvmap.generate(FancyEye, (-1.6, 1.6, -1.0, 1.0))
    uvmap.save_uv(sys.argv[1])
