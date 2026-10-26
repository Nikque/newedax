# newedax: tests and measurements

Tests and measurements done for each version, moved without changes from the former README-NIKQUE.en.md.

For what changed, see the [release notes](../../RELEASE-NOTES.md). "Above" and "below" in the text refer to positions in the former document.


## v4.5.5-newedax.8

Checks:

- The tests of the stages taken over and of the hint (the tests of the C version, `tests/book-analyze.cpp`): for 502,470 moves of 200 random books, the stages taken over are those walked from each position alone; the same on 2 test books (about 4.78 million moves on the book of 409,494 positions); the 24 cases of the hint pass. The answers of `book moveinfo` are the same as those of v4.5.5-newedax.7 and of the C version (2 test books, each position of 2 games, 3 bases).
- The tests of the analysis commands (the comparisons on 3 small books, the real `book deviate` family, 40,000 random graphs of positions, the background thread, the lines of `book bestlines` and `book findline`) all pass with the released `wEdax-x86-64-v3.exe`; so do the `book reduce` tests, 500 cases, and the 14 tests of damaged books and wrong words.
- The answers of 10 commands on the same positions (6,880 answers) are all the same as those of the C version v4.5.5-nikque.23 (except the time).
- Search: the 5 Windows release programs solve fforum-20-39 with 1 thread with the same results and node counts as v4.5.5-newedax.7: 78,733,223 with the `eval.dat` of eval2, 77,639,567 with that of Edax 4.5; the 7 Cassio cases pass. The 3 DLLs: 206 API checks, 0 failures, with both `eval.dat`. The 14 eval2 tools: built and tested without failure (compared with the C tools, only the seconds of a progress line of `label` differ; the files made are the same). No program, DLL or Android binary holds a folder of the build machine. The 4 Android binaries: no warning (not run on a device).
- The search, the evaluation, the book learning and the file format are not changed (the code changed finds the precision stages of `book moveinfo`, and writes the warning of an unknown command).
- Size (against v4.5.5-newedax.7): `wEdax-x86-64-v4.exe` (no PGO) +4,096 bytes, the 4 PGO programs +2,560 to +4,096 bytes, the 3 DLLs the same size; the Android programs +2,544 and +4,352 bytes, the libraries +2,392 and +4,368 bytes.
- The speed was not measured.

## v4.5.5-newedax.7

Checks:

- The tests of `book reduce` and `book reducecount` (the tests of the C version) all agree, 546 cases (with 4 threads and 1 thread, with the count first, with the settings changed). The books saved after `book reduce` with the executables of v4.5.5-newedax.5 and of this version are the same byte for byte in 54 cases, with the count first too (but for the save date). The tests of damaged books and wrong words (14 cases) pass.
- The tests of the analysis commands (the comparisons on 3 small books, the real `book deviate` family, 40,000 random graphs of positions, the background thread, the lines of `book bestlines` and `book findline`) all pass with the released `wEdax-x86-64-v3.exe`; so do the `book reduce` tests, 500 cases, and the 14 tests of damaged books and wrong words.
- The answers of 10 commands on the same positions (6,880 answers) are all the same as those of the C version v4.5.5-nikque.22 (except the time).
- Search: the 5 Windows release programs solve fforum-20-39 with 1 thread with the same results and node counts as v4.5.5-newedax.6: 78,733,223 with the `eval.dat` of eval2, 77,639,567 with that of Edax 4.5; the 7 Cassio cases pass. The 3 DLLs: 206 API checks, 0 failures, with both `eval.dat`. The 14 eval2 tools: built and tested without failure, and the files made in the 26 comparisons with the C tools are all the same. No program, DLL or Android binary holds a folder of the build machine. The 4 Android binaries: no warning (not run on a device).
- The search, the evaluation, the book learning and the file format are not changed (the code changed runs only when `book reduce` or `book reducecount` is typed, or reads their settings).
- Size (against v4.5.5-newedax.6): `wEdax-x86-64-v4.exe` (no PGO) +6,144 bytes, the 4 PGO programs +3,584 to +4,608 bytes, the 3 DLLs +1,024 to +1,536 bytes. Android programs +3,512 and +6,408 bytes, libraries +3,624 and +6,552 bytes.
- With the real 29 GB book, the speed of `book reducecount` was measured with the C version only (the nikque.22 section).

