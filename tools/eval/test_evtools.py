# Tests of the evaluation tools with inputs the C tools of D:\edaxwork\ev did not handle (errors, limits, paths),
# run on one logical CPU. Each test runs a tool of --exe and checks its exit code, messages or outputs.
# usage: py -3 test_evtools.py --exe <dir of the tools> --work <work dir> [--ev D:\edaxwork\ev] [--edax <new edax exe built with EVAL_MOBILITY=0>]
#                              [--exe-avx2 <dir of the tools built with /arch:AVX2>] [--tests a,b,...] [--cpu n]
#                              [--edax-mob <new edax exe built with EVAL_MOBILITY=1>] [--eval2 <eval.dat of eval2>]
#   --edax: an edax.exe of this repository (to write a book of form 2 with -book-convert, an eval.dat of form 2 with
#   -eval-convert; built with /D EVAL_MOBILITY=0, the default being 1).
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
    """pattern lines with missing or bad squares, more than 16 squares, or a square twice, are refused (C: crash or
    overflow)"""
    res = []
    for i, line in enumerate(('bad 3 a1 b1\n', 'bad 2 a1 z9\n', 'big 17 a1 b1 c1 d1 e1 f1 g1 h1 a2 b2 c2 d2 e2 f2 g2 h2 a3\n', 'neg -1\n',
                              'dup 2 a1 a1\n')):
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
    """bookdump and bookscan read a book of form 2 as the same book of form 1 (C: read the form 2 file as form 1);
    a damaged or truncated book of form 2 is refused"""
    if not a.edax:
        return None, 'no --edax'
    b2, b1 = os.path.join(d, 'b2.dat'), os.path.join(d, 'b1.dat')	# the form 1 book written back from b2 (same order)
    run([a.edax, '-book-format', '2', '-book-convert', os.path.join(a.work, 'A_big.dat'), b2], d)	# (many times the buffer of BookReader in both forms)
    run([a.edax, '-book-format', '1', '-book-convert', b2, b1], d)
    if not (os.path.exists(b2) and os.path.exists(b1)):
        return False, 'no book of form 2 or 1 written'
    res = []
    d1, d2 = os.path.join(d, 'f1'), os.path.join(d, 'f2')
    os.makedirs(d1); os.makedirs(d2)
    rc1, out1 = run([a.tool('bookdump'), b1, d1], d)
    rc2, out2 = run([a.tool('bookdump'), b2, d2], d)
    same_files = sorted(os.listdir(d1)) == sorted(os.listdir(d2)) and len(os.listdir(d1)) > 0 and all(
        open(os.path.join(d1, f), 'rb').read() == open(os.path.join(d2, f), 'rb').read() for f in os.listdir(d1))
    res.append(('bookdump', rc1, rc2, out1 == out2, same_files))
    rc1, out1 = run([a.tool('bookscan'), b1], d)
    rc2, out2 = run([a.tool('bookscan'), b2], d)
    res.append(('bookscan', rc1, rc2, out1 == out2))
    data = bytearray(open(b2, 'rb').read())
    bad = bytearray(data); bad[64 + 100] ^= 0x01	# a byte of the positions
    open(os.path.join(d, 'bad.dat'), 'wb').write(bad)
    open(os.path.join(d, 'cut.dat'), 'wb').write(data[:-20])
    rc3, out3 = run([a.tool('bookscan'), os.path.join(d, 'bad.dat')], d)
    rc4, out4 = run([a.tool('bookscan'), os.path.join(d, 'cut.dat')], d)
    ok = res[0][1:] == (0, 0, True, True) and res[1][1:] == (0, 0, True) and rc3 == 1 and rc4 == 1
    return ok, f'{res}; damaged: exit {rc3} {out3.strip()[-60:]!r}; truncated: exit {rc4} {out4.strip()[-60:]!r}'


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


# ---- a reference board (squares 0 = a1 ... 63 = h8; P: side to move) ----

def ref_flips(P, O, x):
    if (P | O) >> x & 1:
        return 0
    f = 0
    r0, c0 = divmod(x, 8)
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            r, c, run = r0 + dr, c0 + dc, 0
            while 0 <= r < 8 and 0 <= c < 8:
                q = 1 << (r * 8 + c)
                if O & q:
                    run |= q
                elif P & q:
                    f |= run
                    break
                else:
                    break
                r += dr; c += dc
    return f


