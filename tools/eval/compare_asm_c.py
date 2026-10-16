# Compare the machine code of a C tool (D:\edaxwork\ev\<tool>.c) and its C++ port (tools\eval\<tool>.cpp), function by
# function, from the assembly listings of MSVC (/FA): the instructions with the labels, addresses and names of the
# functions normalized. Only builds (no run), on one logical CPU.
# usage: py -3 compare_asm_c.py --out <dir> --tools evsearch,evtrain,... [--ev D:\edaxwork\ev]
import argparse, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
VSDEV = r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
C_OPTS = '/nologo /utf-8 /O2 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /arch:AVX512'
CPP_OPTS = '/nologo /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /utf-8 /O2 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /arch:AVX512'
OMP = {'evsearch', 'evtrain'}


def build_listing(src, opts, out, name):
    os.makedirs(out, exist_ok=True)
    asm = os.path.join(out, name + '.asm')
    cl = f'cl {opts} /c /FA /Fa"{asm}" /Fo"{os.path.join(out, name + ".obj")}" "{src}"'
    p = subprocess.Popen(f'cmd /c call "{VSDEV}" -arch=x64 -host_arch=x64 >nul && {cl}', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    import ctypes
    ctypes.windll.kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(p._handle)), ctypes.c_size_t(1))
    log = p.communicate()[0].decode('cp932', 'replace')
    if p.returncode:
        print(log); sys.exit(1)
    return asm


def functions(asm):
    """name -> normalized instructions"""
    funcs, cur, body = {}, None, []
    proc = re.compile(r'^(\S+)\s+PROC\b')
    for line in open(asm, encoding='cp932', errors='replace'):
        line = line.split(';')[0].rstrip()
        m = proc.match(line)
        if m:
            cur, body = m.group(1), []
            continue
        if cur and re.match(r'^\S+\s+ENDP\b', line):
            funcs[norm_name(cur)] = body
            cur = None
            continue
        if cur and line.strip():
            body.append(norm_ins(line.strip()))
    return funcs


def norm_name(n):
    n = re.sub(r'^\?', '', n)
    n = re.sub(r'@@.*$', '', n)           # C++ decoration
    n = re.sub(r'@evtool$', '', n)
    return n


def norm_ins(s):
    s = re.sub(r'\?\?_C@[\w@$?]+', 'STR', s)                # string literals
    s = re.sub(r'\?([\w$]+)@[\w@$?]*', r'\1', s)             # decorated C++ names -> plain
    s = re.sub(r'\$LN\d+@[\w$]+|\$L\w+|\$\w+@\d+', 'L', s)  # labels
    s = re.sub(r'\btv\d+', 'tv', s)                          # temporaries of the compiler
    s = re.sub(r'\b\w+\$\d+\[', 'V[', s)                     # names of the local variables
    s = re.sub(r'\s+', ' ', s)
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True); ap.add_argument('--tools', required=True); ap.add_argument('--ev', default=r'D:\edaxwork\ev')
    a = ap.parse_args()
    for t in a.tools.split(','):
        omp = ' /openmp' if t in OMP else ''
        fc = functions(build_listing(os.path.join(a.ev, t + '.c'), C_OPTS + omp, a.out, t + '-c'))
        fp = functions(build_listing(os.path.join(HERE, t + '.cpp'), CPP_OPTS + omp + f' /I"{HERE}"', a.out, t + '-cpp'))
        same = [f for f in fc if f in fp and fc[f] == fp[f]]
        diff = [f for f in fc if f in fp and fc[f] != fp[f]]
        only_c = [f for f in fc if f not in fp]
        only_p = [f for f in fp if f not in fc]
        print(f'{t}: functions same {len(same)}, different {len(diff)}, only in C {len(only_c)}, only in C++ {len(only_p)}')
        for f in diff:
            print(f'  different: {f} ({len(fc[f])} / {len(fp[f])} instructions)')
        if only_c: print('  only in C:', ' '.join(only_c[:20]))
        if only_p: print('  only in C++:', ' '.join(only_p[:20]))


if __name__ == '__main__':
    main()
