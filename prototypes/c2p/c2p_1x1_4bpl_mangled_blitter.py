#!/usr/bin/env python3

from common import Bit, Word, Channel, Blit, Array


def c2p(bitplane_output=True):
    m0 = Word.Mask('ff00')
    m1 = Word.Mask('f0f0')
    m2 = Word.Mask('cccc')

    print("=[ c2p 1x1 4bpl (blitter + mangled) ]=".center(48, '-'))

    def MakeWord(c, color):
        return Word([Bit.Var(c[0], 0, color),
                     Bit.Var(c[1], 0, color),
                     Bit.Var(c[0], 2, color),
                     Bit.Var(c[1], 2, color),
                     Bit.Var(c[0], 1, color),
                     Bit.Var(c[1], 1, color),
                     Bit.Var(c[0], 3, color),
                     Bit.Var(c[1], 3, color),
                     Bit.Var(c[2], 0, color),
                     Bit.Var(c[3], 0, color),
                     Bit.Var(c[2], 2, color),
                     Bit.Var(c[3], 2, color),
                     Bit.Var(c[2], 1, color),
                     Bit.Var(c[3], 1, color),
                     Bit.Var(c[2], 3, color),
                     Bit.Var(c[3], 3, color)])

    A = Array.Make(MakeWord)
    N = len(A)
    Array.Print("Data:", *A)

    B = Array.Zero(N, 16)
    Blit(lambda a, b: (a & m0) | ((b >> 8) & ~m0),
         N // 4, 2, Channel(A, 0, 2), Channel(A, 2, 2), Channel(B, 0, 2))
    Blit(lambda a, b: ((a << 8) & m0) | (b & ~m0),
         N // 4, 2, Channel(A, 0, 2), Channel(A, 2, 2), Channel(B, 2, 2))
    Array.Print("Swap 8x4:", *B)

    C = Array.Zero(N, 16)
    Blit(lambda a, b: (a & m1) | ((b >> 4) & ~m1),
         N // 2, 1, Channel(B, 0, 1), Channel(B, 1, 1), Channel(C, 0, 1))
    Blit(lambda a, b: ((a << 4) & m1) | (b & ~m1),
         N // 2, 1, Channel(B, 0, 1), Channel(B, 1, 1), Channel(C, 1, 1))
    Array.Print("Swap 4x2:", *C)

    if bitplane_output:
        D = [Array.Zero(N // 4, 16) for i in range(4)]
        Blit(lambda a, b: (a & m2) | ((b >> 2) & ~m2), N // 4, 1,
             Channel(C, 0, 3), Channel(C, 2, 3), Channel(D[0], 0, 0))
        Blit(lambda a, b: (a & m2) | ((b >> 2) & ~m2), N // 4, 1,
             Channel(C, 1, 3), Channel(C, 3, 3), Channel(D[1], 0, 0))
        Blit(lambda a, b: ((a << 2) & m2) | (b & ~m2), N // 4, 1,
             Channel(C, 0, 3), Channel(C, 2, 3), Channel(D[2], 0, 0))
        Blit(lambda a, b: ((a << 2) & m2) | (b & ~m2), N // 4, 1,
             Channel(C, 1, 3), Channel(C, 3, 3), Channel(D[3], 0, 0))
        print("Bitplanes:")
        Array.Print("[0]:", *D[0])
        Array.Print("[1]:", *D[1])
        Array.Print("[2]:", *D[2])
        Array.Print("[3]:", *D[3])
    else:
        D = Array.Zero(N, 16)
        Blit(lambda a, b: ((a >> 2) & ~m2) | (b & m2),
             N // 4, 2, Channel(C, 2, 2), Channel(C, 0, 2), Channel(D, 0, 2))
        Blit(lambda a, b: ((a << 2) & m2) | (b & ~m2),
             N // 4, 2, Channel(C, 0, 2), Channel(C, 2, 2), Channel(D, 2, 2))
        Array.Print("Swap 2x2:", *D)


c2p()