def ref_moves(P, O):
    return [x for x in range(64) if ref_flips(P, O, x)]


def ref_next(P, O, x):
    f = ref_flips(P, O, x)
    return O ^ f, P | f | (1 << x)


def read_records(path):
    data = open(path, 'rb').read() if os.path.exists(path) else b''
    return [struct.unpack('<QQb', data[i:i + 17]) for i in range(0, len(data), 17)]


def t_trans2rec(a, d):
    """trans2rec: the records of random legal games (with passes) as computed here; an illegal move is refused"""
    import random
    rnd = random.Random(5)
    lines, expected = [], {}
    frm = 6
    for g in range(300):
        P, O, colour, moves, recs = 0x0000000810000000, 0x0000001008000000, 1, '', []
        while True:
            ms = ref_moves(P, O)
            if not ms:
                P, O, colour = O, P, -colour
                ms = ref_moves(P, O)
                if not ms:
                    break
            if g % 50 == 7 and len(moves) >= 40:	# a few games stop before their end
                break
            x = rnd.choice(ms)
            if 64 - bin(P | O).count('1') <= 60 - frm:
                recs.append((P, O, colour))
            moves += 'abcdefgh'[x % 8] + str(x // 8 + 1)
            P, O = ref_next(P, O, x)
            colour = -colour
        diff = bin(P).count('1') - bin(O).count('1')
        e = 64 - bin(P | O).count('1')
        diff = diff + e if diff > 0 else diff - e if diff < 0 else 0
        for p, o, s in recs:
            ply = bin(p | o).count('1') - 4
            expected.setdefault(ply, []).append(struct.pack('<QQb', p, o, diff if s == colour else -diff))
        lines.append(moves.upper() if g % 3 == 0 else moves + (' 12 extra\r' if g % 5 == 0 else ''))
    tr = os.path.join(d, 'games.txt')
    open(tr, 'w', newline='\n').write('# comment\n\n' + '\n'.join(lines) + '\n')
    out = os.path.join(d, 'out'); os.makedirs(out)
    rc, text = run([a.tool('trans2rec'), out, 'tr', '-from', str(frm), tr], d)
    got = {int(f[2:4]): open(os.path.join(out, f), 'rb').read() for f in os.listdir(out)}
    same = got == {k: b''.join(v) for k, v in expected.items()}
    bad = os.path.join(d, 'bad.txt'); open(bad, 'w').write('f5d6c3a1\n')
    rc2, text2 = run([a.tool('trans2rec'), out, 'bad', bad], d)
    return rc == 0 and same and rc2 == 1 and 'illegal' in text2, f'exit {rc} same {same} ({len(got)} plies): {text.strip()[-80:]!r}; illegal: exit {rc2} {text2.strip()[-60:]!r}'


def t_selfplay_alt(a, d):
    """selfplay3 -alt: the files sp, ss are the same as without it; each position of sa follows a position of ss by
    one legal move, and its value is a score"""
    e = rel_ev(a, d)
    base, alt = os.path.join(d, 'base'), os.path.join(d, 'alt')
    os.makedirs(base); os.makedirs(alt)
    common = ['30', '3', '1', '-level', '2', '-exact', '8', '-eval', f'{e}/eval_orig.dat']
    rc1, _ = run([a.tool('selfplay3'), base] + common, d)
    rc2, text = run([a.tool('selfplay3'), alt] + common + ['-alt', '0.5'], d)
    same = all(open(os.path.join(base, f), 'rb').read() == open(os.path.join(alt, f), 'rb').read() for f in os.listdir(base))
    n_alt, reachable = 0, True
    for q in range(61):
        sa = read_records(os.path.join(alt, f'sa{q:02d}.bin'))
        if not sa:
            continue
        parents = read_records(os.path.join(alt, f'ss{q - 1:02d}.bin'))
        children = {ref_next(p, o, x) for p, o, _ in parents for x in ref_moves(p, o)}
        for p, o, v in sa:
            n_alt += 1
            reachable &= (p, o) in children and -64 <= v <= 64
    return rc1 == 0 and rc2 == 0 and same and n_alt > 0 and reachable, f'exit {rc1}/{rc2}, sp ss same {same}, sa positions {n_alt}, reachable {reachable}; {text.strip()[-60:]!r}'


def t_evsearch_window(a, d):
    """evsearch -window 0 gives the same outputs as without it; -window 1 trains on more samples; bad values refused"""
    e = rel_ev(a, d)
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2 d8\n')
    common = [a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '30', '-threads', '1', '-src', f'ss:{e}/sp8:1:20000', '-iter', '3']
    outs = {}
    for name, extra in (('none', []), ('w0', ['-window', '0']), ('w1', ['-window', '1', '-decay', '0.5'])):
        o = os.path.join(d, name); os.makedirs(o)
        rc, text = run(common + extra + ['-dump', o], d)
        outs[name] = (rc, text, open(os.path.join(o, 'Rt_30.w'), 'rb').read() if rc == 0 else b'')
    rc, text = run(common + ['-window', '1', '-decay', '0'], d)
    ok = (outs['none'][0] == 0 and outs['none'][1] == outs['w0'][1] and outs['none'][2] == outs['w0'][2]
          and outs['w1'][0] == 0 and outs['w1'][2] != outs['none'][2] and ' 30:ss ' in outs['w1'][1] and rc == 1)
    return ok, f"none/w0 same {outs['none'][2] == outs['w0'][2]}, w1 differs {outs['w1'][2] != outs['none'][2]}; bad decay exit {rc}"


START_OBF = '-' * 27 + 'OX' + '-' * 6 + 'XO' + '-' * 27 + ' X;\n'


def eval2_form2(a, d):
    """the eval.dat of eval2 (form 1 + MOBW) converted into form 2 by --edax; the name of the file"""
    f2 = os.path.join(d, 'eval2-f2.dat')
    run([a.edax, '-eval-convert', a.eval2, f2], d)
    return f2


def mobility_table(path, ply):
    """the mobility weights of a ply of an eval.dat of form 1 + MOBW"""
    data = open(path, 'rb').read()
    at = 28 + 61 * 114364 * 2 + 4 + ply * 128
    return list(struct.unpack('<64h', data[at:at + 128]))


def t_eval_form2_mob(a, d):
    """eval.dat of form 2 with the mobility group (DESIGN-format.md 3.4): eval2 into form 2 and back gives the same
    bytes; a build without EVAL_MOBILITY refuses the file, a build with it reads it"""
    if not (a.edax and a.edax_mob and a.eval2):
        return None, 'no --edax, --edax-mob or --eval2'
    f2, f1 = eval2_form2(a, d), os.path.join(d, 'eval2-f1.dat')
    rc0, out0 = run([a.edax, '-eval-convert', f2, f1], d)
    same = os.path.exists(f1) and open(f1, 'rb').read() == open(a.eval2, 'rb').read()
    obf = os.path.join(d, 'start.obf'); open(obf, 'w').write(START_OBF)
    common = ['-eval-file', f2, '-book-usage', 'off', '-n', '1', '-l', '2', '-solve', obf]
    rc1, out1 = run([a.edax] + common, d, timeout=60)
    rc2, out2 = run([a.edax_mob] + common, d, timeout=60)
    ok = rc0 == 0 and same and rc1 != 0 and 'EVAL_MOBILITY' in out1 and rc2 == 0
    return ok, f'convert back exit {rc0} same {same}; EVAL_MOBILITY=0 build exit {rc1} {out1.strip()[-80:]!r}; EVAL_MOBILITY build exit {rc2}'


def t_evcheck_mob(a, d):
    """evcheck (built with EVAL_MOBILITY): the evaluation of evtool, mobility included, as Edax's on the eval.dat of eval2
    in form 1 and in form 2"""
    if not (a.edax and a.eval2):
        return None, 'no --edax or --eval2'
    f2 = eval2_form2(a, d)
    res = []
    for f in (a.eval2, f2):
        rc, out = run([a.tool('evcheck'), f, '300'], d)
        res.append((rc, 'mobility weights compared too' in out, out.strip()[-60:]))
    return all(rc == 0 and mob for rc, mob, _ in res), repr(res)


def t_teacher_prior_mob(a, d):
    """evsearch with the eval.dat of eval2: -teacher gives the same outputs from form 1 and form 2; -prior frozen
    (-lambda-prior 1e30) keeps the mobility weights of eval2; evtrain refuses a file with mobility weights"""
    if not (a.edax and a.eval2):
        return None, 'no --edax or --eval2'
    f2 = eval2_form2(a, d)
    e = rel_ev(a, d)
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2 d8 mob\n')
    common = [a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '30', '-threads', '1', '-src', f'ss:{e}/sp8:1:20000', '-iter', '3']
    outs = []
    for name, f in (('t1', a.eval2), ('t2', f2)):
        o = os.path.join(d, name); os.makedirs(o)
        rc, text = run(common + ['-teacher', f, '-alpha', '0.5', '-dump', o], d)
        outs.append((rc, text, open(os.path.join(o, 'Rt_30.w'), 'rb').read() if rc == 0 else b''))
    teacher_same = outs[0][0] == 0 and outs[0][1:] == outs[1][1:]
    cfgp = os.path.join(d, 'cfgp.txt'); open(cfgp, 'w').write('Rm l2 d8 mob\n')
    o = os.path.join(d, 'prior'); os.makedirs(o)
    rc, text = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfgp, '-plies', '30', '-threads', '1', '-src', f'ss:{e}/sp8:1:2000',
                    '-iter', '1', '-prior', a.eval2, '-lambda-prior', '1e30', '-dump', o], d)
    prior_kept = False
    if rc == 0:
        scl = struct.unpack('<22i', open(os.path.join(o, 'Rm.scl'), 'rb').read())
        w = open(os.path.join(o, 'Rm_30.w'), 'rb').read()
        mob = [round(x * 128) for x in struct.unpack('<64d', w[8 * scl[1]:8 * scl[1] + 512])]
        prior_kept = scl[0] == 1 and mob == mobility_table(a.eval2, 30)
    rc3, text3 = run([a.tool('evtrain'), '-init', a.eval2, '-out', os.path.join(d, 'x.dat'), '-src', f'ss:{e}/sp8:1:100', '-plies', '30-30', '-threads', '1'], d)
    ok = teacher_same and prior_kept and rc3 == 1 and 'mobility' in text3
    return ok, f'teacher form 1 / form 2 same {teacher_same}; frozen prior keeps the mobility weights {prior_kept} (exit {rc}); evtrain exit {rc3} {text3.strip()[-60:]!r}'


