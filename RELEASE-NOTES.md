# newedax release notes

[日本語](RELEASE-NOTES.ja.md) · [README](README.md) · [Manual](manual/en/README.md) · [Tests and measurements](docs/validation/newedax.en.md)

User-facing changes of each version of newedax (the eval2 line of the C Edax 4.5.5 nikque, ported to C++23). Detailed test and measurement records are linked at the end of each entry.

For the details of changes taken from the C version, see the [C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md).

## Relation to the C versions

Each version contains all changes of the C versions on the same row.

| newedax | C (nikque) | C (eval2) |
|---|---|---|
| newedax.8 | nikque.23 | eval2.11 |
| newedax.7 | nikque.22 | eval2.10 |
| newedax.6 | nikque.21 | eval2.9 |
| newedax.5 | nikque.20 | eval2.8 |
| newedax.4 | nikque.19 | eval2.7 |
| newedax.3 | nikque.18 | eval2.6 |
| newedax.2 | nikque.18 | eval2.6 |
| newedax.1 | nikque.17 | eval2.5 |

---

## v4.5.5-newedax.8

As in the C version, the precision of the candidate moves of the GUI (`stage=` of `book moveinfo`) is taken over from the answers of the positions before and after, so that less is walked. A book command typed without `book` gets a hint on how to type it. The manual and the documents were rewritten.

**Changes**

- The changes of C version v4.5.5-nikque.23 are included ([C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque23)).
  - What the kept stage of the current position and the kept stages of the next positions of the position after a move already tell is not walked again. The answers (`stage=`, `missing=`) are the same as in v4.5.5-newedax.7.
  - A book command typed at the top level (`leaf expand 30 30`, `fill2 2`, …) adds `(book command: type "book leaf expand 30 30" or "b leaf expand 30 30")` to the warning. Names that are also top-level commands, settings or `base` commands get no hint.
- Manual: the same review as the C version (the wording, every book command in the form `book …`, the time of a save in 9.4: 18 to 28 seconds, the note on `refit` in chapter 4 links to nikque.5 in the C release notes). 12.4 and 13.2 describe the two changes above.
- Documents: the README and the release notes were rewritten, the test and measurement records moved to `docs/validation/`, and `README-NIKQUE.*` was removed. The package holds `README.md` instead of `README-NIKQUE.*`. The first comment lines of `config.ini` point to chapter 4 of the manual.

**Compatibility**

- The answers of `book moveinfo` are the same as in v4.5.5-newedax.7.
- The warning line of an unknown command gets a hint at its end only for the names of book commands (its first part is the same).
- The search results, `eval.dat` and the book file format do not change.

**Checked**: with the same tests as the C version (`tests/book-analyze.cpp`), the stages taken over are the stages walked from each position alone, for every move (200 random books and 2 test books). The answers of `book moveinfo` are the same as those of v4.5.5-newedax.7 and of the C version (2 test books, each position of 2 games, 3 bases).

**Not checked**: the speed on a real book, the behaviour in the GUI, Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax8)

---

## v4.5.5-newedax.7

`book reduce` and `book reducecount` are now as fast and as light on memory as in the C version.

**Changes**

- The changes of the C version v4.5.5-nikque.22 are included ([the C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque22)).
  - `book reducecount` is faster, and a `book reduce` right after it reuses what was counted. After the cut, the memory no longer used is freed at once.
  - New settings `book-reduce-index`, `book-reduce-reuse`, `book-reduce-shrink` and `book-reduce-walk` were added (all of them work as they are by default).
- The answers of the commands are those of the C version v4.5.5-nikque.22.

**Compatibility**

- The positions kept and what each position holds are those of v4.5.5-newedax.6.
- The results of the search, `eval.dat` and the book file format are not changed.

**Not checked**: the speed in newedax (the speed of `book reducecount` on the real book of 29 GB was measured only with the C version), real Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax7)

---

## v4.5.5-newedax.6

`book reduce` and `book reducecount` get the same fixes and the same new way of working as the C version.

**Changes**

- The changes of the C version v4.5.5-nikque.21 are included ([the C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque21)).
  - `book reducecount` now runs in parallel with the `n-tasks` threads.
  - `book reducecount` did not end with a damaged book, and positions were removed even with wrong input. Both were fixed (now an error or a warning is shown and the book is not changed).
- The answers of the commands are those of the C version v4.5.5-nikque.21.

**Compatibility**

- With a sound book, the positions kept and the book saved after `book reduce` are those of v4.5.5-newedax.5.
- The results of the search, `eval.dat` and the book file format are not changed.

**Not checked**: the real book of 29 GB, real Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax6)

