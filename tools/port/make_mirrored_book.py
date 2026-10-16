#!/usr/bin/env python3
"""Write a copy of a book where some positions are stored mirrored (not as their unique board).

book fix (book_fix_unique_boards) moves such positions to their unique board: this file makes
that path run. The positions with a count above 65534, and one position out of <step>, are
mirrored horizontally (board, links and leaf). The header, the order and the other bytes are kept.

usage: make_mirrored_book.py <in.dat> <out.dat> [step]
"""

import sys
from pathlib import Path


def mirror_bits(b):
    out = 0
    for row in range(8):
        byte = (b >> (8 * row)) & 0xff
        rev = int('{:08b}'.format(byte)[::-1], 2)
        out |= rev << (8 * row)
    return out


def mirror_move(x):
    return x if x >= 64 else (x & ~7) | (7 - (x & 7))


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    data = bytearray(Path(argv[1]).read_bytes())
    step = int(argv[3]) if len(argv) > 3 else 100
    p, n_mirrored, n_big = 42, 0, 0
    for i in range(int.from_bytes(data[38:42], 'little')):
        n_link = data[p + 38]
        size = 42 + 2 * n_link
        counts = [int.from_bytes(data[p + 16 + 4 * k:p + 20 + 4 * k], 'little') for k in range(4)]
        big = max(counts) > 65534
        n_big += big
        if big or i % step == 0:
            player = int.from_bytes(data[p:p + 8], 'little')
            opponent = int.from_bytes(data[p + 8:p + 16], 'little')
            data[p:p + 8] = mirror_bits(player).to_bytes(8, 'little')
            data[p + 8:p + 16] = mirror_bits(opponent).to_bytes(8, 'little')
            for k in range(n_link):
                data[p + 40 + 2 * k + 1] = mirror_move(data[p + 40 + 2 * k + 1])
            data[p + size - 1] = mirror_move(data[p + size - 1])
            n_mirrored += 1
        p += size
    assert p == len(data)
    Path(argv[2]).write_bytes(bytes(data))
    print(f'{n_mirrored} positions mirrored ({n_big} with a count above 65534)')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
