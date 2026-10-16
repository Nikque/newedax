#!/usr/bin/env python3
"""Compare the per-position results of two `edax -solve` outputs.

Each position line gives the depth, score, time, nodes, speed and principal variation.
The time and the speed are left out; the depth, score, nodes and principal variation
must be equal. The total node count of the summary line is compared too.

usage: compare_solve.py reference.stdout candidate.stdout
exit code: 0 when equal, 1 when different, 2 on a usage or read error.
"""

import re
import sys

LINE = re.compile(r'^\s*(\d+)\|\s*(\S+)\s+([+-]\d+)\s+\S+\s+(\d+)\s+(\d*)\s*(.*?)\s*$')
TOTAL = re.compile(r'(\d+) nodes in')


def read(path):
    rows, total = {}, None
    with open(path, encoding='utf-8', errors='replace') as f:
        for text in f:
            m = LINE.match(text)
            if m and '|' in text:
                number, depth, score, nodes, _, pv = m.groups()
                rows[int(number)] = (depth, score, int(nodes), ' '.join(pv.split()))
                continue
            t = TOTAL.search(text)
            if t:
                total = int(t.group(1))
    return rows, total


def main(argv):
    if len(argv) != 3:
        print(__doc__.strip())
        return 2
    try:
        a, ta = read(argv[1])
        b, tb = read(argv[2])
    except OSError as e:
        print(e)
        return 2
    differences = 0
    for n in sorted(set(a) | set(b)):
        if a.get(n) != b.get(n):
            differences += 1
            print(f'position {n}:\n  reference {a.get(n)}\n  candidate {b.get(n)}')
    if ta != tb:
        differences += 1
        print(f'total nodes: reference {ta}, candidate {tb}')
    print(f'{len(a)} / {len(b)} positions, total nodes {ta} / {tb}: '
          + ('equal' if differences == 0 else f'{differences} differences'))
    return 0 if differences == 0 and a else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