---

## v4.5.5-newedax.5

The command `book reduce`, which makes the book smaller, and `book reducecount`, which counts first, were added.

**Changes**

- The changes of the C version v4.5.5-nikque.20 are included ([the C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque20)).
  - `book reduce X Y` keeps only the positions in the range that `book deviate3 X Y` walks, and removes the others. `book reducecount` counts how many are kept and how many are removed (the book is not changed).
- The commands and answers are those of the C versions v4.5.5-nikque.20 and v4.5.5-eval2.8.
- How to use them: [Manual 8.4](manual/en/08-book-maintenance.md), [12.4](manual/en/12-integration.md)

**Compatibility**: the results of the search, `eval.dat` and the book file format are those of v4.5.5-newedax.4.

**Not checked**: the speed of the new commands, the real book of 29 GB, real Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax5)

---

## v4.5.5-newedax.4

Book analysis commands for GUI programs were added.

**Changes**

- The changes of the C version v4.5.5-nikque.19 are included ([the C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque19)).
  - New commands: `book moveinfo`, `book missing`, `book levels`, `book bestlines`, `book findline`, and `stop id=N`.
- The commands and answers are those of the C versions v4.5.5-nikque.19 and v4.5.5-eval2.7.
- None of the new commands is slower than in the C version (a book of 19.14 million positions, 1 thread: counting the positions from the first move takes 5.03 s against 5.92 s for the C version). The working memory of the book alone is 880 MB against 1,235 MB for the C version.
- The Windows executables are built as in v4.5.5-newedax.3 (`wEdax-x86-64-v4.exe` without PGO, the other 4 with PGO).
- How to use them: [Manual 12.4](manual/en/12-integration.md)

**Compatibility**: the results of the search, `eval.dat` and the book file format are those of v4.5.5-newedax.3.

**Not checked**: the real book of 29 GB, real Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax4)

---

## v4.5.5-newedax.3

The way the Windows executables are built is changed, so that their speed with 1 thread matches the C version. The first full release that is not a pre-release.

**Changes**

- Of the Windows executables, `wEdax-x86-64-v4.exe` is now built without PGO (optimization that uses records of runs), and the other 4 with PGO (`tools/port/build-release.cmd`). In v4.5.5-newedax.2, none used PGO.
- `wEdax-x86-64-v4.exe` is faster without PGO, so its machine code stays that of v4.5.5-newedax.2. The 3 DLLs and Android are without PGO too, and their machine code is not changed.
- Speed (ratio against the C version v4.5.5-eval2.6): in the midgame with 1 thread, `wEdax-x86-64-v4.exe` is about 3 to 4% faster than the C version (0.961 to 0.969). `wEdax-x86-64.exe`, `wEdax-x86-sse.exe` and `wEdax-x86.exe` are about as fast as the C version or faster.
- **`wEdax-x86-64-v3.exe` is still about 1% slower than the C version** (the midgame with 1 thread; `book learn` and the `fill` commands with 32 threads). On AVX-512 CPUs, use `wEdax-x86-64-v4.exe`.
- The peak memory is the same as the C version or less (`book import` 614 → 545 MiB). When loading a book of 34.73 million positions made by the C version, the peak memory is 3,243 → 1,667 MiB.
- Of the eval2 tools, `w2dat2` and `label` are about 0.7% slower than the C tools. The others are as fast or faster.
- Record of the measurements: `docs/port/MEASURE-2.md`.

**Compatibility**

- The results of the search are those of v4.5.5-newedax.2 (and of v4.5.5-eval2.6).
- `eval.dat` and the book file format are not changed.

**Notes**: with PGO, the machine code changes a little with each build. The released `-v3`, `wEdax-x86-sse.exe` and `wEdax-x86.exe` are the very files that were compared with the C version again.

**Not checked**: why `wEdax-x86-64-v3.exe`, `w2dat2` and `label` are slower, real Android devices.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax3)

---

## v4.5.5-newedax.2

The errors found by the cross-check of v4.5.5-newedax.1 were fixed. Some executables are still slower than the C version with 1 thread, so this is still a pre-release.

**Changes**