def crc32c(data):
    """CRC-32C (as crc32c_update of evtool.hpp)"""
    c = 0xffffffff
    for v in data:
        c ^= v
        for _ in range(8):
            c = (c >> 1) ^ (0x82f63b78 if c & 1 else 0)
    return c ^ 0xffffffff


START_P, START_O = 0x0000000810000000, 0x0000001008000000	# the initial position, black to move


def book1(P, O):
    """a book of form 1 of one position (no link, no leaf)"""
    h = bytearray(42); h[:8] = b'XADEKOOB'; h[8] = 4; h[9] = 5; struct.pack_into('<i', h, 18, 1); struct.pack_into('<I', h, 38, 1)
    return bytes(h) + struct.pack('<QQIIIIhhhBBbB', P, O, 0, 0, 0, 0, 0, 0, 0, 0, 1, -65, 64)


def book2(P, O, counts=b'\0' * 4):
    """a book of form 2 of one position (no link, no leaf) with the counts given as their LEB128 bytes"""
    h = bytearray(64); h[:8] = b'NEDXBOOK'; struct.pack_into('<HH', h, 8, 2, 64); h[18] = 1; struct.pack_into('<Q', h, 48, 1)
    struct.pack_into('<I', h, 60, crc32c(h[:60]))
    b = struct.pack('<QQBBbbbbB', P, O, 0, 1, 0, 0, 0, -65, 64) + counts
    return bytes(h) + b + struct.pack('<QI', len(b), crc32c(b)) + b'BEND'