## v4.5.5-newedax.6

Checks:

- The tests of `book reduce` and `book reducecount` (the tests of the C version) all agree, 546 cases, with 4 threads and with 1 thread. The books saved after `book reduce` with the executables of v4.5.5-newedax.5 and of this version are the same byte for byte in 54 cases (but for the save date). The tests of a damaged book and of wrong words (the 14 cases of the C version) all pass.
- The tests of the analysis commands (the comparisons on 3 small books, the real `book deviate` family, 40,000 random graphs of positions, the background thread, the lines of `book bestlines` and `book findline`) all pass with the released `wEdax-x86-64-v3.exe`; so do the `book reduce` tests, 500 cases, and the 14 tests of damaged books and wrong words.
- The answers of 10 commands on the same positions (6,880 answers) are all the same as those of the C version v4.5.5-nikque.21 (except the time).
- Search: the 5 Windows release programs solve fforum-20-39 with 1 thread with the same results and node counts as v4.5.5-newedax.5: 78,733,223 with the `eval.dat` of eval2, 77,639,567 with that of Edax 4.5; the 7 Cassio cases pass. The 3 DLLs: 206 API checks, 0 failures, with both `eval.dat`. The 14 eval2 tools: built and tested without failure (against the C tools, only the seconds of the progress line of `label` differ; the files made are the same; one test of the tools ran out of time once while the CPU was busy, and passed when run again). No program, DLL or Android binary holds a folder of the build machine. The 4 Android binaries: no warning (not run on a device).
- The search, the evaluation, the book learning and the file format are not changed (the code changed runs only when `book reduce` or `book reducecount` is typed).
- Size (against v4.5.5-newedax.5): `wEdax-x86-64-v4.exe` (no PGO) +8,704 bytes, the 4 PGO programs +4,608 to +5,120 bytes, the 3 DLLs the same size. Android programs +4,144 and +7,216 bytes, libraries +4,144 and +7,216 bytes.
- Not checked with the 29 GB real book.

## v4.5.5-newedax.5

Checks:

- The tests of `book reduce` and `book reducecount` (the tests of the C version: 3 small books made for the tests, compared with a computation written apart) all agree, 529 cases.
- The tests of the analysis commands (the comparisons on 3 small books, the real `book deviate` family, 40,000 random graphs of positions, the background thread, the lines of `book bestlines` and `book findline`) all pass with the released `wEdax-x86-64-v3.exe`; so do the `book reduce` tests, 500 cases.
- The answers of 10 commands on the same positions (6,880 answers) are all the same as those of the C version v4.5.5-nikque.20 (except the time).
- Search: the 5 Windows release programs solve fforum-20-39 with 1 thread with the same results and node counts as v4.5.5-newedax.4: 78,733,223 with the `eval.dat` of eval2, 77,639,567 with that of Edax 4.5; the 7 Cassio cases pass. The 3 DLLs: 206 API checks, 0 failures, with both `eval.dat`. The 14 eval2 tools: built and tested without failure (against the C tools, only the seconds of the progress line of `label` differ; the files made are the same). No program, DLL or Android binary holds a folder of the build machine. The 4 Android binaries: no warning (not run on a device).
- Speed: the search, the evaluation, the book learning and the file format are not changed (the code added runs only when a new command is typed). The speed of the new commands was not measured.
- Size (against v4.5.5-newedax.4): `wEdax-x86-64-v4.exe` (no PGO) +7,680 bytes, the 4 PGO programs +3,072 to +4,096 bytes, the 3 DLLs the same size. Android programs +3,120 and +7,056 bytes, libraries +3,248 and +7,104 bytes.
- Not checked with the 29 GB real book.

