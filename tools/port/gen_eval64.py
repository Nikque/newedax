#!/usr/bin/env python3
"""Write src/eval/eval_feature64.h: the feature tables of the 64-slot evaluation (EVAL_SLOTS=64).

The 48-slot tables of src/eval/eval.cpp (EVAL_FEATURE, EVAL_FEATURE_all_opponent) are read and
checked against the squares of the features (EVAL_F2X) and their offsets (EVAL_OFFSET); slots 48 to 63
get the extra pattern groups below.

usage: gen_eval64.py [check]
  check: only check that eval_feature64.h is the one this script writes (exit 1 if not).
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EVAL = ROOT / 'src' / 'eval' / 'eval.cpp'
OUT = ROOT / 'src' / 'eval' / 'eval_feature64.h'

NAMES = [c + r for r in '12345678' for c in 'ABCDEFGH']  # A1 = 0, B1 = 1, ..., H8 = 63
SQ = {n: i for i, n in enumerate(NAMES)}


def xy(x):
    return x % 8, x // 8


def at(c, r):
    return c + 8 * r


SYM = {
    'id': lambda c, r: (c, r),
    'h': lambda c, r: (7 - c, r),        # mirror A <-> H
    'v': lambda c, r: (c, 7 - r),        # mirror 1 <-> 8
    'hv': lambda c, r: (7 - c, 7 - r),
    't': lambda c, r: (r, c),            # transpose (A1-H8 diagonal)
    'ht': lambda c, r: (7 - r, c),       # transpose then mirror A <-> H
}

# Extra pattern groups of the 64-slot evaluation: 4 groups of 10 squares, 4 places each (slots 48-63).
# They are placeholders of the right size (3^10 weights a group), to build and measure the 64-slot
# evaluation: the patterns of a trained evaluation are chosen later.
EXTRA = [
    ('edge block', 'C1 D1 E1 F1 C2 D2 E2 F2 D3 E3', ['id', 'v', 't', 'ht']),
    ('corner 2x5', 'A1 B1 C1 D1 E1 A2 B2 C2 D2 E2', ['id', 'h', 'v', 'hv']),
    ('corner 5x2', 'A1 A2 A3 A4 A5 B1 B2 B3 B4 B5', ['id', 'h', 'v', 'hv']),
    ('diagonal band', 'A1 B2 C3 D4 B1 C2 D3 A2 B3 C4', ['id', 'h', 'v', 'hv']),
]


def extra_features():
    feats = []
    for _, squares, places in EXTRA:
        base = [SQ[s] for s in squares.split()]
        for p in places:
            feats.append([at(*SYM[p](*xy(x))) for x in base])
    assert len(feats) == 16
    for f in feats:
        assert len(set(f)) == len(f)
    return feats


def c_array(text, name):
    m = re.search(r'\b' + name + r'\[\]\s*=\s*\{(.*?)\};', text, re.S)
    return [int(v) for v in re.findall(r'-?\d+', re.sub(r'//[^\n]*', '', m.group(1)))]


def main(argv):
    text = EVAL.read_text(encoding='utf-8')
    # EVAL_F2X: the squares of the features
    m = re.search(r'EVAL_F2X\[\]\s*=\s*\{(.*?)\n\};', text, re.S)
    f2x = []
    for n, sq in re.findall(r'\{\s*(\d+),\s*\{([^}]*)\}\}', m.group(1)):
        f2x.append([SQ[s.strip()] for s in sq.split(',') if s.strip() in SQ][:int(n)])
    assert len(f2x) == 47, len(f2x)
    offset = c_array(text, 'EVAL_OFFSET')
    assert len(offset) == 48
    # EVAL_FEATURE: 65 rows of 48 values
    m = re.search(r'const EVAL_FEATURE_V EVAL_FEATURE\[65\] = \{(.*?)\n\};', text, re.S)
    rows = re.findall(r'\{\{\s*//\s*(\w+)\s*(.*?)\}\}', m.group(1), re.S)
    table = [[int(v) for v in re.findall(r'\d+', body)] for _, body in rows]
    assert len(table) == 65 and all(len(r) == 48 for r in table), [len(r) for r in table]
    m = re.search(r'EVAL_FEATURE_all_opponent = \{\{(.*?)\}\};', text, re.S)
    all_opp = [int(v) for v in re.findall(r'\d+', re.sub(r'//[^\n]*', '', m.group(1)))]
    assert len(all_opp) == 48

    # check the 48-slot tables against the squares of the features
    for i in range(48):
        squares = f2x[i] if i < 47 else []
        expect = [0] * 65
        for j, x in enumerate(squares):
            expect[x] = 3 ** (len(squares) - 1 - j)
        got = [table[x][i] for x in range(65)]
        assert got == expect, ('EVAL_FEATURE', i, got, expect)
        assert all_opp[i] == sum(expect) + offset[i], ('all_opponent', i)

    extra = extra_features()
    full = [row + [0] * 16 for row in table]
    opp = all_opp + [0] * 16
    for k, squares in enumerate(extra):
        for j, x in enumerate(squares):
            full[x][48 + k] = 3 ** (len(squares) - 1 - j)
        opp[48 + k] = sum(3 ** j for j in range(len(squares)))

    out = ['/**', ' * @file eval_feature64.h', ' *',
           ' * Feature tables of the 64-slot evaluation (EVAL_SLOTS=64), written by tools/port/gen_eval64.py',
           ' * from the 48-slot tables of eval.cpp (slots 0-47) and the extra pattern groups (slots 48-63).',
           ' * Do not edit: run the script again.', ' */', '',
           '/** squares of the extra features (slots 48 to 63) */',
           'static const unsigned char EVAL_X_SQUARE[16][10] = {']
    for k, squares in enumerate(extra):
        out.append('\t{' + ', '.join(NAMES[x] for x in squares) + '},' + ('\t// ' + EXTRA[k // 4][0] if k % 4 == 0 else ''))
    out.append('};')
    out.append('')
    out.append('const EVAL_FEATURE_V EVAL_FEATURE[65] = {')
    for x in range(65):
        name = NAMES[x].lower() if x < 64 else 'pass'
        vals = full[x]
        lines = ['\t\t' + ', '.join('%5d' % v for v in vals[i:i + 16]) for i in range(0, 64, 16)]
        out.append('\t{{ // ' + name)
        out.append(',\n'.join(lines))
        out.append('\t}}' + (',' if x < 64 else ''))
    out.append('};')
    out.append('')
    out.append('const EVAL_FEATURE_V EVAL_FEATURE_all_opponent = {{')
    out.append(',\n'.join('\t' + ', '.join('%5d' % v for v in opp[i:i + 16]) for i in range(0, 64, 16)))
    out.append('}};')
    data = '\n'.join(out) + '\n'

    if argv[1:] == ['check']:
        same = OUT.exists() and OUT.read_text(encoding='utf-8') == data
        print('eval_feature64.h', 'is up to date' if same else 'differs')
        return 0 if same else 1
    OUT.write_bytes(data.encode('utf-8'))
    print('48-slot tables checked; wrote', OUT)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
