#!/usr/bin/env python3

from binascii import hexlify
from collections import namedtuple

import argparse
import struct
import sys
import zlib


IHDR = namedtuple('IHDR',
                  'width height bpp color compression filter interlace')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Encode 8-bit samples with IMA ADPCM encoding.')
    parser.add_argument(
        'png', metavar='PNG', type=str, help='PNG image optimized with optipng.')
    parser.add_argument(
        'raw', metavar='RAW', type=str, help='Raw decoded IHDR & IDAT chunk.')
    args = parser.parse_args()

    chunks = dict()

    with open(args.png, 'rb') as f:
        print(hexlify(f.read(8)))
        while hdr := f.read(8):
            size, chunkId = struct.unpack('>I4s', hdr)
            chunkType = chunkId.decode()
            print(chunkType, size)
            chunk = f.read(size)
            calc_crc = zlib.crc32(chunkId + chunk)
            if chunkType == 'IDAT':
                chunk = zlib.decompress(chunk)
            if chunkType == 'IHDR':
                print(IHDR(*struct.unpack('>IIbbbbb', chunk)))
            chunks[chunkType] = chunk
            file_crc = struct.unpack('>I', f.read(4))[0]
            assert calc_crc == file_crc

    with open(args.raw, 'wb') as f:
        f.write(chunks['IHDR'])
        f.write(chunks['IDAT'])
