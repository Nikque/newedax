# Tests of the evaluation tools with inputs the C tools of D:\edaxwork\ev did not handle (errors, limits, paths),
# run on one logical CPU. Each test runs a tool of --exe and checks its exit code, messages or outputs.
# usage: py -3 test_evtools.py --exe <dir of the tools> --work <work dir> [--ev D:\edaxwork\ev] [--edax <new edax exe>]
#                              [--exe-avx2 <dir of the tools built with /arch:AVX2>] [--tests a,b,...] [--cpu n]
#   --edax: an edax.exe of this repository (to write a book of form 2 with -book-convert).
import argparse, ctypes, os, shutil, struct, subprocess, sys, time
from ctypes import wintypes

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
k32.CreateJobObjectW.restype = wintypes.HANDLE


class IO_COUNTERS(ctypes.Structure):
    _fields_ = [(n, ctypes.c_ulonglong) for n in ('r', 'w', 'o', 'rb', 'wb', 'ob')]


class BASIC(ctypes.Structure):
    _fields_ = [('PerProcessUserTimeLimit', ctypes.c_longlong), ('PerJobUserTimeLimit', ctypes.c_longlong),
                ('LimitFlags', wintypes.DWORD), ('MinimumWorkingSetSize', ctypes.c_size_t), ('MaximumWorkingSetSize', ctypes.c_size_t),
                ('ActiveProcessLimit', wintypes.DWORD), ('Affinity', ctypes.c_size_t), ('PriorityClass', wintypes.DWORD),
                ('SchedulingClass', wintypes.DWORD)]


class EXTENDED(ctypes.Structure):
    _fields_ = [('Basic', BASIC), ('Io', IO_COUNTERS), ('ProcessMemoryLimit', ctypes.c_size_t), ('JobMemoryLimit', ctypes.c_size_t),
                ('PeakProcessMemoryUsed', ctypes.c_size_t), ('PeakJobMemoryUsed', ctypes.c_size_t)]


CPU = 0


def run(cmd, cwd, timeout=180, memory_limit=0):
    """Run on logical CPU --cpu (below-normal priority), optionally with a limit of the committed memory (bytes).
    Returns (exit code or 'timeout', stdout + stderr)."""
    p = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=0x4000 | (0x4 if memory_limit else 0))
    h = wintypes.HANDLE(int(p._handle))
    k32.SetProcessAffinityMask(h, ctypes.c_size_t(1 << CPU))
    if memory_limit:
        job = k32.CreateJobObjectW(None, None)
        info = EXTENDED()
        info.Basic.LimitFlags = 0x100  # JOB_OBJECT_LIMIT_PROCESS_MEMORY
        info.ProcessMemoryLimit = memory_limit
        k32.SetInformationJobObject(job, 9, ctypes.byref(info), ctypes.sizeof(info))
        k32.AssignProcessToJobObject(job, h)
        ntdll.NtResumeProcess(h)
    try:
        out = p.communicate(timeout=timeout)[0]
        return p.returncode, out.decode('cp932', 'replace')
    except subprocess.TimeoutExpired:
        p.kill()
        out = p.communicate()[0]
        return 'timeout', out.decode('cp932', 'replace')


def rel_ev(a, cwd):
    return os.path.relpath(a.ev, cwd)


# ---- tests: each returns (passed, detail) ----

