#!/usr/bin/env python3
"""Move the sources of src/ into module directories, and rewrite the includes (stage 2 of the C++ port).

The includes of the moved files become paths from src/ ("core/board.h"); the build files get src/ as an
include directory. Run from the repository root, on a clean working tree: the moves are done with git mv.

usage: restructure_src.py [--dry-run]
"""

import re
import subprocess
import sys
from pathlib import Path

MODULES = {
    'core': ['bit.h', 'bit.cpp', 'board.h', 'board.cpp', 'board_mmx.cpp', 'board_sse.cpp', 'move.h', 'move.cpp',
             'hash.h', 'hash.cpp', 'const.h', 'settings.h', 'empty.h'],
    'core/flip': None,  # flip_*, count_flip, count_last_flip_*, generate_*
    'eval': ['eval.h', 'eval.cpp', 'eval_sse.cpp', 'eval_builder.cpp'],
    'search': ['search.h', 'search.cpp', 'midgame.cpp', 'endgame.cpp', 'endgame_sse.cpp', 'endgame_neon.cpp',
               'root.cpp', 'ybwc.h', 'ybwc.cpp'],
    'book': ['book.h', 'book.cpp'],
    'game': ['game.h', 'game.cpp', 'base.h', 'base.cpp', 'opening.h', 'opening.cpp'],
    'protocol': ['ui.h', 'ui.cpp', 'edax.cpp', 'cassio.h', 'cassio.cpp', 'ggs.cpp', 'gtp.cpp', 'nboard.cpp',
                 'xboard.cpp', 'event.h', 'event.cpp', 'play.h', 'play.cpp'],
    'util': ['options.h', 'options.cpp', 'util.h', 'util.cpp', 'stats.h', 'stats.cpp', 'cpp_headers.h',
             'boost_support.cpp'],
    'app': ['main.cpp', 'perft.h', 'perft.cpp', 'obftest.h', 'obftest.cpp', 'histogram.h', 'histogram.cpp',
            'bench.cpp', 'harness.cpp', 'intro.h'],
    'api': ['libedax.h', 'libedax.cpp', 'libedax_utf8.h'],
}

SRC = Path('src')


def plan():
    names = sorted(p.name for p in SRC.iterdir() if p.is_file() and p.suffix in ('.h', '.cpp'))
    where = {}
    for d, files in MODULES.items():
        if files is None:
            continue
        for f in files:
            assert f in names, f
            where[f] = d
    for f in names:
        if f in where or f == 'all.cpp':
            continue
        if f.startswith(('flip_', 'count_flip', 'count_last_flip_', 'generate_')):
            where[f] = 'core/flip'
    left = [f for f in names if f not in where and f != 'all.cpp']
    assert not left, left
    return where


INCLUDE = re.compile(r'^(\s*#\s*include\s+")([^"/]+)(")', re.M)


def rewrite(text, where):
    return INCLUDE.sub(lambda m: m.group(1) + (where[m.group(2)] + '/' + m.group(2) if m.group(2) in where else m.group(2)) + m.group(3), text)


def main(argv):
    dry = '--dry-run' in argv
    where = plan()
    for f, d in sorted(where.items(), key=lambda x: (x[1], x[0])):
        print(f'{d}/{f}')
    if dry:
        return 0
    for d in sorted(set(where.values())):
        (SRC / d).mkdir(parents=True, exist_ok=True)
    for f, d in where.items():
        subprocess.run(['git', 'mv', str(SRC / f), str(SRC / d / f)], check=True)
    files = [SRC / 'all.cpp'] + [SRC / d / f for f, d in where.items()]
    for p in files:
        b = p.read_bytes()
        text = b.decode('utf-8')
        new = rewrite(text, where)
        if new != text:
            p.write_bytes(new.encode('utf-8'))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
