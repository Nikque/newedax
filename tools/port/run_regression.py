#!/usr/bin/env python3
"""Run cases of tests/regression on one logical CPU and keep their outputs.

usage: run_regression.py <regression.exe> <work dir> <eval.dat> <fixture.dat> [case ...]
Each case runs in <work dir> (with tests/merge-complete-fixture.dat copied there), with
EDAX_EVAL_FILE set, a time limit of 60 s (REGRESSION_TIMEOUT changes it), and the output saved as <case>.out. A summary
line per case gives the exit code and the time.
"""

import os
import shutil
import subprocess
import sys
import time

CASES = ['bool', 'short', 'ggf', 'edx', 'savefail', 'deviatesave', 'expandredo', 'analyze', 'enhance',
         'core', 'nboard', 'gtp', 'gtptime', 'gtpreg', 'xboard', 'formats', 'pgnlong', 'pgnfen',
         'unsupported', 'bookio', 'truncated', 'corruptstartup', 'ggsclock', 'cassio', 'events',
         'missinglink', 'mergefile', 'linkstore', 'mergetwice', 'negamaxloop', 'learnline', 'hashlock',
         'lastgame', 'basereplace', 'keepunread', 'statsrange', 'gamescore', 'badinput', 'wthortrunc',
         'cassiolist', 'parsers', 'tools', 'unique', 'bookform2', 'evalform2', 'evalslots', 'crc32c', 'bigcounts']


def main(argv):
    if len(argv) < 5:
        print(__doc__.strip())
        return 2
    exe, work, eval_file, fixture = (os.path.abspath(a) for a in argv[1:5])
    cases = argv[5:] or CASES
    os.makedirs(os.path.join(work, 'tests'), exist_ok=True)
    shutil.copyfile(fixture, os.path.join(work, 'tests', 'merge-complete-fixture.dat'))
    env = dict(os.environ, EDAX_EVAL_FILE=eval_file)
    failed = 0
    for case in cases:
        t = time.time()
        with open(os.path.join(work, case + '.out'), 'wb') as out:
            p = subprocess.Popen([exe, case], cwd=work, env=env, stdout=out, stderr=subprocess.STDOUT)
            try:
                if os.name == 'nt':
                    import ctypes
                    h = ctypes.windll.kernel32.OpenProcess(0x0200 | 0x0400, False, p.pid)
                    ctypes.windll.kernel32.SetProcessAffinityMask(h, 1)
                    ctypes.windll.kernel32.CloseHandle(h)
                code = p.wait(timeout=int(os.environ.get('REGRESSION_TIMEOUT', '60')))
            except subprocess.TimeoutExpired:
                p.kill()
                p.wait()
                code = 'timeout'
        failed += code != 0
        print(f'{case}: exit {code}, {time.time() - t:.1f} s')
    print(f'{len(cases)} cases, {failed} not 0')
    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
