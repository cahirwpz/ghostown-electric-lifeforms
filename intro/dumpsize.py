#!/usr/bin/env python3

import argparse
import subprocess


def fileinfo(path):
    output = subprocess.run(
            ['m68k-amigaos-objdump', '-h', path], capture_output=True)
    secsize = {}
    for line in output.stdout.decode('utf-8').splitlines():
        fields = line.split()
        try:
            int(fields[0])
        except (IndexError, ValueError):
            continue
        if fields[1] in ['.text', '.data', '.bss', '.datachip', '.bsschip']:
            secsize[fields[1]] = int(fields[2], 16)

    output = subprocess.run(
            ['m68k-amigaos-objdump', '-r', path], capture_output=True)
    relocs = {}
    secname = None

    for line in output.stdout.decode('utf-8').splitlines():
        fields = line.split()
        if not fields:
            continue
        if 'RELOCATION' == fields[0]:
            secname = fields[3][1:-2]
            relocs[secname] = 0
        if 'RELOC' in fields[1]:
            relocs[secname] += 1

    print('{:>10} {:>6} {:>6}'.format('section', 'size', 'relocs'))
    for key, value in sorted(secsize.items()):
        print('{:>10} {:>6} {:>6}'.format(key, value, relocs.get(key, 0)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Report sizes of sections and number of relocations.')
    parser.add_argument('objfiles', metavar='OBJFILES', type=str, nargs='+',
                        help='AmigaHunk object files.')
    args = parser.parse_args()

    for path in args.objfiles:
        print(f'{path}:\n')
        fileinfo(path)
        print()