- The executables and libraries no longer hold the folder in which they were built (the same fix as the C version v4.5.5-nikque.18; [the C release notes](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.md#v455-nikque18)). Error messages now show only the file name of the source.
- `vcomp140.dll` (the MSVC OpenMP runtime library) is now in `tools/eval` of the ZIP. 7 tools use it.
- The tool scripts take the input folders as arguments (`--ev`, `--book40`, `-Inputs`; no default value).
- The Windows executables are without PGO, as in v4.5.5-newedax.1.
- Speed (`wEdax-x86-64-v4.exe` with 32 threads): the same as v4.5.5-newedax.1, and as fast as the C version v4.5.5-eval2.6 or faster (0.959 to 1.001 times the time for 7 book commands). The peak memory is the same or less.

**Fixes** (the eval2 tools and `-eval-convert`)

- `bookdump` and `bookscan`: with a book that had a position of fewer than 4 discs, the move number was negative (the C tools have the same error). Such books are now refused.
- When the same scalar was written twice in the tool settings, it was counted twice. When a pattern had the same square twice, the symmetric mapping was broken. Both are now refused.
- In the 5-byte numbers of format 2, the upper 4 bits of the 5th byte were not checked (2^32−1 is read, 2^32 is refused).
- `build-evtools.ps1` returned the exit code 0 even when some tools failed to build. It now returns 1.
- `-eval-convert`: it converted format 1 files with fewer than 54 move numbers, and made files that neither format could read. In format 2, the number of move numbers × weights overflowed when it did not fit in 32 bits. Both are now refused.

**Compatibility**: the results of the search are those of v4.5.5-newedax.1 (and of v4.5.5-eval2.6).

**Notes**: with 1 thread, the 4 executables other than `wEdax-x86-64-v4.exe` are slower than the C version (a PGO build) (up to 11.6% in the midgame, `wEdax-x86-sse.exe`). This is fixed in v4.5.5-newedax.3.

**Not checked**: the strength of the default build (eval2), builds other than MSVC on Windows and the NDK on Android, real Android devices, AddressSanitizer and ThreadSanitizer, real books of billions of positions, a second reading by another person.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax2)

---

## v4.5.5-newedax.1

The first version of new edax. The C version v4.5.5-nikque.17 is ported to C++23, and the evaluation function of the eval2 line (v4.5.5-eval2.5) is the default. The sources are not yet checked by another person, and the speed of the default build is not yet measured, so this is a pre-release.

**Changes**

- In the default build, the build option `EVAL_MOBILITY` is 1 and `bin/data/eval.dat` is the eval2 one.
- The commands and options are those of the C version.
- Format 2 of the book and evaluation files (`NEDXBOOK`, `NEDXEVAL`) was added. Books are about 33% smaller and get a CRC-32C. Format 1 is read too.
- Options that change the format:

| Option | What it does |
|---|---|
| `-book-format` | Chooses the book file format |
| `-book-convert` | Converts the book format (both ways) |
| `-eval-convert` | Converts the evaluation file format (both ways) |

- A book position now takes 40 bytes in memory (48 bytes in the C version). Loading a book of 6.49 million positions takes 11 to 45% less memory.
- New build options: `EVAL_MOBILITY=0` (the mobility weights are not read), `EVAL_SLOTS=64` (64 slots; a base for a stronger evaluation function; the default is 48).
- `tools/eval`: the C++ version of the learning tools of the eval2 evaluation function (14 tools; the ZIP has only the x64 AVX-512 ones).
- Speed: compared with the C version v4.5.5-nikque.17 built the same way (without PGO), no case slower than the C version was found (`docs/port/MEASURE-1.md`).
- Release builds (MSVC 2022 `/O2 /GL`, **without PGO**; Android with NDK r27d):
  - Windows: `wEdax-x86-64-v4.exe`, `wEdax-x86-64-v3.exe`, `wEdax-x86-64.exe`, `wEdax-x86-sse.exe`, `wEdax-x86.exe`, `libedax-x64-v4.dll`, `libedax-x64-v3.dll`, `libedax-x64.dll`.
  - Android: `aEdax-arm64-v8a`, `aEdax-armeabi-v7a`, `libedax-arm64-v8a.so`, `libedax-armeabi-v7a.so`.
  - There are no executables for Linux, macOS or Windows ARM64 (to build from the sources, see "Build" in the README).
  - PGO is not used because the `book deviate` commands were about 1.5 to 2% faster without PGO (the same in the C version). In other cases, PGO and no PGO were not compared directly.

**Compatibility**

- The results of the search are those of the C version: those of v4.5.5-eval2.5 with the eval2 `eval.dat`, and those of v4.5.5-nikque.17 with the Edax 4.5 `eval.dat`.
- Format 1 books and `eval.dat` are read as they are.

**Not checked**: the speed and strength of the default build (eval2), builds other than MSVC on Windows and the NDK on Android, real Android devices, tests with builds that detect memory errors, real books of billions of positions, a check of the sources by another person.

[Tests and measurements](docs/validation/newedax.en.md#v455-newedax1)
