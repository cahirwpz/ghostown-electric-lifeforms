#!/usr/bin/env python3

import argparse
from array import array

# Rewritten from C++, original source here:
# https://github.com/Kalmalyzer/adpcm-68k/blob/master/Codec/codec.cpp
# ... and adapted to 8-bit samples.

IndexTable = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8]


StepSizeTable = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767]


def encode(data_in):
    valpred = 0
    index = 0
    step = StepSizeTable[index]
    bufferstep = 0
    outval = 0
    data_out = array('B')

    for val in data_in:
        val <<= 8

        # Step 1 - compute difference with previous value
        diff = val - valpred
        sign = 8 if diff < 0 else 0

        if sign:
            diff = -diff

        # Step 2 - Divide and clamp
        # Note:
        # This code *approximately* computes:
        #    delta = diff*4/step;
        #    vpdiff = (delta+0.5)*step/4;
        # but in shift step bits are dropped. The net result of this is
        # that even if you have fast mul/div hardware you cannot put it to
        # good use since the fixup would be too expensive.
        delta = 0
        vpdiff = step >> 3

        if diff >= step:
            delta = 4
            diff -= step
            vpdiff += step

        step >>= 1
        if diff >= step:
            delta |= 2
            diff -= step
            vpdiff += step

        step >>= 1
        if diff >= step:
            delta |= 1
            vpdiff += step

        # Step 3 - Update previous value
        if sign:
            valpred -= vpdiff
        else:
            valpred += vpdiff

        # Step 4 - Clamp previous value to 8 bits
        if valpred > 32767:
            valpred = 32767
        elif valpred < -32768:
            valpred = -32768

        # Step 5 - Assemble value, update index and step values
        delta |= sign

        index += IndexTable[delta]
        if index < 0:
            index = 0
        if index > 88:
            index = 88
        step = StepSizeTable[index]

        # Step 6 - Output value
        if bufferstep:
            outval = (delta << 4) & 0xf0
        else:
            outval |= delta & 0x0f
            data_out.append(outval)

        bufferstep = not bufferstep

    # Output last step, if needed
    if not bufferstep:
        data_out.append(outval)

    return data_out


def decode(data_in):
    valpred = 0
    index = 0
    step = StepSizeTable[index]
    bufferstep = 0
    data_out = array('b')

    for i in range(len(data_in) * 2):
        # Step 1 - get the delta value
        if bufferstep:
            delta = data_in[i // 2] & 0xf
        else:
            delta = (data_in[i // 2] >> 4) & 0xf

        bufferstep = not bufferstep

        # Step 2 - Find new index value (for later)
        index += IndexTable[delta]

        if index < 0:
            index = 0
        if index > 88:
            index = 88

        # Step 3 - Separate sign and magnitude
        sign = delta & 8
        delta = delta & 7

        # Step 4 - Compute difference and new predicted value
        #
        # Computes 'vpdiff = (delta+0.5)*step/4', but see comment in `encode`.
        vpdiff = step >> 3
        if delta & 4:
            vpdiff += step
        if delta & 2:
            vpdiff += step >> 1
        if delta & 1:
            vpdiff += step >> 2

        if sign:
            valpred -= vpdiff
        else:
            valpred += vpdiff

        # Step 5 - clamp output value
        if valpred > 32767:
            valpred = 32767
        elif valpred < -32768:
            valpred = -32768

        # Step 6 - Update step value
        step = StepSizeTable[index]

        # Step 7 - Output value
        data_out.append(valpred >> 8)

    return data_out


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Encode 8-bit samples with IMA ADPCM encoding.')
    parser.add_argument(
        'decoded', metavar='DECODED', type=str, help='Decoded samples file.')
    parser.add_argument(
        'encoded', metavar='ENCODED', type=str, help='Encoded samples file.')
    args = parser.parse_args()

    raw = array('b')

    with open(args.decoded, 'rb') as f:
        raw.frombytes(f.read())

    encoded = encode(raw)

    with open(args.encoded, 'wb') as f:
        f.write(encoded)
