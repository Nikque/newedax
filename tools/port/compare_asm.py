#!/usr/bin/env python3
"""Compare the machine code of two MSVC assembly listings (cl /FAs), function by function.

The functions are matched by their plain names: a C++ function is named by its decorated
name without the decoration (?name@@...). Before the comparison,
the source lines and comments, the numbers of the local labels and the decorated names are
removed, so that the same instructions compare equal.

usage: compare_asm.py reference.asm candidate.asm [function ...]
  without function names: a summary, and the list of the functions which differ;
  with function names: the differing instructions of these functions.
"""

import difflib
import re
import sys

PROC = re.compile(r'^(\S+)\s+PROC\b')
ENDP = re.compile(r'^(\S+)\s+ENDP\b')
LABEL = re.compile(r'\$(LN|SG|LL|LC|M|N|T|R|EN|unwind|chain|pdata|ip2state|cppxdata|stateUnwindMap)\w*', re.I)
DECORATED = re.compile(r'\?[A-Za-z_][\w@?$]*')


def plain(name):
    m = re.match(r'\?([A-Za-z_]\w*)@', name)
    if m:
        return m.group(1)
    return re.sub(r'@@\d+$', '', name)  # __vectorcall functions of C: name@@<bytes of arguments>


def read(path):
    functions, current, body, names = {}, None, [], {}
    with open(path, encoding='latin-1') as f:
        for raw in f:
            line = raw.rstrip('\n')
            if current is None:
                m = PROC.match(line)
                if m:
                    current = plain(m.group(1))
                    names[m.group(1)] = current
                    body = []
                continue
            if ENDP.match(line):
                functions.setdefault(current, body)
                current = None
                continue
            text = line.split(';', 1)[0].strip()
            if not text:
                continue
            body.append(text)
    return functions, names


def normal(lines, names):
    out = []
    for text in lines:
        for k, v in names.items():
            if k in text:
                text = text.replace(k, v)
        text = DECORATED.sub(lambda m: plain(m.group(0)), text)
        text = LABEL.sub('$L', text)
        text = re.sub(r'\s+', ' ', text)
        out.append(text)
    return out


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip())
        return 2
    a, na = read(argv[1])
    b, nb = read(argv[2])
    names = {**na, **nb}
    if len(argv) > 3:
        for name in argv[3:]:
            x, y = normal(a.get(name, []), names), normal(b.get(name, []), names)
            print(f'== {name}: {len(x)} / {len(y)} instructions')
            for d in difflib.unified_diff(x, y, 'reference', 'candidate', lineterm='', n=1):
                print(d)
        return 0
    same, different, only_a, only_b = [], [], [], []
    for name in sorted(set(a) | set(b)):
        if name not in b:
            only_a.append(name)
        elif name not in a:
            only_b.append(name)
        elif normal(a[name], names) == normal(b[name], names):
            same.append(name)
        else:
            different.append((name, len(a[name]), len(b[name])))
    print(f'functions: same {len(same)}, different {len(different)}, only in reference {len(only_a)}, only in candidate {len(only_b)}')
    for name, x, y in different:
        print(f'  differs: {name} ({x} -> {y} instructions)')
    for name in only_a:
        print(f'  only in reference: {name}')
    for name in only_b:
        print(f'  only in candidate: {name}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
