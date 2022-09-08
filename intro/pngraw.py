#!/usr/bin/env python3

from binascii import hexlify
from collections import namedtuple
from enum import IntEnum

import argparse
import struct
import sys
import zlib


class ColourType(IntEnum):
    GRAYSCALE = 0
    TRUECOLOR = 2
    INDEXED = 3
    GRAYSCALE_ALPHA = 4
    TRUECOLOR_ALPHA = 6


IHDR = namedtuple('IHDR', ('width height bit_depth colour_type' +
                           ' compression filter interlace'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Write uncompressed PNG IDAT chunk to file.')
    parser.add_argument('png', metavar='PNG', type=str,
                        help='PNG image optimized with optipng.')
    parser.add_argument('raw', metavar='RAW', type=str,
                        help='Raw decoded IHDR & IDAT chunk.')
    args = parser.parse_args()

    chunks = dict()

    with open(args.png, 'rb') as f:
        assert f.read(8) == b'\x89\x50\x4e\x47\x0d\x0a\x1a\x0a'

        while hdr := f.read(8):
            size, chunkId = struct.unpack('>I4s', hdr)
            chunkType = chunkId.decode()
            print(chunkType, size)
            chunk = f.read(size)
            calc_crc = zlib.crc32(chunkId + chunk)
            if chunkType == 'IDAT':
                chunk = zlib.decompress(chunk)
                assert chunk[0] < 2, "method check failed"
            if chunkType == 'IHDR':
                chunk = IHDR(*struct.unpack('>IIbbbbb', chunk))
                assert chunk.colour_type <= 3, "colour type check failed"
                assert chunk.compression == 0, "compression check failed"
                assert chunk.filter == 0, "filter check failed"
                assert chunk.interlace == 0, "interlace check failed"
                chunk = chunk._replace(
                        colour_type=ColourType(chunk.colour_type))
                print(chunk)
            chunks[chunkType] = chunk
            file_crc = struct.unpack('>I', f.read(4))[0]
            assert calc_crc == file_crc

    with open(args.raw, 'wb') as f:
        ihdr = chunks['IHDR']
        bpp = ihdr.bit_depth

        if ihdr.colour_type == ColourType.GRAYSCALE_ALPHA:
            bpp *= 2
        elif ihdr.colour_type == ColourType.TRUECOLOR:
            bpp *= 3
        elif ihdr.colour_type == ColourType.TRUECOLOR_ALPHA:
            bpp *= 4

        row_size = (bpp * width + 7) // 8
        pix_size = (bpp + 7) // 8

        hdr = struct.pack('>HHHH', ihdr.width, ihdr.height, row_size, pix_size)
        f.write(hdr)
        f.write(chunks['IDAT'])