## v4.5.5-newedax.4

Checks:

- The tests of the analysis commands (the comparisons on 3 small books, 164 runs of the real `book deviate`, 40,000 random graphs of positions, the background thread) all pass.
- The answers of 10 commands on the same positions (6,880 answers) are all the same as those of the C version v4.5.5-nikque.19 (except the time).
- Search: 17 positions with 1 thread, level 14 and no book give the same depths, scores, node counts (7,676,900 in all) and principal variations as v4.5.5-newedax.2 (whose search v4.5.5-newedax.3 keeps). The 5 Windows release programs solve fforum-20-39 with 1 thread with the same results and node counts as v4.5.5-newedax.3: 78,733,223 with the `eval.dat` of eval2, 77,639,567 with that of Edax 4.5; the 7 Cassio cases pass. The 3 DLLs: 206 API checks, 0 failures, with both `eval.dat`. The 14 eval2 tools: built and tested without failure (against the C tools, only the seconds of the progress line of `label` differ; the files made are the same). No program, DLL or Android binary holds a folder of the build machine. The 4 Android binaries: no warning (not run on a device).
- Speed and memory of the new commands (a test book of 19.1 million positions, 1 thread, alternating with the C version): the number of positions from the start 5.03 s (C version 5.92 s), umigame numbers with loss 2/4 1.62 s (1.82 s), `book missing` deviate2 2 4 0.30 s (0.36 s). The analyses take 83 MB more memory (C version 86 MB); the book alone takes 880 MB (C version 1,235 MB). No command was slower than in the C version.
- Size (against v4.5.5-newedax.3): `wEdax-x86-64-v4.exe` (no PGO) +60,416 bytes, the 4 PGO programs +39,936 to +41,472 bytes, the 3 DLLs the same size. Android programs +31,568 and +52,288 bytes, libraries +32,032 and +53,152 bytes (the unused analysis code stays).
- Not checked with the 29 GB real book.

## v4.5.5-newedax.3

### Speed and memory (2026-10-19 to 20, measured)

One thread (one logical CPU, CPU cycles, 16 to 48 alternating pairs, ratio to the C version v4.5.5-eval2.6; in parentheses with the eval.dat of Edax 4.5, ratio to v4.5.5-nikque.18; the same node counts). Above 1, new edax is slower:

| Executable | fforum-20-39 | 30 middle-game positions (level 18) |
|---|---|---|
| `wEdax-x86-64-v4.exe` (no PGO) | 1.000 | **0.961 to 0.969** (0.979 to 0.981) |
| `wEdax-x86-64-v3.exe` (PGO) | 0.996 to 1.007 (0.998) | **1.008 to 1.011** (1.014 to 1.021) |
| `wEdax-x86-64.exe` (PGO) | 0.999 to 1.005 (1.002) | 1.000 to 1.005 (1.004) |
| `wEdax-x86-sse.exe` (PGO) | 0.990 to 0.991 (0.989) | 0.934 to 0.936 (0.953) |
| `wEdax-x86.exe` (PGO) | 0.921 to 0.926 (0.988) | 0.992 to 0.998 (0.985) |

32 threads (32 logical CPUs, book commands, ratio of the times to the C version v4.5.5-eval2.6, 6 rounds in the order A B B A):

| Command | `-v4` | `wEdax-x86-64.exe` | `-v3` |
|---|---|---|---|
| suite F (the `book deviate` family, level 21) | 0.978 | 0.987 | 0.999 |
| `book learn` (30 games, level 18) | 0.993 | 0.998 | **1.014** |
| `book fill 2` (level 12, a book of 270,000 positions) | 0.949 | 0.988 | **1.010** |
| `book fill2 2` | 0.960 | 0.990 | 1.006 |
| `book leaf expand` | 0.976 | 1.001 | 1.007 |
| `book fill2 2 5 5` | 0.978 | 0.999 | **1.013** |
| `book import` (a text book of 4 million positions) | 0.966 | 0.966 | 0.976 |

