#!/usr/bin/env python3

from math import sin, pi
from contextlib import redirect_stdout

if __name__ == '__main__':
    with open('sintab.c', 'w') as fh:
        with redirect_stdout(fh):
            prec = 4096
            n = prec // 4

            print('#include <linkerset.h>')
            print('#include <debug.h>\n')
            print('short sintab[%d];\n' % (prec + 1))

            sintab = [int(sin(i * 2 * pi / prec) * prec) for i in range(n + 1)]

            print('static char _sintab_encoded[%d] = {' % len(sintab))
            prev = 0
            for curr in sintab:
                print('\t%d,' % (curr - prev))
                prev = curr
            print('};')

            print('\nvoid InitSinTab(void) {')
            print('  short sum = 0, n = %d;' % n)
            print('  char *encoded = _sintab_encoded;')
            print('  short *inc = &sintab[0];')
            print('  short *dec = &sintab[%d + 1];' % (n * 2))
            print('  Log("[Init] Preparing sinus table\\n");')
            print('  do {')
            print('    sum += *encoded++;')
            print('    inc[2048] = -sum;')
            print('    *inc++ = sum;')
            print('    *--dec = sum;')
            print('    dec[2048] = -sum;')
            print('  } while (--n != -1);')
            print('}')
            print('\nADD2INIT(InitSinTab, 0);')
