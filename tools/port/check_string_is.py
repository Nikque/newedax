#!/usr/bin/env python3
"""Check that the string_is replacements of a commit keep the logic of the strcmp chains.

usage: check_string_is.py <repo> <commit>
Both versions of each changed file are normalized: every strcmp(S, W) comparison and every
string_is(S, W...) becomes a group G+[S|W,...] (S is one of the words) or G-[S|W,...] (S is none
of them); chains "== 0 ||" (and "!= 0 &&") of the same S become one group. Then the files are
compared without spaces and without parentheses around a single group.
"""
import re
import subprocess
import sys


def git(repo, *args):
    return subprocess.run(['git', '-C', repo] + list(args), capture_output=True, check=True).stdout.decode('utf-8', 'replace')


def split_args(text, start):
    """text[start] is just after '(': return (list of top-level args, index after ')')."""
    depth, i, args, cur = 0, start, [], []
    in_str = None
    while i < len(text):
        c = text[i]
        if in_str:
            cur.append(c)
            if c == '\\':
                cur.append(text[i + 1]); i += 2; continue
            if c == in_str:
                in_str = None
        elif c in '"\'':
            in_str = c; cur.append(c)
        elif c == '(':
            depth += 1; cur.append(c)
        elif c == ')':
            if depth == 0:
                args.append(''.join(cur).strip())
                return args, i + 1
            depth -= 1; cur.append(c)
        elif c == ',' and depth == 0:
            args.append(''.join(cur).strip()); cur = []
        else:
            cur.append(c)
        i += 1
    raise ValueError('unbalanced')


SP = re.compile(r'\s*')


def atoms(text):
    """Replace strcmp comparisons and string_is calls by tokens \x01sign|S|W1,W2\x02."""
    out, i = [], 0
    pat = re.compile(r'(!\s*)?\b(strcmp|string_is|option_is)\s*\(')
    while True:
        m = pat.search(text, i)
        if not m:
            out.append(text[i:]); break
        out.append(text[i:m.start()])
        args, j = split_args(text, m.end())
        neg = bool(m.group(1))
        if m.group(2) in ('string_is', 'option_is'):
            sign = '-' if neg else '+'
            out.append('\x01%s|%s|%s\x02' % (sign, args[0], ','.join(args[1:])))
            i = j
            continue
        rest = re.match(r'\s*(==|!=)\s*0\b', text[j:])
        if neg:
            sign = '+'                      # !strcmp(S, W): S is W
            if rest: raise ValueError('! with compare: ' + text[m.start():j + 10])
        elif rest:
            sign = '+' if rest.group(1) == '==' else '-'
            j += rest.end()
        else:
            sign = '-'                      # strcmp(S, W) as a truth value: S is not W
        out.append('\x01%s|%s|%s\x02' % (sign, args[0], args[1]))
        i = j
    return ''.join(out)


TOK = r'\x01([+-])\|([^|]*)\|([^\x02]*)\x02'


def group(text, flagged):
    """Join chains of tokens of the same subject: + with ||, - with &&."""
    tok = re.compile(TOK)
    changed = True
    while changed:
        changed = False
        for m in tok.finditer(text):
            sign, s, w = m.groups()
            op = r'\|\|' if sign == '+' else '&&'
            n = re.match(r'\s*' + op + r'\s*' + TOK, text[m.end():])
            if n and n.group(1) == sign and n.group(2) == s:
                before = text[:m.start()].rstrip()[-2:]
                other = '&&' if sign == '+' else '||'
                if before == other:
                    flagged.append(text[max(0, m.start() - 60):m.end() + n.end() + 20])
                text = text[:m.start()] + '\x01%s|%s|%s,%s\x02' % (sign, s, w, n.group(3)) + text[m.end() + n.end():]
                changed = True
                break
    return text


def strip(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'template\s*<class\.\.\.\s*\w+>\s*static inline bool (string_is|option_is)\s*\([^)]*\)\s*\{[^}]*\}', '', text)


def normalize(text, flagged):
    text = group(atoms(strip(text)), flagged)
    text = re.sub(r'\s+', '', text)
    prev = None
    while prev != text:
        prev = text
        text = re.sub(r'\((\x01[^\x02]*\x02)\)', r'\1', text)
    return text


def main(repo, commit):
    files = git(repo, 'diff', '--name-only', commit + '^', commit, '--', 'src').split()
    bad = 0
    n_groups = 0
    for f in files:
        flagged = []
        old = normalize(git(repo, 'show', '%s^:%s' % (commit, f)), flagged)
        new = normalize(git(repo, 'show', '%s:%s' % (commit, f)), flagged)
        n_groups += new.count('\x01')
        if f.endswith('util.h'):
            new = re.sub(r'template<class\.\.\.Words>staticinlineboolstring_is.*?\}', '', new, flags=re.S)
        if old != new:
            bad += 1
            # show the first difference
            k = next(i for i in range(min(len(old), len(new))) if old[i] != new[i]) if old[:min(len(old), len(new))] != new[:min(len(old), len(new))] else min(len(old), len(new))
            print('DIFF', f, '\n  old:', old[max(0, k - 150):k + 150].replace('\x01', '<').replace('\x02', '>'), '\n  new:', new[max(0, k - 150):k + 150].replace('\x01', '<').replace('\x02', '>'))
        for x in flagged:
            print('PRECEDENCE?', f, x.replace('\x01', '<').replace('\x02', '>').replace('\n', ' '))
    print(f'{len(files)} files, {bad} with differences, {n_groups} groups in the new files')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