- **`wEdax-x86-64-v3.exe` stays about 1 % slower than the C version** (one-thread middle game, 32-thread `book learn` and `fill`). In its PGO build, 3 functions (the flips among them) stay called instead of inlined, and the evaluation function, with the same machine code, takes longer than in the C version. None of 9 tried changes helped (`docs/port/MEASURE-2.md`, chapter 2). On a CPU with AVX-512, use `wEdax-x86-64-v4.exe` (faster than the C version).
- Peak memory: the same as the C version or less (`book import` 614 -> 545 MiB).
- Loading a book of 34.7 million positions (made by the C version) from an HDD: 0.914 times the time of the C version, CPU time 8.3 -> 3.8 s, peak memory 3,243 -> 1,667 MiB.
- The eval2 tools: `w2dat2` and `label` about 0.7 % slower than the C tools (cause not found); the others the same or faster.

### Checks

- The 5 Windows executables: one-thread `-solve` of fforum-20-39 gives the same depths, scores, node counts and lines as v4.5.5-eval2.5 with `bin/data/eval.dat` (eval2; 78,733,223), and as v4.5.5-nikque.17 with the `eval.dat` of Edax 4.5 (77,639,567). The 7 Cassio cases pass.
- The 3 DLLs: 206 API checks, 0 failures each, with both `eval.dat`. The same machine code as v4.5.5-newedax.2.
- The books of 14 test suites (the machine code of `wEdax-x86-64-v4.exe`, one thread and 32 threads): the same as the records; only the one-thread run of S3 with the default settings differs, as its book seed is the clock (as before).
- The 14 eval2 tools: all tests pass; on 26 cases their outputs are the same as those of the C tools.
- Android (NDK r27d, armeabi-v7a and arm64-v8a): built without warnings, the same size as v4.5.5-newedax.2; not run on a device.
- None of the 26 executables, libraries, tools and Android binaries holds a folder of the build machine (`tests/check-build-paths.py`).
- Size: `-v4` 824,832, `-v3` 698,368, `wEdax-x86-64.exe` 715,264, `wEdax-x86-sse.exe` 642,560, `wEdax-x86.exe` 692,224 bytes (the 4 PGO builds 48 to 144 KB smaller than in v4.5.5-newedax.2); the DLLs the same. Against the C version v4.5.5-eval2.6: the 4 PGO builds +18 to 30 KB, `-v4` +163 KB (no PGO).
- The released `-v3`, `wEdax-x86-sse.exe` and `wEdax-x86.exe` differ a little from the measured builds (a PGO build changes from a build to the next), so the released ones were measured again against the C version (the table above). `-v4`, `wEdax-x86-64.exe` and the DLLs have the machine code of the measured ones.

## v4.5.5-newedax.2

### Speed and memory (2026-10-19, measured)

Windows, the released `wEdax-x86-64-v4.exe` against each other, 32 threads (32 logical CPUs), the `config.ini` of the release. Ratio of the times (the newer one / the one compared; below 1 is faster): the mean and standard deviation of 6 rounds in the order A B B A.

| Command | against v4.5.5-newedax.1 | against the C version v4.5.5-eval2.6 |
|---|---|---|
| suite F (`book deviate`, `deviate2`, `deviate3`, `enhance`, level 21) | 1.006 ± 0.014 | **0.985 ± 0.012** |
| `book learn` (30 games, level 18) | 0.997 ± 0.009 | 1.001 ± 0.022 |
| `book fill 2` (level 12, a book of 270,000 positions) | 0.988 ± 0.009 | **0.959 ± 0.008** |
| `book fill2 2` | 0.999 ± 0.012 | **0.973 ± 0.011** |
| `book leaf expand` | 0.991 ± 0.012 | **0.967 ± 0.010** |
| `book fill2 2 5 5` | 0.987 ± 0.013 | 0.979 ± 0.024 |
| `book import` (a text book of 4 million positions) | 0.996 ± 0.020 | **0.963 ± 0.010** |
| fforum-20-39 (`-solve`) | — | 0.995 ± 0.028 |
| 30 middle-game positions (level 18, `-solve`) | — | 0.986 ± 0.011 |