def t_book_bad_board(a, d):
    """bookdump and bookscan refuse a book position of fewer than 4 discs (C: the ply of the position, below 0, indexes
    their arrays)"""
    res = []
    for form, make in ((1, book1), (2, book2)):
        for name, P, O in (('start', START_P, START_O), ('empty', 0, 0), ('three', START_P, 0x0000001000000000)):
            f = os.path.join(d, f'{name}{form}.dat'); open(f, 'wb').write(make(P, O))
            out_dir = os.path.join(d, f'dump-{name}{form}'); os.makedirs(out_dir)
            rc1, out1 = run([a.tool('bookscan'), f], d)
            rc2, out2 = run([a.tool('bookdump'), f, out_dir], d)
            want = 0 if name == 'start' else 1
            res.append((name, form, rc1, rc2, rc1 == want and rc2 == want and (want == 0 or ('damaged' in out1 and 'damaged' in out2))))
    return all(r[-1] for r in res), repr(res)


def t_book_count_33bits(a, d):
    """bookdump and bookscan refuse a count of form 2 of more than 32 bits, as new edax does; 2^32 - 1 is read"""
    res = []
    for name, counts, want in (('max', b'\xff\xff\xff\xff\x0f' + b'\0' * 3, 0), ('over', b'\x80\x80\x80\x80\x10' + b'\0' * 3, 1)):
        f = os.path.join(d, f'{name}.dat'); open(f, 'wb').write(book2(START_P, START_O, counts))
        out_dir = os.path.join(d, f'dump-{name}'); os.makedirs(out_dir)
        rc1, out1 = run([a.tool('bookscan'), f], d)
        rc2, out2 = run([a.tool('bookdump'), f, out_dir], d)
        res.append((name, rc1, rc2, rc1 == want and rc2 == want and (want == 0 or ('32 bits' in out1 and '32 bits' in out2))))
    return all(r[-1] for r in res), repr(res)