def t_src_drive(a, d):
    """evsearch and evtrain read a data source given with a drive letter (C: no record, nothing trained)"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2 d8\n')
    rc, out = run([a.tool('evsearch'), '-lib', os.path.join(a.ev, 'patterns.txt'), '-configs', cfg, '-plies', '20', '-threads', '1',
                   '-src', 'ss:' + os.path.join(a.ev, 'sp8') + ':1:20000', '-iter', '2'], d)
    ok1 = rc == 0 and ' 20:ss ' in out
    rc2, out2 = run([a.tool('evtrain'), '-init', os.path.join(a.ev, 'eval_orig.dat'), '-src', 'ss:' + os.path.join(a.ev, 'sp8') + ':1:20000',
                     '-plies', '20-20', '-eval', '-threads', '1'], d)
    ok2 = rc2 == 0 and 'ss       ' in out2 and ' -> ' in out2
    return ok1 and ok2, f'evsearch exit {rc}: {out.strip()[-120:]!r}; evtrain exit {rc2}: {out2.strip()[-120:]!r}'


def t_base_drive(a, d):
    """evsearch -base takes a directory with a drive letter (C: cannot open D/...)"""
    base = os.path.join(d, 'base'); os.makedirs(base)
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2 d8\n')
    e = rel_ev(a, d)
    common = [a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20', '-threads', '1', '-src', f'ss:{e}/sp8:1:20000', '-iter', '2']
    run(common + ['-dump', base], d)
    open(cfg, 'w').write('Rb l3\n')
    rc, out = run(common + ['-base', base + ':Rt'], d)
    return rc == 0 and ' 20:ss ' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_src_bad(a, d):
    """a data source without its weight is refused (C: its samples get the weight 0)"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2\n')
    e = rel_ev(a, d)
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20', '-threads', '1', '-src', f'ss:{e}/sp8', '-iter', '2'], d)
    return rc == 1 and 'bad -src' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_many_instances(a, d):
    """a configuration of more than 256 pattern places is refused (C: written beyond its arrays)"""
    names = [l.split()[0] for l in open(os.path.join(a.ev, 'patterns.txt')) if l.strip() and not l.startswith('#')]
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Big ' + ' '.join(names[:60]) + '\n')
    e = rel_ev(a, d)
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20', '-threads', '1', '-src', f'ss:{e}/sp8:1:2000', '-iter', '1'], d)
    return rc == 1 and 'too many' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_many_sources(a, d):
    """more than 8 data sources are refused (C: written beyond its array)"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2\n')
    e = rel_ev(a, d)
    srcs = sum((['-src', f's{i}:{e}/sp8:1:100'] for i in range(9)), [])
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20', '-threads', '1', '-iter', '1'] + srcs, d)
    rc2, out2 = run([a.tool('evtrain'), '-init', f'{e}/eval_orig.dat', '-plies', '20-20', '-eval', '-threads', '1'] + srcs, d)
    return rc == 1 and 'too many' in out and rc2 == 1 and 'too many' in out2, f'evsearch exit {rc} {out.strip()[-80:]!r}; evtrain exit {rc2} {out2.strip()[-80:]!r}'


def t_many_plies(a, d):
    """more than 64 plies in -plies are refused (C: written beyond its arrays)"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2\n')
    e = rel_ev(a, d)
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', ','.join(['20'] * 65), '-threads', '1',
                   '-src', f'ss:{e}/sp8:1:100', '-iter', '1'], d)
    return rc == 1 and 'too many' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_bad_ply_range(a, d):
    """evtrain -plies outside 0..60 is refused (C: weights of ply 61 written beyond the table)"""
    e = rel_ev(a, d)
    rc, out = run([a.tool('evtrain'), '-init', f'{e}/eval_orig.dat', '-out', os.path.join(d, 'o.dat'), '-src', f'ss:{e}/sp8:1:100',
                   '-plies', '60-61', '-threads', '1', '-iter', '1'], d)
    return rc == 1 and 'plies' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_pattern_bad(a, d):
    """pattern lines with missing or bad squares, or more than 16 squares, are refused (C: crash or overflow)"""
    res = []
    for i, line in enumerate(('bad 3 a1 b1\n', 'bad 2 a1 z9\n', 'big 17 a1 b1 c1 d1 e1 f1 g1 h1 a2 b2 c2 d2 e2 f2 g2 h2 a3\n', 'neg -1\n')):
        lib = os.path.join(d, f'lib{i}.txt'); open(lib, 'w').write('l2 8 a2 b2 c2 d2 e2 f2 g2 h2\n' + line)
        cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2\n')
        rc, out = run([a.tool('evsearch'), '-lib', lib, '-configs', cfg, '-plies', '20', '-threads', '1', '-src', f'ss:{rel_ev(a, d)}/sp8:1:100', '-iter', '1'], d)
        res.append((rc, 'pattern' in out, out.strip()[-80:]))
    return all(rc == 1 and found for rc, found, _ in res), repr(res)


