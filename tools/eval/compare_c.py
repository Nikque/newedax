# Run the C tools (built from D:\edaxwork\ev, sources unchanged) and the C++ tools of tools\eval with the same inputs,
# on one logical CPU, and compare their outputs byte for byte (output files, standard output and error).
# usage: py -3 compare_c.py --c <dir of the C exes> --cpp <dir of the C++ exes> --work <empty work dir>
#                           [--c2 <dir of the C exes built with the eval2 sources>] [--eval2 <eval.dat of eval2>]
#                           [--ev D:\edaxwork\ev] [--cases E1,T1,...] [--repeat n]
#   The cases of the tools with the search are run with the C tools of --c (sources of nikque.17, evaluation file of
#   Edax 4.5) and, with the evaluation file of eval2 (mobility weights), with the C tools of --c2.
#   --repeat n: run each case n times more, alternating C and C++, and print the CPU cycles and peak memory of both.
import argparse, ctypes, filecmp, os, random, re, shutil, struct, subprocess, sys
from ctypes import wintypes

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi')


class PMC(ctypes.Structure):
    _fields_ = [('cb', wintypes.DWORD), ('PageFaultCount', wintypes.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage')]


def run1(cmd, cwd, stdout, stderr):
    """Run cmd on logical CPU 0 at below-normal priority; return (exit code, cycles, peak working set in bytes)."""
    with open(stdout, 'wb') as o, open(stderr, 'wb') as e:
        p = subprocess.Popen(cmd, cwd=cwd, stdout=o, stderr=e, creationflags=0x4000)
        h = wintypes.HANDLE(int(p._handle))
        k32.SetProcessAffinityMask(h, ctypes.c_size_t(1))
        rc = p.wait()
        c = ctypes.c_ulonglong(0)
        k32.QueryProcessCycleTime(h, ctypes.byref(c))
        m = PMC(); m.cb = ctypes.sizeof(m)
        psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb)
    return rc, c.value, m.PeakWorkingSetSize


def prepare(a):
    """Inputs made once in the work directory (from the data of ev, which is only read)."""
    w = a.work
    os.makedirs(w, exist_ok=True)
    # a small book of form 1: the first 20000 positions of A_cut40.dat
    book = os.path.join(w, 'A_head.dat')
    if not os.path.exists(book):
        src = r'D:\edaxwork\data\A_cut40.dat'
        with open(src, 'rb') as f:
            hdr = bytearray(f.read(42))
            body = bytearray()
            for _ in range(20000):
                h = f.read(40)
                body += h + f.read(2 * (h[38] + 1))
        hdr[38:42] = struct.pack('<I', 20000)
        open(book, 'wb').write(hdr + body)
    # record files whose alignment is broken: junk bytes inserted in records of ss20 and ss21
    sp = os.path.join(w, 'spbroken')
    if not os.path.exists(sp):
        os.makedirs(sp)
        rnd = random.Random(1)
        for ply in (20, 21):
            data = bytearray(open(os.path.join(a.ev, 'sp8', f'ss{ply}.bin'), 'rb').read(17 * 3000))
            for _ in range(5):
                at = rnd.randrange(len(data))
                data[at:at] = bytes(rnd.randrange(256) for _ in range(rnd.randrange(1, 30)))
            open(os.path.join(sp, f'ss{ply}.bin'), 'wb').write(data)
    # weights for w2dat2: Rm_l100 of ev\pr (mob), and copies read as "mobp" (side to move only) and without scalars
    wd = os.path.join(w, 'w2in')
    if not os.path.exists(wd):
        os.makedirs(wd)
        pr = os.path.join(a.ev, 'pr')
        sc = open(os.path.join(pr, 'Rm_l100.scl'), 'rb').read()
        v = list(struct.unpack(f'<{len(sc) // 4}i', sc))
        for name, scl in (('Vp', [0, 0, 1, v[1]] + [0] * (len(v) - 4)), ('Vn', [0] * len(v))):
            shutil.copy(os.path.join(pr, 'Rm_l100.pat'), os.path.join(wd, name + '.pat'))
            open(os.path.join(wd, name + '.scl'), 'wb').write(struct.pack(f'<{len(scl)}i', *scl))
            for ply in range(2, 54):
                shutil.copy(os.path.join(pr, f'Rm_l100_{ply:02d}.w'), os.path.join(wd, f'{name}_{ply:02d}.w'))
    # small start files for the tools with the search: the first records of book40.bin and book20.bin
    for name, src, n in (('b40s.bin', 'book40.bin', 2000), (os.path.join('leafdata', 'book20.bin'), 'book20.bin', 20000)):
        p = os.path.join(w, name)
        if not os.path.exists(p):
            os.makedirs(os.path.dirname(p), exist_ok=True)
            open(p, 'wb').write(open(os.path.join(a.ev, 'data', src), 'rb').read(17 * n))
    # evsearch configurations
    open(os.path.join(w, 'cfg_E1.txt'), 'w').write('Rt c9 c10 s100 s101 l2 l3 l4 d8 d7 d6 d5 d4 mob\n')
    open(os.path.join(w, 'cfg_E2.txt'), 'w').write('Rs c9 l2 d4 mob mobp cmob qpar stab front disc estab mobhv mobd mob2d\n# comment\nRs2 c10 s101 mob\n')
    open(os.path.join(w, 'cfg_E3.txt'), 'w').write('Rb l2 d8\nRb2 s100\n')
    open(os.path.join(w, 'cfg_E4.txt'), 'w').write('Rn c10 s100 l3\n')


def cases(a):
    """name, tool, arguments ({OUT}: the output directory of the side, which is also the working directory, so that
    a file written there is compared too), regular expression of the parts of the standard output that may differ
    (times). The data of ev are given by paths relative to {OUT} (the data sources "prefix:dir:weight" of the C tools
    do not take a drive letter); label and gen read eval_orig.dat in the working directory: a copy is put there."""
    w = a.work
    e = os.path.relpath(a.ev, os.path.join(w, 'X', 'c'))         # ev seen from {OUT} (all at the same depth)
    common = ['-lib', f'{e}/patterns.txt', '-threads', '1']
    eo = f'{e}/eval_orig.dat'
    return [
        ('E1', 'evsearch', common + ['-configs', f'{w}/cfg_E1.txt', '-plies', '20,21', '-src', f'ss:{e}/sp8:1:100000', '-src', f'ega:{e}/egd:1:100000',
                                     '-iter', '10', '-prior', eo, '-lambda-prior', '100', '-dump', '{OUT}'], None),
        ('E2', 'evsearch', common + ['-configs', f'{w}/cfg_E2.txt', '-plies', '30', '-src', f'ss:{e}/sp8:1:60000', '-src', f'book:{e}/data:1:60000',
                                     '-iter', '10', '-teacher', eo, '-alpha', '0.5', '-dump', '{OUT}'], None),
        ('E3', 'evsearch', common + ['-configs', f'{w}/cfg_E3.txt', '-plies', '20,21', '-src', f'ss:{e}/sp8:1:80000',
                                     '-iter', '8', '-base', '../../E1base/c:Rt', '-dump', '{OUT}'], None),
        ('E4', 'evsearch', common + ['-configs', f'{w}/cfg_E4.txt', '-plies', '2,3,44', '-src', f'ss:{e}/sp8:1:50000', '-src', f'ega:{e}/egd:2:50000',
                                     '-iter', '6', '-notie', '-lambda', '50', '-prior', eo, '-dump', '{OUT}'], None),
        ('T1', 'evtrain', ['-init', eo, '-out', '{OUT}/t.dat', '-src', f'ss:{e}/sp8:1:50000', '-src', f'ega:{e}/egd:1:50000',
                           '-plies', '20-21', '-window', '1', '-iter', '10', '-calib', '0', '-threads', '1'], r'\s+\d+s$'),
        ('T2', 'evtrain', ['-init', eo, '-src', f'ss:{e}/sp8:1', '-plies', '30-30', '-eval', '-threads', '1', '-test', '16'], r'\s+\d+s$'),
        ('W1', 'w2dat2', [eo, f'{e}/pr', 'Rm_l100', '{OUT}/m.dat', '4.5.5'], None),
        ('W2', 'w2dat2', [eo, f'{w}/w2in', 'Vp', '{OUT}/p.dat'], None),
        ('W3', 'w2dat2', [eo, f'{w}/w2in', 'Vn', '{OUT}/n.dat'], None),
        ('S1', 'sprepair', [f'{w}/spbroken', '{OUT}', 'ss'], None),
        ('P1', 'sample', [f'{e}/sp8/ss20.bin', '200', '7'], None),
        ('B1', 'bookdump', [f'{w}/A_head.dat', '{OUT}'], None),
        ('B2', 'bookscan', [f'{w}/A_head.dat'], None),
        # tools with the search (C side: --c, or --c2 for the cases ending with 'e', which use the evaluation file of eval2)
        ('G1', 'selfplay3', ['{OUT}', '20', '5', '1', '-level', '4', '-exact', '10', '-eval', eo], r' in \d+ s$'),
        ('G1e', 'selfplay3', ['{OUT}', '20', '5', '1', '-level', '4', '-exact', '10', '-eval', a.eval2], r' in \d+ s$'),
        ('L1', 'label', [f'{e}/sp8/ss20.bin', '{OUT}/l.bin', '300', '1', '-seed', '3', '-level', '6'], r' in \d+ s$'),
        ('L1e', 'label', [f'{e}/sp8/ss20.bin', '{OUT}/l.bin', '300', '1', '-seed', '3', '-level', '6', '-eval', a.eval2], r' in \d+ s$'),
        ('O1', 'playout', [f'{e}/sp8/ss40.bin', '{OUT}', '30', '3', '1', '-level', '4', '-exact', '14', '-eval', eo], r' in \d+ s$'),
        ('O1e', 'playout', [f'{e}/sp8/ss40.bin', '{OUT}', '30', '3', '1', '-level', '4', '-exact', '14', '-eval', a.eval2], r' in \d+ s$'),
        ('N1', 'gen', [f'{w}/b40s.bin', '{OUT}', '5', '7', '1', '-last', '44'], r' in \d+ s$'),
        ('N1e', 'gen', [f'{w}/b40s.bin', '{OUT}', '5', '7', '1', '-last', '44', '-eval', a.eval2], r' in \d+ s$'),
        ('F1', 'leafgen', [f'{w}/leafdata', '20', '20', '10', '50', '1', '{OUT}', '-level', '6', '-eval', eo], r', \d+ s$'),
        ('F1e', 'leafgen', [f'{w}/leafdata', '20', '20', '10', '50', '1', '{OUT}', '-level', '6', '-eval', a.eval2], r', \d+ s$'),
        ('K1', 'evcheck', [eo, '300'], None),
    ]


NEEDS_EVAL_ORIG = {'label', 'gen'}	# the C tools open eval_orig.dat in the working directory (no -eval option)


def side_run(a, exe_dir, side, name, tool, args):
    out = os.path.join(a.work, name, side)
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    if tool in NEEDS_EVAL_ORIG:	# the evaluation file of the case under that name (the same on both sides)
        shutil.copy(a.eval2 if name.endswith('e') else os.path.join(a.ev, 'eval_orig.dat'), os.path.join(out, 'eval_orig.dat'))
    cmd = [os.path.join(exe_dir, tool + '.exe')] + [x.replace('{OUT}', out) for x in args]
    rc, cyc, peak = run1(cmd, out, os.path.join(a.work, name, side + '.out'), os.path.join(a.work, name, side + '.err'))
    return out, rc, cyc, peak


def same_text(f1, f2, mask, out1, out2):
    """Same contents, once the output directories of the two sides and the parts matched by mask are taken out."""
    t1 = open(f1, 'rb').read().replace(out1.encode(), b'{OUT}')
    t2 = open(f2, 'rb').read().replace(out2.encode(), b'{OUT}')
    if mask:
        r = re.compile(mask.encode(), re.M)
        t1 = r.sub(b'', t1.replace(b'\r\n', b'\n'))
        t2 = r.sub(b'', t2.replace(b'\r\n', b'\n'))
    return t1 == t2


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--c', required=True); ap.add_argument('--cpp', required=True); ap.add_argument('--work', required=True)
    ap.add_argument('--ev', default=r'D:\edaxwork\ev'); ap.add_argument('--cases', default=''); ap.add_argument('--repeat', type=int, default=0)
    ap.add_argument('--c2', default=''); ap.add_argument('--eval2', default='')
    a = ap.parse_args()
    prepare(a)
    want = set(a.cases.split(',')) if a.cases else None
    base = os.path.join(a.work, 'E1base', 'c')
    if (want is None or 'E3' in want) and not os.path.exists(os.path.join(base, 'Rt_21.w')):
        # the base of E3 (-base ../../E1base/c:Rt): the weights of E1 trained by the C tool
        os.makedirs(base, exist_ok=True)
        e1 = [c for c in cases(a) if c[0] == 'E1'][0]
        run1([os.path.join(a.c, 'evsearch.exe')] + [x.replace('{OUT}', base) for x in e1[2]], base,
             os.path.join(a.work, 'E1base', 'c.out'), os.path.join(a.work, 'E1base', 'c.err'))
    bad = 0
    for name, tool, args, mask in cases(a):
        if want is not None and name not in want:
            continue
        cdir = a.c
        if name.endswith('e'):
            if not (a.c2 and a.eval2):
                print(f'{name:3s} {tool:9s} skipped (no --c2 / --eval2)')
                continue
            cdir = a.c2
        if not os.path.exists(os.path.join(cdir, tool + '.exe')) or not os.path.exists(os.path.join(a.cpp, tool + '.exe')):
            print(f'{name:3s} {tool:9s} skipped (no exe)')
            continue
        oc, rcc, cc, pc = side_run(a, cdir, 'c', name, tool, args)
        op, rcp, cp, pp = side_run(a, a.cpp, 'cpp', name, tool, args)
        diffs = []
        if rcc != rcp:
            diffs.append(f'exit {rcc} / {rcp}')
        for s in ('out', 'err'):
            if not same_text(os.path.join(a.work, name, f'c.{s}'), os.path.join(a.work, name, f'cpp.{s}'), mask if s == 'out' else None, oc, op):
                diffs.append('std' + s)
        fc, fp = sorted(os.listdir(oc)), sorted(os.listdir(op))
        if fc != fp:
            diffs.append(f'files {len(fc)} / {len(fp)}')
        for f in fc:
            if f in fp and not filecmp.cmp(os.path.join(oc, f), os.path.join(op, f), shallow=False):
                diffs.append(f)
        size = sum(os.path.getsize(os.path.join(oc, f)) for f in fc)
        print(f'{name:3s} {tool:9s} exit {rcc} files {len(fc):3d} ({size:>11,d} bytes)  '
              + ('same' if not diffs else 'DIFFERENT: ' + ', '.join(diffs[:8])), flush=True)
        bad += bool(diffs)
        if a.repeat:
            cyc = {'c': [], 'cpp': []}
            peak = {'c': [], 'cpp': []}
            for r in range(a.repeat):
                order = (('c', cdir), ('cpp', a.cpp)) if r % 2 == 0 else (('cpp', a.cpp), ('c', cdir))
                for side, d in order:
                    _, _, cy, pk = side_run(a, d, side, name, tool, args)
                    cyc[side].append(cy); peak[side].append(pk)
            mc, mp = sorted(cyc['c'])[len(cyc['c']) // 2], sorted(cyc['cpp'])[len(cyc['cpp']) // 2]
            print(f'    cycles median C {mc / 1e9:.3f}G  C++ {mp / 1e9:.3f}G  (C++/C {mp / mc:.3f}); min C {min(cyc["c"]) / 1e9:.3f}G  C++ {min(cyc["cpp"]) / 1e9:.3f}G;'
                  f'  peak MiB C {max(peak["c"]) / 2**20:.1f}  C++ {max(peak["cpp"]) / 2**20:.1f}', flush=True)
    print(f'cases different: {bad}')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