def t_config_twice(a, d):
    """evsearch refuses a configuration that names a scalar feature twice (C: the features of the second one are not
    computed and are read as weight indices)"""
    cfg = os.path.join(d, 'cfg.txt'); open(cfg, 'w').write('Rt l2 mob mob\n')
    e = rel_ev(a, d)
    rc, out = run([a.tool('evsearch'), '-lib', f'{e}/patterns.txt', '-configs', cfg, '-plies', '20', '-threads', '1', '-src', f'ss:{e}/sp8:1:100', '-iter', '1'], d)
    return rc == 1 and 'twice' in out, f'exit {rc}: {out.strip()[-160:]!r}'


def t_build_failure(a, d):
    """build-evtools.ps1 exits with 1 when a tool is not built"""
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build-evtools.ps1')
    rc, out = run(['pwsh', '-NoProfile', '-File', script, '-Out', os.path.join(d, 'out'), '-Tools', 'no_such_tool', '-Cpu', str(CPU)], d)
    return rc == 1 and 'failed: 1' in out, f'exit {rc}: {out.strip()[-160:]!r}'


TESTS = [t_src_drive, t_base_drive, t_src_bad, t_many_instances, t_many_sources, t_many_plies, t_bad_ply_range, t_pattern_bad,
         t_missing_files, t_out_of_memory, t_short_weights, t_book_form2, t_selfplay_random_range, t_gen_low_ply,
         t_gen_no_training, t_leafgen_keep_all, t_trans2rec, t_selfplay_alt, t_evsearch_window,
         t_eval_form2_mob, t_evcheck_mob, t_teacher_prior_mob, t_book_bad_board, t_book_count_33bits, t_config_twice,
         t_build_failure]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True); ap.add_argument('--work', required=True); ap.add_argument('--ev', default=r'D:\edaxwork\ev')
    ap.add_argument('--edax', default=''); ap.add_argument('--exe-avx2', default=''); ap.add_argument('--tests', default='')
    ap.add_argument('--cpu', type=int, default=0)
    ap.add_argument('--edax-mob', default=''); ap.add_argument('--eval2', default='')
    a = ap.parse_args()
    global CPU
    CPU = a.cpu
    k32.SetErrorMode(0x8003)	# a tool that crashes exits (no error dialog); inherited by the tools run
    a.tool = lambda t: os.path.join(a.exe, t + '.exe')
    want = set(a.tests.split(',')) if a.tests else None
    import compare_c	# the inputs made from the data of ev (A_head.dat, spbroken, leafdata, ...) in the work directory
    compare_c.prepare(a)
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
        try:
            ok, detail = t(a, d)
        except Exception as e:	# a test that cannot run fails; the others still run
            ok, detail = False, f'{type(e).__name__}: {e}'
        status = 'skipped' if ok is None else 'ok' if ok else 'FAILED'
        failed += ok is False
        print(f'{name:22s} {status:7s} ({time.time() - t0:5.1f} s)  {detail[:400]}', flush=True)
    print(f'tests failed: {failed}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