- With 32 threads and `wEdax-x86-64-v4.exe`, **no case slower than v4.5.5-newedax.1 or than the C version was found** (every difference is within the standard deviation or faster; the 1.006 of suite F against newedax.1 has a median of 1.002, 3 of the 6 rounds slower). The C version is a PGO build, new edax is not.
- Peak memory: the same as v4.5.5-newedax.1 (±1 MiB); the same as the C version v4.5.5-eval2.6 or less (`book import`: 614 MiB → 544 MiB).
- **With one thread, the 4 executables other than `wEdax-x86-64-v4.exe` are slower than the C version** (one logical CPU, CPU cycles, 16 alternating pairs, ratio to the C version v4.5.5-eval2.6, the same node counts):

  | Executable | fforum-20-39 | 30 middle-game positions (level 18) |
  |---|---|---|
  | `wEdax-x86-64-v4.exe` | 1.000 | 0.966 |
  | `wEdax-x86-64-v3.exe` | **1.008** | **1.017** |
  | `wEdax-x86-64.exe` | **1.008** | **1.025** |
  | `wEdax-x86-sse.exe` | **1.010** | **1.116** |
  | `wEdax-x86.exe` | 0.936 | **1.047** |

  The releases of the C version are PGO builds, new edax is not. Without PGO on both sides (the same options), new edax is as fast or faster (middle game: `-v3` 0.992, `wEdax-x86-64.exe` 0.998). A PGO build of new edax narrows the gap, but the middle game is still slower than the C version (`-v3` 1.008, `wEdax-x86-64.exe` 1.013, `wEdax-x86-sse.exe` 1.026): PGO inlines other functions into their callers than in the C version (`get_moves` and `search_eval_1` are inlined into `NWS_shallow` in the C version, seen by sampling); this is presumably the cause (not checked). The next version fixes it.
- `wEdax-x86.exe` (32-bit, no SSE) is as fast as in v4.5.5-newedax.1 (0.9995, one logical CPU, 40 pairs).
- Size: the Windows executables are 74 to 173 KB larger than those of the C version v4.5.5-eval2.6, the DLLs about 27 KB. Most of it comes from the build without PGO (without PGO on both sides: about 32 KB larger: the new features such as the files of form 2, the sort and the tables of Boost).

### Checks

- The 5 Windows executables: one-thread `-solve` of fforum-20-39 gives the same depths, scores, node counts and lines as v4.5.5-eval2.5 with `bin/data/eval.dat` (eval2; 78,733,223 nodes), and as v4.5.5-nikque.17 with the `eval.dat` of Edax 4.5 (77,639,567). The 7 Cassio cases pass.
- The 3 DLLs: 206 API checks, 0 failures each, with both `eval.dat`.
- The 14 eval2 tools: 26 tests pass; on 26 cases their outputs are the same as those of the C tools.
- No executable, library or tool holds a folder of the build machine.
- Android (NDK r27d, armeabi-v7a and arm64-v8a): built without warnings; not run on a device.
- Size: the same as v4.5.5-newedax.1 (`libedax-x64.dll` 512 bytes smaller). Android: 288 to 448 bytes larger (the code of the fixes).

## v4.5.5-newedax.1

Checks:

- With the same options as the C version v4.5.5-nikque.17 (no PGO): no case slower than the C version in fforum-20-39 (1 and 32 threads), fforum-40-59, 30 middle-game positions (levels 18 and 21), suite F, or the start (`docs/port/MEASURE-1.md`).
- The same books as the C version in 14 test suites (with the same random seed), the same C API of libedax and Cassio protocol; 53 regression cases.

Not checked yet (also for newedax.2): builds other than MSVC on Windows and the Android NDK, an Android device, AddressSanitizer and ThreadSanitizer, real books of billions of positions, a review by a second reader.
