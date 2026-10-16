#!/usr/bin/env python3
"""Compare the machine code of two linked executables, function by function.

The input is the output of `dumpbin /disasm:nobytes` of an executable linked with /DEBUG
(so that the functions are named). The functions are matched by their plain names
(C++ names without their decoration, C names without @@<n>). The addresses are removed
before the comparison: the code is compared instruction by instruction.

usage: compare_disasm.py reference.txt candidate.txt [function ...]
  without function names: a summary and the list of the functions which differ;
  with function names: the differing instructions of these functions.
"""

import difflib
import re
import sys

NAME = re.compile(r'^([A-Za-z_?@$][^\s:]*):$')
CODE = re.compile(r'^\s+[0-9A-F]{16}:\s+(.*)$')
DECORATED = re.compile(r'\?\??[A-Za-z_]\w*@[\w@?$]*')
ADDRESS = re.compile(r'\b[0-9A-F]{8,16}h?\b')


def plain(name):
    m = re.match(r'\?([A-Za-z_]\w*)@', name)
    if m:
        return m.group(1)
    return re.sub(r'@@\d+$', '', name)


def read(path):
    functions, current = {}, None
    with open(path, encoding='latin-1') as f:
        for line in f:
            line = line.rstrip('\n')
            m = NAME.match(line)
            if m:
                current = plain(m.group(1))
                functions.setdefault(current, [])
                continue
            m = CODE.match(line)
            if m and current is not None:
                text = DECORATED.sub(lambda d: plain(d.group(0)), m.group(1))
                text = ADDRESS.sub('ADDR', text)
                text = re.sub(r'\s+', ' ', text).strip()
                functions[current].append(text)
    return functions


def main(argv):
    sys.stdout.reconfigure(errors="replace")
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    a, b = read(argv[1]), read(argv[2])
    if len(argv) > 3:
        for name in argv[3:]:
            x, y = a.get(name, []), b.get(name, [])
            print(f'== {name}: {len(x)} / {len(y)} instructions')
            for d in difflib.unified_diff(x, y, 'reference', 'candidate', lineterm='', n=1):
                print(d)
        return 0
    same, different = 0, []
    for name in sorted(set(a) & set(b)):
        if a[name] == b[name]:
            same += 1
        else:
            different.append((name, len(a[name]), len(b[name])))
    only_a = sorted(set(a) - set(b))
    only_b = sorted(set(b) - set(a))
    print(f'functions: same {same}, different {len(different)}, only in reference {len(only_a)}, only in candidate {len(only_b)}')
    for name, x, y in different:
        print(f'  differs: {name} ({x} -> {y} instructions)')
    for name in only_a:
        print(f'  only in reference: {name}')
    for name in only_b:
        print(f'  only in candidate: {name}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
