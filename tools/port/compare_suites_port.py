#!/usr/bin/env python3
"""Compare the suite runs of a new build with the recorded runs of v4.5.5-nikque.17.

For each suite and each kind of run (R1: -n 32 -book-seed 1 -book-reproducible on;
Dn1: -n 1, default settings), the saved files of <new runs>/S-<suite>-<kind> are compared
with <recorded runs>/S-<suite>-<kind>: books with compare.py of the reproducibility work
(bytes except the date, record order, raw records, logical data), other files by bytes.

usage: compare_suites_port.py <compare.py directory> <recorded runs> <new runs> <out.json>
"""

import json
import sys
from pathlib import Path

SUITES = ['A', 'B', 'F', 'G', 'MB', 'UA', 'UB', 'UF', 'S1', 'S2', 'S3', 'S3off', 'M1', 'L18']
KINDS = ['R1', 'Dn1']


def files(d):
    out = {}
    for f in sorted(list((d / 'out').rglob('*')) + list((d / 'data').glob('book.dat*'))):
        if f.is_file():
            out[f.relative_to(d).as_posix()] = f
    return out


def main(argv):
    if len(argv) != 5:
        print(__doc__.strip())
        return 2
    sys.path.insert(0, argv[1])
    from compare import compare
    old, new = Path(argv[2]), Path(argv[3])
    rows = []
    for suite in SUITES:
        for kind in KINDS:
            a, b = old / f'S-{suite}-{kind}', new / f'S-{suite}-{kind}'
            if not ((a / 'run.json').exists() and (b / 'run.json').exists()):
                rows.append(dict(suite=suite, kind=kind, file=None, missing_run='old' if not (a / 'run.json').exists() else 'new'))
                continue
            ra = json.loads((a / 'run.json').read_text(encoding='utf-8'))
            rb = json.loads((b / 'run.json').read_text(encoding='utf-8'))
            fa, fb = files(a), files(b)
            for name in sorted(fa.keys() | fb.keys()):
                row = dict(suite=suite, kind=kind, file=name, exit_old=ra['exit'], exit_new=rb['exit'])
                if name not in fa or name not in fb:
                    row.update(missing='old' if name not in fa else 'new')
                elif name.endswith('.txt'):
                    row.update(kind_of_file='text', same=fa[name].read_bytes() == fb[name].read_bytes())
                else:
                    try:
                        c = compare(fa[name], fb[name])
                        row.update(kind_of_file='book', same=c['bytes_equal_except_date'],
                                   **{k: c[k] for k in ('record_order_equal', 'raw_record_differences', 'logical_differences', 'positions_a', 'positions_b')})
                    except Exception as e:
                        row.update(kind_of_file='other', same=fa[name].read_bytes() == fb[name].read_bytes(), note=str(e))
                rows.append(row)
            rows.append(dict(suite=suite, kind=kind, file='(run)', seconds_old=ra['seconds'], seconds_new=rb['seconds'],
                             cpu_old=ra.get('cpu_seconds'), cpu_new=rb.get('cpu_seconds'),
                             peak_mib_old=ra.get('peak_working_set_mib'), peak_mib_new=rb.get('peak_working_set_mib')))
    Path(argv[4]).write_text(json.dumps(rows, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    for suite in SUITES:
        for kind in KINDS:
            r = [x for x in rows if x['suite'] == suite and x['kind'] == kind and x.get('file') not in (None, '(run)')]
            run = [x for x in rows if x['suite'] == suite and x['kind'] == kind and x.get('file') == '(run)']
            if not r:
                print(f'{suite:6} {kind:4} run missing')
                continue
            same = sum(1 for x in r if x.get('same'))
            missing = sum(1 for x in r if 'missing' in x)
            logical = sum(x.get('logical_differences', 0) for x in r)
            t = run[0] if run else {}
            print(f"{suite:6} {kind:4} same {same}/{len(r)} missing {missing} logical_diff {logical}"
                  f"  peak MiB {t.get('peak_mib_old')} -> {t.get('peak_mib_new')}")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
