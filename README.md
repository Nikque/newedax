# newedax

**new edax** is [Edax 4.5.5](https://github.com/Nikque/edax-reversi-AVX) (the corrected fork, its `eval2` line: v4.5.5-nikque.17 and v4.5.5-eval2.5) ported to C++23. [日本語](#日本語)

- **Status: in development; the first pre-release is [v4.5.5-newedax.1](https://github.com/Nikque/newedax/releases/tag/v4.5.5-newedax.1)** (Windows x64/x86 and Android executables, libedax DLLs, the eval2 tools; [release notes](RELEASE-NOTES.md)). `bin/` of the repository holds the data only (`bin/data/eval.dat` of eval2, `bin/data/book.dat`, `bin/config.ini`): download the ZIP of the release, or build the executable yourself (below). The executables of the C version are in the [releases of edax-reversi-AVX](https://github.com/Nikque/edax-reversi-AVX/releases) (v4.5.5-nikque.17, v4.5.5-eval2.5).
- **Same results as the C version**, checked on Windows (MSVC 2022, x64 and x86): with `bin/data/eval.dat` (eval2), fforum-20-39 is solved with the same node counts, scores and lines as v4.5.5-eval2.5; with the eval.dat of Edax 4.5, as v4.5.5-nikque.17. Also the same: the books of 14 test suites (with the same random seed), the C API of libedax, the Cassio protocol.
- **Speed and memory** (Windows, the same compiler options, no PGO, the eval.dat of Edax 4.5): no case slower than the C version was found; loading a book of 6.5 million positions takes 11 to 45 % less memory. The default build with the eval.dat of eval2 is not measured yet. Records (in Japanese): [docs/port](docs/port) (`PROGRESS-1.md`, `MEASURE-1.md`).

### What is new
- Files of form 2: books (`NEDXBOOK`, about 33 % smaller) and evaluation files (`NEDXEVAL`), with a CRC-32C. Options `-book-format`, `-book-convert`, `-eval-convert` (both ways). Files of form 1 are still read.
- Book positions take 40 bytes in memory (48 in the C version).
- Build options: `EVAL_MOBILITY` (1, the default: the mobility weights of eval2 are read and used; an eval.dat without them gives the evaluation of Edax 4.5; 0: they are not read), `EVAL_SLOTS=64` (64 feature slots, a base for a stronger evaluation; the default is 48).
- [tools/eval](tools/eval): the training tools of the eval2 evaluation in C++ (`evsearch`, `evtrain`, `w2dat2`, `selfplay3`, ...; records `docs/port/EVTOOLS-1.md` to `EVTOOLS-3.md`).

### Build
- Boost 1.92 (headers only) in `third_party/boost_1_92_0` next to the folder of the repository (`../third_party/boost_1_92_0`), or `BOOST_DIR=<directory>`.
- Windows, Visual Studio 2022 (x64 Developer Command Prompt): `build-win-v4.cmd` (x64 AVX-512), or in `src`: `nmake -f NMakefile vc-x64-v3` (other targets, PGO builds `vc-pgo-...`: `src/NMakefile`).
- Linux and macOS (`src/makefile`, gcc 13 or later, or clang): not built yet. Android (NDK r27d, `ndk-build`): compiled and linked (before the default `EVAL_MOBILITY=1`), not run yet.

### Not checked yet
Builds other than MSVC on Windows (and the Android link); AddressSanitizer and ThreadSanitizer on the C++ version; real books of billions of positions; a review by a second reader.

License: GPL-3.0 ([LICENSE](LICENSE)). Boost is under the Boost Software License and is not included.

## 日本語

**new edax** は、[Edax 4.5.5 の修正版](https://github.com/Nikque/edax-reversi-AVX)（`eval2` の系列：v4.5.5-nikque.17 と v4.5.5-eval2.5）を C++23 に移したものです。

- **開発中です。最初のプレリリースは [v4.5.5-newedax.1](https://github.com/Nikque/newedax/releases/tag/v4.5.5-newedax.1)**（Windows の x64・x86 と Android の実行ファイル、libedax の DLL、eval2 の道具。[修正一覧](RELEASE-NOTES.ja.md)）。リポジトリの `bin/` にはデータ（eval2 の `bin/data/eval.dat`、`bin/data/book.dat`、`bin/config.ini`）だけがあります。Release の ZIP を使うか、自分でビルドしてください（上の「Build」）。C 版の実行ファイルは [edax-reversi-AVX の Releases](https://github.com/Nikque/edax-reversi-AVX/releases)（v4.5.5-nikque.17、v4.5.5-eval2.5）にあります。
- **C 版と同じ結果**（Windows、MSVC 2022、x64 と x86 で確かめた）：`bin/data/eval.dat`（eval2）では fforum-20-39 の節点数・評価値・読み筋が v4.5.5-eval2.5 と同じ、Edax 4.5 の eval.dat では v4.5.5-nikque.17 と同じ。14 の試験の book（乱数の種をそろえたとき）、libedax の C の API、Cassio の通信も同じ。
- **速さとメモリ**（Windows、同じコンパイルの選択肢、PGO なし、Edax 4.5 の eval.dat）：C 版より遅い条件は見つかっていません。649 万局面の book の読み込みはメモリが 11〜45% 少ない。eval2 の eval.dat を使う既定のビルドの速さは、まだ測っていません。記録は [docs/port](docs/port)（`PROGRESS-1.md`、`MEASURE-1.md`）。
- 新しいこと：book と評価ファイルの形2（`-book-format`・`-book-convert`・`-eval-convert`。形1も読む）、book の局面が 40 バイト、`EVAL_MOBILITY`（既定 1）・`EVAL_SLOTS=64` の選択肢、[tools/eval](tools/eval)（eval2 の学習の道具の C++ 版）。
- まだ確かめていないこと：Windows の MSVC 以外のビルド（Android はリンクまで。`EVAL_MOBILITY` の既定を 1 にする前）、メモリの誤りを検出するビルドでの試験、数十億局面の実 book、別の人による確認。

---

# edax-reversi-AVX (the README of the fork and of the upstream project)

**Edax 4.5.5 corrected build (C version):** [English release README](README-NIKQUE.en.md) · [日本語の修正版README](README-NIKQUE.ja.md) · [Windows executable and release downloads](https://github.com/Nikque/edax-reversi-AVX/releases)

The historical upstream README follows below.

Automatically exported from code.google.com/p/okuharaandroid-edax-reversi

Edax is a strong othello program. Its main features are:

    fast bitboard based & multithreaded engine.
    accurate midgame-evaluation function.
    opening book learning capability.
    text based rich interface.
    multi-protocol support to connect to graphical interfaces or play on Internet (GGS).
    multi-OS support to run under MS-Windows, Linux and Mac OS X.

This is SSE/AVX optimized version of Edax 4.4.0. Functionally equivalent to the parent project, provided no bugs are introduced.

Thanks to AVX2, x64-modern build solves fforum-40-59.obf 60% faster than official edax-4.4 on Haswell, and runs level 30 autoplay 80% faster.

See http://www.amy.hi-ho.ne.jp/okuhara/bitboard.htm and http://www.amy.hi-ho.ne.jp/okuhara/edaxopt.htm for optimization details in Japanese.

## 1. Mobility (board_sse.c, board_mmx.c)

### 1.1 new SSE2 version of get_moves
Diagonals are SIMD'd using vertical mirroring by bswap.

    Athlon -get_moves_sse
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.998 (13922185 nodes/s).
    mobility: 81.10 < 81.28 +/- 0.17 < 82.03
    Athlon +get_moves_sse
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.889 (14114544 nodes/s).
    mobility: 71.08 < 71.72 +/- 0.34 < 73.53
    Core2 -get_moves_sse
    problem/fforum-20-39.obf: 111349635 nodes in 0:10.180 (10938078 nodes/s).
    mobility: 78.06 < 78.18 +/- 0.08 < 78.41
    Core2 +get_moves_sse
    problem/fforum-20-39.obf: 111349635 nodes in 0:09.978 (11159514 nodes/s).
    mobility: 60.84 < 61.19 +/- 0.13 < 61.47

### 1.2 can_move
Now calls SIMD'd get_moves for x86/x64 build.

## 2. Stability (board.c, board_sse.c, board_mmx.c)

### 2.1 get_full_lines_h, get_full_lines_v
get_full_lines for horizontal and vertical are simplified. The latter is compiled into rotation instrunction.

### 2.2 rearranged loop
The last while loop is rearranged not to call bit_count in case stable == 0.

### 2.3 new SSE2 version with bswap and pcmpeqb
    Athlon -get_stability_sse
    stability: 90.10 < 90.28 +/- 0.24 < 91.20
    Athlon +get_stability_sse
    stability: 81.59 < 81.93 +/- 0.73 < 86.25
    Core2 -get_stability_sse
    stability: 79.24 < 79.39 +/- 0.15 < 79.93
    Core2 +get_stability_sse
    stability: 71.80 < 71.85 +/- 0.06 < 72.07

### 2.4 get_corner_stability
Kindergarten version eliminates bit_count call.

### 2.5 find_edge_stable
Loop optimization and flip using carry propagation. One time execution but affect total solving time.

## 3. eval.c (4.4.5)
Eval feature calculation using SSE2 / AVX2 (now in eval_sse.c) improves midgame by 15-30% and endgame by 8-12%.
Restoring eval from backup instead of rewinding.
eval_open (one time execution) is also optimized.

## 4. hash.c
I think hash->data.move[0] on line 677 should be hash->data.move[1].

## 5. board_symetry, board_unique (board.c, board_sse.c)
SSE optimization and mirroring reduction. (Not used in solving game)

## 6. endgame_sse.c (4.4.7)
Keep more variables in SSE registers.  SSE optimized count_last_flip.  Parity sort by shuffle.

## 7. board_get_hash_code (4.5.0)
Changed to use CRC32c.  This enables hardware acceleration on modern build.

## 8. AVX2 versions (x64-modern build only)
In many cases AVX2 version is simplest, thanks to variable shift instructions (although they are 3 micro-op instructions).

Benchmarks are on Core i5-4260U (Haswell) 1.4GHz (TB 2.7GHz) single thread.

    4.4.0 original x64-modern clang
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.726 (19446321 nodes/s).
    +optimizations 1-5 above, no-avx2
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.342 (20844185 nodes/s).
    +get_moves (board_sse.c)
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.142 (21654927 nodes/s).
    +flip_avx.c
    problem/fforum-20-39.obf: 111349635 nodes in 0:04.946 (22513068 nodes/s).
    +count_last_flip_sse.c
    problem/fforum-20-39.obf: 111349635 nodes in 0:04.906 (22696624 nodes/s).

## 9. makefile
gcc-old, x86 build should be -m32, not -m64. Some flags and defines added for optimization.