def t_missing_files(a, d):
    """missing input files and output directories give an error, not a crash"""
    e = rel_ev(a, d)
    no = os.path.join(d, 'no', 'such')
    shutil.copy(os.path.join(a.ev, 'eval_orig.dat'), d)	# for the tools that open it in the working directory
    cases = [
        ('evsearch', [a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', no, '-plies', '20', '-threads', '1', '-src', f'ss:{e}/sp8:1:100']),
        ('evsearch', [a.tool('evsearch'), '-configs', no, '-plies', '20']),
        ('sample', [a.tool('sample'), no, '10', '1']),
        ('bookscan', [a.tool('bookscan'), no]),
        ('bookdump', [a.tool('bookdump'), os.path.join(a.work, 'A_head.dat'), no]),
        ('sprepair', [a.tool('sprepair'), os.path.join(a.work, 'spbroken'), no, 'ss']),
        ('selfplay3', [a.tool('selfplay3'), no, '1', '1', '1', '-level', '1', '-eval', f'{e}/eval_orig.dat']),
        ('playout', [a.tool('playout'), f'{e}/sp8/ss40.bin', no, '1', '1', '1', '-level', '1', '-eval', f'{e}/eval_orig.dat']),
        ('playout', [a.tool('playout'), no, d, '1', '1', '1', '-level', '1', '-eval', f'{e}/eval_orig.dat']),
        ('gen', [a.tool('gen'), no, d, '1', '1', '1', '-eval', f'{e}/eval_orig.dat']),
        ('label', [a.tool('label'), no, os.path.join(d, 'l.bin'), '1', '1', '-eval', f'{e}/eval_orig.dat']),
        ('label', [a.tool('label'), f'{e}/sp8/ss50.bin', os.path.join(no, 'l.bin'), '1', '1', '-level', '1', '-eval', f'{e}/eval_orig.dat']),
        ('leafgen', [a.tool('leafgen'), os.path.join(a.work, 'leafdata'), '20', '20', '1', '50', '1', no, '-level', '1', '-eval', f'{e}/eval_orig.dat']),
        ('w2dat2', [a.tool('w2dat2'), f'{e}/eval_orig.dat', f'{e}/pr', 'Rm_l100', os.path.join(no, 'x.dat')]),
    ]
    res = []
    for name, cmd in cases:
        if not os.path.exists(cmd[0]):
            continue
        rc, out = run(cmd, d, timeout=60)
        res.append((name, rc, out.strip()[-60:]))
    return all(rc == 1 for _, rc, _ in res), repr(res)


def t_out_of_memory(a, d):
    """evsearch and evtrain stop with a message when memory runs out (C: crash), under a limit of 40 MB"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt c9 c10 s100 s101 l2 l3 l4 d8 d7 d6 d5 d4 mob\n')
    e = rel_ev(a, d)
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20,21', '-threads', '1',
                   '-src', f'ss:{e}/sp8:1', '-src', f'ega:{e}/egd:1', '-iter', '1'], d, memory_limit=40 << 20)
    rc2, out2 = run([a.tool('evtrain'), '-init', f'{e}/eval_orig.dat', '-out', os.path.join(d, 'o.dat'), '-src', f'ss:{e}/sp8:1',
                     '-src', f'ega:{e}/egd:1', '-plies', '20-21', '-threads', '1', '-iter', '1'], d, memory_limit=40 << 20)
    return rc == 1 and 'out of memory' in out and rc2 == 1 and 'out of memory' in out2, f'evsearch exit {rc} {out.strip()[-80:]!r}; evtrain exit {rc2} {out2.strip()[-80:]!r}'


def t_short_weights(a, d):
    """truncated weight files are refused by w2dat2 and evsearch -base (C: read on, garbage weights)"""
    src = os.path.join(a.ev, 'pr')
    wd = os.path.join(d, 'w'); os.makedirs(wd)
    for f in ('Rm_l100.pat', 'Rm_l100.scl'):
        shutil.copy(os.path.join(src, f), wd)
    for ply in range(2, 54):
        b = open(os.path.join(src, f'Rm_l100_{ply:02d}.w'), 'rb').read()
        open(os.path.join(wd, f'Rm_l100_{ply:02d}.w'), 'wb').write(b if ply != 30 else b[:len(b) // 2])
    e = rel_ev(a, d)
    rc, out = run([a.tool('w2dat2'), f'{e}/eval_orig.dat', wd, 'Rm_l100', os.path.join(d, 'x.dat')], d)
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rb l2\n')
    rc2, out2 = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '30', '-threads', '1', '-src', f'ss:{e}/sp8:1:1000',
                     '-iter', '1', '-base', os.path.relpath(wd, d) + ':Rm_l100'], d)
    return rc == 1 and rc2 == 1, f'w2dat2 exit {rc} {out.strip()[-80:]!r}; evsearch exit {rc2} {out2.strip()[-80:]!r}'


def t_book_form2(a, d):
    """bookdump and bookscan refuse a book that is not of form 1 (C: read as form 1)"""
    if not a.edax:
        return None, 'no --edax'
    b2 = os.path.join(d, 'b2.dat')
    run([a.edax, '-book-format', '2', '-book-convert', os.path.join(a.work, 'A_head.dat'), b2], d)
    if not os.path.exists(b2):
        return False, 'no book of form 2 written'
    rc, out = run([a.tool('bookdump'), b2, d], d)
    rc2, out2 = run([a.tool('bookscan'), b2], d)
    return rc == 1 and 'form' in out and rc2 == 1 and 'form' in out2, f'bookdump exit {rc} {out.strip()[-80:]!r}; bookscan exit {rc2} {out2.strip()[-80:]!r}'


def t_selfplay_random_range(a, d):
    """selfplay3 refuses -rmin above -rmax (C: division by zero)"""
    rc, out = run([a.tool('selfplay3'), d, '1', '1', '1', '-rmin', '10', '-rmax', '5', '-level', '1', '-eval', f'{rel_ev(a, d)}/eval_orig.dat'], d, timeout=60)
    return rc == 1 and 'rmin' in out, f'exit {rc}: {out.strip()[-120:]!r}'


def t_gen_low_ply(a, d):
    """gen writes the positions of plies 40 and below (C: written to a file it did not open)"""
    start = os.path.join(d, 'b38.bin')
    open(start, 'wb').write(open(os.path.join(a.ev, 'data', 'book38.bin'), 'rb').read(17 * 200))
    shutil.copy(os.path.join(a.ev, 'eval_orig.dat'), d)
    rc, out = run([a.tool('gen'), start, d, '1', '1', '1', '-last', '39'], d, timeout=300)
    f = os.path.join(d, 'gen39.bin')
    return rc == 0 and os.path.exists(f) and os.path.getsize(f) == 17, f'exit {rc}: {out.strip()[-120:]!r}'


def t_gen_no_training(a, d):
    """gen stops when the start file has no position of the training split (C: endless loop) or none at all (C: division by zero)"""
    shutil.copy(os.path.join(a.ev, 'eval_orig.dat'), d)
    recs = open(os.path.join(a.ev, 'data', 'book40.bin'), 'rb').read(17 * 4000)
    test = b''
    for i in range(0, len(recs), 17):
        P, O = struct.unpack('<QQ', recs[i:i + 16])
        if mix64((P * 0x9E3779B97F4A7C15 ^ O) & M64) % 32 == 0:
            test += recs[i:i + 17]
    open(os.path.join(d, 'test.bin'), 'wb').write(test)
    open(os.path.join(d, 'empty.bin'), 'wb').write(b'')
    rc, out = run([a.tool('gen'), os.path.join(d, 'test.bin'), d, '1', '1', '1'], d, timeout=20)
    rc2, out2 = run([a.tool('gen'), os.path.join(d, 'empty.bin'), d, '1', '1', '1'], d, timeout=20)
    return rc == 1 and rc2 == 1, f'test split only: exit {rc} {out.strip()[-80:]!r}; empty: exit {rc2} {out2.strip()[-80:]!r}'


def t_leafgen_keep_all(a, d):
    """leafgen keeps every training position when n_per_ply is at least their number, in a build without AVX-512
    (C: the threshold 1.0 * 2^64 converted to 0)"""
    if not a.exe_avx2:
        return None, 'no --exe-avx2'
    data = os.path.join(a.work, 'leafdata')
    recs = open(os.path.join(data, 'book20.bin'), 'rb').read()
    n = sum(1 for i in range(0, len(recs), 17)
            if mix64((struct.unpack('<Q', recs[i:i + 8])[0] * 0x9E3779B97F4A7C15 ^ struct.unpack('<Q', recs[i + 8:i + 16])[0]) & M64) % 32)
    exe = os.path.join(a.exe_avx2, 'leafgen.exe')
    rc, out = run([exe, data, '20', '20', '100000', '1000000', '1', d, '-level', '1', '-eval', f'{rel_ev(a, d)}/eval_orig.dat'], d, timeout=600)
    return rc == 0 and f'{n} start positions' in out, f'expected {n}; exit {rc}: {out.strip()[:120]!r}'


M64 = (1 << 64) - 1


def mix64(x):
    x ^= x >> 30; x = (x * 0xbf58476d1ce4e5b9) & M64; x ^= x >> 27; x = (x * 0x94d049bb133111eb) & M64; x ^= x >> 31
    return x


TESTS = [t_src_drive, t_base_drive, t_src_bad, t_many_instances, t_many_sources, t_many_plies, t_bad_ply_range, t_pattern_bad,
         t_missing_files, t_out_of_memory, t_short_weights, t_book_form2, t_selfplay_random_range, t_gen_low_ply,
         t_gen_no_training, t_leafgen_keep_all]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True); ap.add_argument('--work', required=True); ap.add_argument('--ev', default=r'D:\edaxwork\ev')
    ap.add_argument('--edax', default=''); ap.add_argument('--exe-avx2', default=''); ap.add_argument('--tests', default='')
    ap.add_argument('--cpu', type=int, default=0)
    a = ap.parse_args()
    global CPU
    CPU = a.cpu
    a.tool = lambda t: os.path.join(a.exe, t + '.exe')
    want = set(a.tests.split(',')) if a.tests else None
    failed = 0
    for t in TESTS:
        name = t.__name__[2:]
        if want and name not in want:
            continue
        d = os.path.join(a.work, 'tests', os.path.basename(os.path.normpath(a.exe)), name)
        if os.path.exists(d):
            shutil.rmtree(d)
        os.makedirs(d)
        t0 = time.time()
        ok, detail = t(a, d)
        status = 'skipped' if ok is None else 'ok' if ok else 'FAILED'
        failed += ok is False
        print(f'{name:22s} {status:7s} ({time.time() - t0:5.1f} s)  {detail[:400]}', flush=True)
    print(f'tests failed: {failed}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
