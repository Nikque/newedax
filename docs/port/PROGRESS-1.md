# new edax：C++ への移植の記録（段階0・1、2026-10-16）

設計は `DESIGN-cpp1.md`（第2版）。この資料は、その段階0・1で行ったことと結果の記録。

- 区別：**確かめた**（このセッションで実行した）／**記録にある**（以前の資料）／**見立て**（推測）。
- CPU の使い方の決まり：ビルドまでと、fforum-20-39 を1スレッドで解く程度の短い試験だけ。すべて1論理CPU（affinity 1）、優先度「通常」。

## 1. 段階0：準備

| 項目 | 内容 |
|---|---|
| フォルダ | 作業フォルダー（`newedax\` リポジトリ、`baseline\`、`runs\`、`third_party\`） |
| リポジトリ | `newedax`。v4.5.5-nikque.17 の作業コピーから履歴ごと複製し、`80e054e` から `main` を作った。リモートなし。作者 `Nikque <Nikque@users.noreply.github.com>`。`core.safecrlf false`（改行の警告を出さない） |
| Boost | 1.92.0（`archives.boost.io`、`boost_1_92_0.tar.gz`、235,384,471 バイト、SHA-256 `c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce` が配布元の値と一致）。`third_party\boost_1_92_0` にヘッダー（`boost\`）と `LICENSE_1_0.txt` だけを展開（184MB）。まだビルドには入れていない |
| 基準 | `baseline\`：`edax-repro.exe`（14 suite の C 版、SHA-256 `b2d85931…`、ソースは `80e054e` と同じ）、`release17-windows\`（配布版5種の fforum-20-39 の1スレッドの出力、Cassio、API 試験、`配布Windows検証.json`）、`verify_windows_release17.py`、`repro-suites\`（`run.py`・`compare*.py`・`suites.json`・`build.cmd`）、nikque.17 の検証報告と SHA256SUMS |

基準の値（記録にある）：配布版の5種（x64-v4・v3・x64・x86-sse・x86）とも、`-n 1 -solve fforum-20-39.obf` で 20 問、合計 **77,639,567 ノード**、問題ごとの深さ・評価値・ノード数・読み筋が同じ。DLL 3種の API 試験は 204 件・失敗 0。Cassio は各7条件が通る。

## 2. 段階1：commit

| commit | 内容 |
|---|---|
| `a465896` | C のまま、C++ で通らない所を直す（文字列の配列の大きさ、`void*` からの変換、`register`、`include\stdbool.h` を C だけに、`libedax_utf8.h` で `<io.h>` を先に読む、Android でアフィニティのコードを使わない）。`tools/port/compare_solve.py` |
| `f6e9200` | `src/*.c` → `*.cpp`、`tests/regression.c` → `.cpp`。MSVC は `/std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR-`、gcc・clang は `-std=c++23 -fno-exceptions -fno-rtti`（`Makefile` は `COMP=gcc`・`clang` で `g++`・`clang++` を使う）、Android は `LOCAL_CPPFLAGS`。`tools/port/build-msvc.cmd` |
| `8dbae28` | libedax：昔の libedax と同じ名前で出す5つの関数（`bit_count`・`first_bit`・`last_bit`・`get_moves`・`can_move`）を C の名前で出す。MSVC では `can_move` を int（0か1）で返す（C 版の MSVC の `bool` は 4 バイト） |
| `60beb99` | `tests/regression.cpp` を C++ でビルドできるように（型変換だけ）。`tools/port/compare_asm.py`・`run_regression.py` |
| `eaeee50` | `EVAL_FEATURE`・`EVAL_FEATURE_all_opponent` の宣言を `eval.h` に（Android の 32ビット ARM のリンク、3.9）。`tests/build-libedax-test.cmd` が前回の `libtest-*` を消す（3.4）。`build-msvc.cmd` の `=` の扱い。`tools/port/compare_disasm.py` |
| `e317c29` | 手順書に Boost のヘッダーの場所（`BOOST_DIR`、既定は `..\..\third_party\boost_1_92_0`）。`tests/boost_check.cpp`（3.11） |

`tests/libedax_test.c` と `tests/libedax_example.c` は C のまま（C の API を確かめるため）。

## 3. 確かめたこと

### 3.1 Android の分岐

NDK r27d のプリプロセッサーで、C 版（`-x c -std=gnu17`）の Android は `thread_set_cpu` で「何もしない」分岐に入ることを確かめた。C++ では `_GNU_SOURCE` が自動で定義され、`CPU_SET` の分岐に入って `pthread_setaffinity_np`（bionic にない）でエラーになっていた。`thread_set_cpu` と `thread_cpu_bound` の条件に `!defined(__ANDROID__)` を足した（C 版と同じ分岐）。

### 3.2 fforum-20-39、1スレッド（MSVC、PGO なし、`/O2 /GL`）

| ビルド | 実行ファイルの大きさ | 結果 |
|---|---|---|
| C（`a465896`、x64 AVX-512） | 792,064 | 20 問すべて記録と同じ（77,639,567） |
| C++（x64 AVX-512） | 789,504 | 同じ |
| C++（x64） | 813,056 | 同じ |
| C++（x64 AVX2） | 807,424 | 同じ |
| C++（x86 SSE、32ビット） | 681,984 | 同じ |
| C++（x86 IA32、32ビット） | 714,240 | 同じ |
| C++（x86 AVX2、32ビット。CRC のインライン化の誤りが出た構成） | 708,096 | 同じ |

比べ方：`tools/port/compare_solve.py`（問題ごとの深さ・評価値・ノード数・読み筋と合計）。道具が違いを見逃さないことは、記録の1か所を書き換えた写しで確かめた。基準の C 版（`edax-repro.exe`、同じフラグ）は 793,088 バイト。

### 3.3 Cassio の7条件

C++ の x64 AVX-512・x64 AVX2・x86 SSE・x86 IA32 と C 版で、すべて通った（実行ファイルの隣に `data\eval.dat` が要る。最初は置かずに流して全部失敗した。C 版でも同じだったので、置き方の問題）。

### 3.4 libedax（MSVC x64、`tests/build-libedax-test.cmd`）

- 最初のビルドは、5つの関数が C++ の名前になってリンクで失敗した（`8dbae28` で修正）。
- 修正後：C でコンパイルした `libedax_test.c` が C++ の DLL で **204 件・失敗 0**。
- エクスポート：`dumpbin /exports` で、配布版 `libedax-x64.dll` と同じ **110 個**、名前の差なし、装飾された名前なし。
- 2回目の実行で1件失敗した（`libedax_test.c` 730 行、`base unique` の出力の大きさ）。前回の実行の `libtest-unique.txt` が残っていて、`base_save` は既存のファイルの棋譜を残す仕様（`base.cpp` 939 行）なので、4局になっていた。出力を消すと 204 件・失敗 0。移植とは関係ない試験の手順の不具合で、`.cmd` が毎回 `libtest-*` を消すように直した（`eaeee50`）。直した `.cmd` で 204 件・失敗 0。

### 3.5 警告（C++ で新しく出たもの。動作は C と同じ）

- `game.cpp` 276・1776（C4805）：`int` の `player` に `bool` を `^=`。`bool` は 0 か 1 で、C と同じ計算。
- `flip_carry_sse_32.cpp` 1044 など（C4838、32ビットだけ）：`~0x08...` の符号つきの定数を `unsigned` の初期化に使う縮小変換。ビットの並びは同じ。
- `board_mmx.cpp` 160（C4799、EMMS）：関数の中で `_mm_empty()` を呼んでいる。C 版でも出るかは未確認。

### 3.6 機械語の比較（設計 5.4。MSVC x64 AVX-512、`/O2`、`/GL` なし、`/FAs`）

`tools/port/compare_asm.py` で、C 版（`a465896`）と C++ 版の関数ごとの命令を比べた（名前の装飾・ラベル番号・ソースの注釈は除く）。

- 関数 897 個のうち、同じ 379、違う 515。C++ だけにあるのは `strchr`・`strrchr`・`strstr`（C++ の標準ヘッダーの const つきの包み）。
- 違いの多くは、`bool` の幅（`test eax, eax` → `test al, al`、同じ速さの命令）と、使うレジスターの選び方。`NWS_endgame` の違いは `bool` の幅の1か所だけだった。
- **遅くなり得る所**：`search_eval_2`（評価の中心）で、C++ 版はループの中で `n_empties & 1` をメモリから読み直す命令が1つ増えた（304 → 307 命令）。配布用の PGO のビルドでも同じになるかは未確認。
- ほかの探索の関数の命令の数は ±5 以内（`NWS_midgame` 569 → 568、`PVS_midgame` 496 → 497 など）。
- 機械語が違う以上、速さが同じとは言い切れない。**実測（申請が必要）で確かめる。**

同じ比較を、リンク時最適化（`/GL`、`/LTCG`、PDB つき）の実行ファイルの逆アセンブル（`dumpbin /disasm`）でも行った（`tools/port/compare_disasm.py`）。関数 1,224 個のうち同じ 811、違う 413。`search_eval_2` の読み直しの1命令は、この版でも残っていた（293 → 296 命令）。原因は、ループをまたいで残す値（`eval0.n_empties & 1`）を置くレジスターの割り当ての違い（C++ 版は1つをスタックに置く）。1回の繰り返しで、約300命令の `search_eval_1` の呼び出しに対して読み込みが1つ増える。影響は小さい見込み（見立て）だが、遅くなり得る所として実測で確かめる。

### 3.7 `bool` の大きさ（設計 3.1 の再掲）

Windows（MSVC）では、C 版の `bool` が 4 バイト、C++ 版が 1 バイト。`Node` は 208 → 200 バイト（32ビットは 152 → 136）、`Task` と `Book` は 8 バイト小さい。公開の型の大きさと位置は同じ。

### 3.8 回帰試験（`tests/regression.cpp`、MSVC x64 AVX-512 `/O1`）

- C++ でのビルドには、試験のソースに型変換を足す必要があった（`malloc` などの戻り値、`options.book_file` に入れる文字列定数）。ケースの中身は変えていない（`60beb99`）。
- `reproducible`（記録では 97 秒）を除く 43 ケースを、C 版（`a465896` の `tests/regression.c`）と C++ 版で1論理CPU・60秒の上限で流した（`tools/port/run_regression.py`）：**両方とも 43 ケースすべて終了コード 0**、合わせて 20 秒。
- 出力の違い：エラー表示の中のソースのパス（`book.c` → `book.cpp`）、一時ファイルの名前のプロセス番号、`enhance` の局面数。`enhance` は C 版でも 6 回で 11〜14 と毎回変わる（level 0 の探索が時計で種を取る乱数で手を並べるため。記録にある既知の動作）。

### 3.9 Android（NDK r27d、`ndk-build`、armeabi-v7a と arm64-v8a）

- 1回目：32ビット ARM の本体のリンクで失敗した。`eval_sse.cpp`（NEON 用に別の翻訳単位でコンパイルされる）が `eval.cpp` の `const` の大域の表 `EVAL_FEATURE`・`EVAL_FEATURE_all_opponent` を `extern` で参照していたが、**C++ では、先に `extern` の宣言のない名前空間の `const` の変数は、ほかの翻訳単位から見えない**（C では見える）。宣言を `eval.h` に移して、C と同じ扱いにした。
- 警告 25 件：すべて「C++20 で非推奨の、`volatile` の変数への `++`・`--`」（`n_nodes` などの数え上げ。midgame・endgame・ybwc・move・search・root・event）。動作は C と同じ。段階3で書き方を直す候補。
- 2回目（`eaeee50`）：両方の ABI でビルドが通った。armeabi-v7a 384,748 バイト（C 版の配布物 384,788 より −40）、arm64-v8a 565,128 バイト（565,056 より +72）。警告は ABI ごとに同じ 25 件。端末がないので実行はしていない。
- 修正の後、Windows の x64 AVX-512・x86 SSE を作り直し、fforum-20-39 の20問が記録と同じ、API 試験 204 件・失敗 0 を確かめた（x64 AVX-512 の大きさは修正前と同じ 789,504）。

### 3.11 Boost 1.92（`tests/boost_check.cpp`）

- `unordered_flat_map`・`static_vector`・`endian` を、`BOOST_NO_EXCEPTIONS` と、異常終了する `boost::throw_exception` で使う小さな試験。Edax の本体には入れていない。
- MSVC 19.44 の x64・x86（`/std:c++23preview /EHs-c- /D _HAS_EXCEPTIONS=0 /GR-`）：ビルドして実行し、終了コード 0、値も正しい。
- NDK r27d clang 18 の arm64-v8a・armeabi-v7a（`-std=c++23 -fno-exceptions -fno-rtti`、libc++ を静的にリンク）：コンパイルとリンクが通った（実行はしていない）。大きさは 462,136・310,852 バイト。**本体で標準ライブラリを使い始めると、Android の実行ファイルが大きくなる**（報告の対象）。
- 手順書の `nmake /n`・`make -n` で、Boost の場所が命令に入ることを確かめた。

### 3.12 試験 A：14 suite の book（2026-10-16 16:26〜17:09、1論理CPU、優先度「通常」）

- 実行ファイル：C++ 版（`eaeee50` のソース。`tools/port/build-msvc.cmd x64`、基準と同じ `/O2 /GL /arch:AVX512 /vlen=256`、PGO なし。SHA-256 `91a2b4a9…`）。
- 道具：記録の `run.py` の写し（入力は `inputs`、出力は `runs\suiteA\runs`）。比べ方：`tools/port/compare_suites_port.py`（記録の `compare.py` を使う。book は保存日時以外のバイト・保存順・生の記録・論理データ、ほかはバイト）。
- 比べた相手：記録の `S-<suite>-R1`（`-n 32 -book-seed 1 -book-reproducible on`）と `S-<suite>-Dn1`（`-n 1`、既定の設定）。どちらも C 版 `edax-repro.exe`（`b2d85931…`）の出力。
- 結果：**28 回すべて終了コード 0。264 ファイルのうち 261 が保存日時以外のバイトまで一致**（book 236、テキスト 20、そのほか 8）。`R1` は 14 suite すべて一致。
- 一致しなかった3つ：S3 の `Dn1` の学習の後の book（`book.dat.store`・`u01_learn.dat`・`u02_fix.dat`、各 147 局面、局面数 32,312 と 32,293）。既定の設定では book の乱数の種が時計なので、記録でも C 版どうしで一致しない（nikque.17 の検証報告 5b、`Dn1/Bn1` が 2/5）。確かめ（17:10〜17:12、許可の範囲の時間内）：同じ条件に `-book-seed 1` だけを足して、C 版と C++ 版を1回ずつ流すと、**5ファイルすべて保存日時以外のバイトまで一致**（32,313 局面。記録にある種1の book と同じ局面数）。
- 時間（混んだ PC の1論理CPU。速さの比較には使えない）：`R1` 合計 1,494 秒（記録 1,647 秒）、`Dn1` 1,062 秒（記録 1,074 秒）。最大メモリ：`R1` 685〜2,044 MiB、`Dn1` 99〜120 MiB（記録の側には値がない）。

### 3.13 試験 B：`tests/regression.cpp` の `reproducible`（2026-10-16 17:24、許可を得て実行、1論理CPU）

C++ 版（MSVC x64 AVX-512 `/O1`、`tests/build-regression.cmd`）で **PASS**、7.2 秒。`book-seed` の確認、Link の確かめ直し、4スレッド2回と1スレッドで 225 局面の book が保存日時以外一致（記録の C 版と同じ局面数）。探索し直しは 0 回（記録でも、この条件では 0 回）。

### 3.14 試験 C：Windows の PGO のビルド 5 種（2026-10-16 17:24〜17:38、許可を得て実行、1論理CPU）

- `src/NMakefile` の `vc-pgo-x64-v4`・`v3`・`x64`・`x86-sse`・`x86` を、`runs\wt-pgo`（`ef894a0` の worktree。`bin` の配布物を上書きしないため）で順に実行。x86 は x86 の環境で。5 種とも、学習の情報を使えた命令は 100%。
- 実行ファイルは `runs\pgo-exe`（ログも）。
- 確かめたこと（17:38、短い試験）：5 種とも fforum-20-39 の1スレッドが配布版の記録と同じ（20 問の深さ・評価値・ノード数・読み筋、77,639,567）。Cassio の7条件も 5 種とも通った。

| 版 | C++ の PGO 版 | 配布版 nikque.17（C） | 差 |
|---|---|---|---|
| x64-v4 | 660,992 | 660,480 | +512 |
| x64-v3 | 667,136 | 667,648 | −512 |
| x64 | 681,984 | 684,544 | −2,560 |
| x86-sse | 620,032 | 620,032 | 0 |
| x86 | 664,064 | 665,088 | −1,024 |

（大きさはバイト。**x64-v4 だけ 512 バイト大きい**。）

- 運転に使った `.cmd` は、進み具合の行の時刻と終了コードの表示が誤っていた（ブロックの中の `%time%`・`%errorlevel%`）。ビルドには影響しない（各ログと実行ファイルで確かめた）。直したものを `tools/port/build-pgo.cmd` として残したが、**直した版はまだ実行していない**。

## 3.15 Boost と C++ による高速化・省メモリ（2026-10-16 夜。Boost を積極的に使う方針）

方針：動作（探索のノード数、book のバイト、出力）を変えず、速さとメモリは C 版より悪くならないこと。速さの実測はできない（申請しない指示）ので、理屈と機械語、短い試験の CPU 時間で判断する。

### 入れたもの

| commit | 内容 | 動作 | 速さ・メモリ（見立て・確かめたこと） |
|---|---|---|---|
| `1861c54` | `volatile` の `++`・`--`・連続代入をやめた（C++20 の非推奨） | 同じ | 機械語は 897 関数中 893 が同じ。並列探索の数え上げ3関数は、`volatile` を2回読んでいたのが1回になった（命令 −1） |
| `3db467b` | todo の一覧（`book deviate`・`enhance`・`fill`・展開）の並べ替えを `qsort` → `boost::sort::pdqsort_branchless`、探しを `bsearch` → `std::binary_search`。`base unique` の並べ替えを `pdqsort` に | 同じ（要素がすべて違う、または比較が全順序なので、どの並べ替えでも同じ順） | `qsort` は比較のたびに関数ポインタを呼ぶ。`pdqsort_branchless` は 64 ビットの比較をその場で分岐なしに行う（速くなる見立て、測っていない）。どちらもメモリを確保しないので、メモリ不足のときの動作も同じ。suite A・F が記録と一致 |

### 試して戻したもの

- `book check`・`count positions`・`count shapes` の「すでに出たか」の表を `boost::unordered_flat_set` に替えた（要素を 17・13・8 バイトに詰め、最初に C 版の表と同じ量を予約）。数はすべて C 版と一致し、`count positions 11`・`count shapes 11` の CPU 時間は 35.7・34.8 秒 → 24.8・23.9 秒（2回ずつ）に減ったが、**最大メモリが 560 → 748 MiB に増えた**。
  - 理由：C 版は、2^b 個のバケットのそれぞれが「ちょうどの大きさ」の配列を持つ（1つ足すたびに1つ広げる）。要素が多いと、1要素あたりのメモリはほぼ要素の大きさだけになる。Boost の集合は空きを残し（最大 7/8 まで詰める）、広げるときに前と後の表が同時にある。
  - C 版よりメモリが悪くなる条件を作らないため、3つとも元に戻した（履歴からも外した）。
  - 数での裏づけ（`count positions 11`：11 手先（石 15 個）までの 23,259,310 局面を覚える。`-n 1` の自動の大きさで 2^21 バケット、1バケットあたり約 11 局面）：C 版は局面 13 バイト × 2,326 万 ＝ 約 302MB に、バケット（16 バイト × 2^21 ＝ 34MB）と確保ごとの付帯（バケットごとに 16〜32 バイト、約 50MB）。Boost の集合は、2,326 万を 7/8 以下に収める 2^25 枠 × 14 バイト ＝ 約 470MB で、広げる瞬間は前の 2^24 枠（235MB）も同時にある。差（最大で約 +300MB、測った最大メモリの差は +188 MiB）は、この見積もりとおおむね合う（見立て。ほかの確保もあるので一致はしない）。
  - 取り返す道（どれも未実装）：
    1. **C 版の形のまま速くする**：2^b 個のバケットにちょうどの大きさの配列を持つ形を保ち、配列を「大きさごとの空き一覧を持つ大きな領域」から切り出す（`malloc` の付帯 16〜32 バイトがなくなる）。比較は 13 バイトをまとめて読む。メモリは C 版より必ず少なく、確保の呼び出しが減る分速い。
    2. 区画に分けた集合（ハッシュの上位ビットで 256 個などに分け、広げるのは1区画ずつ）：広げる瞬間の重なりが 1/256 になる。ただし空きを残す分（平均で約 1/0.66 倍）は残るので、要素が多いと 1 だけでは C 版に勝てない。1 と組み合わせる。
    3. 鍵を詰める：バケットの番号がハッシュの一部を表すので、その分を要素から省ける（13 バイトをさらに減らせる見込み）。
    4. `count positions` は深さごとに数えるので、深さが変わる所で集合を作り直す・並べて重なりを除く、などの使い方の見直し。

### これからの候補（見立て。効果の大きい順）

1. **book の局面（`Position`、48 バイト）を詰める**：主な使い方（`book deviate`、6.6億局面の book）でメモリに直接効く。4つの数 `n_wins`・`n_draws`・`n_losses`・`n_lines`（各 32 ビット、計 16 バイト）は多くの局面で小さい値。メモリ上だけ詰めて、ファイルの形は変えない形が考えられる（例：小さい値は 1 バイトずつ、大きい値は別の表）。48 → 40 バイトなら book のメモリは約 17% 減（見積もり）。保存のバイトは同じにできる。設計案は `docs/port/DESIGN-position40.md`（この時点では未実装。3.16 で実装）。
2. `search_eval_2` の読み直し（3.6）：評価の更新の5通りの実装をそろえて、手番の偶奇を引数で渡す形にする。
3. book の表（バケットの配列）の作り直し：段階4（ファイル形式）と一緒に。

## 3.16 book の局面を 40 バイトに（2026-10-16 夜、設計 `DESIGN-position40.md`）

- 4つの数を各 16 ビットにし、65,534 を超える局面だけ book ごとの表（`boost::unordered_flat_map`）に置く。保存するバイトは同じ。
- 確かめたこと（MSVC x64 AVX-512、PGO なし）
  - fforum-20-39 の1スレッドが記録どおり（x64 AVX-512、x86 SSE の 32 ビット版）。API 試験 204 件、回帰 43 ケース。NDK clang 18（armv7a・aarch64）で警告・エラーなし。
  - `base12.dat`（数が 65,534 を超える局面が 4 つ）：読み込み → 保存、`book negamax`、`book fix`、`book merge` の保存が、C 版と保存日時以外まで一致。読み込み → 保存は元のファイルとも一致。
  - `book fix` で盤面を直す経路：`base12.dat` の 837 局面（数の大きい 4 局面を含む）を左右に反転させた book（`tools/port/make_mirrored_book.py`）で、`book fix` と `book negamax` の保存が C 版と一致。
  - 14 suite（R1・Dn1 の 28 回、20:37〜21:21、1論理CPU）：264 ファイルのうち 261 が記録と一致。残り3つは S3 の `Dn1`（時計の種。試験 A と同じ）で、種を固定すると C 版と3ファイルとも一致。
  - 649 万局面の book（`data\A_cut40.dat`、286MB。29GB の実 book とは別）：読み込み → 保存が C 版と一致。**最大メモリ 665 → 563 MiB（−102 MiB、−15%）**、`book info` の見積もり 333.8MB → 285.9MB（−14.3%）、CPU 時間 3.7 → 2.7 秒（1回。混んだ PC）。
- 悪くなり得る条件（見立て）：数の大きい局面が非常に多い book では、表の分（その局面だけ、1 つあたり項目 33 バイトを空きを含めて約 40〜75 バイト）が増える。数を読むたびに「印か」の判定が1つ入る（集計と保存だけで、探索の中ではない）。

## 3.17 段階2：`src` をモジュールごとのフォルダに（2026-10-16 夜）

- `tools/port/restructure_src.py` で 108 ファイルを `git mv`：`core/`（`bit`・`board`・`move`・`hash` など）、`core/flip/`（合法手の計算の各版 40 ファイル）、`eval/`、`search/`、`book/`、`game/`、`protocol/`、`util/`、`app/`、`api/`。`all.cpp` と手順書は `src/` に残す。
- `#include` は `src/` からのパス（`"core/board.h"`）。手順書に `src/` を探す場所として足した（`NMakefile`・`build-msvc.cmd`：`/I.`、`Makefile`：`-I.`、Android：`LOCAL_C_INCLUDES += $(LOCAL_PATH)`、試験の `.cmd`：`/Isrc`）。`Doxyfile` は下のフォルダも読む。
- 機械語：移動の前と後で、1,223 関数すべて同じ（MSVC x64 AVX-512 `/O2`、`/FAs`）。
- 確かめたこと：fforum-20-39 の1スレッド（x64 AVX-512、x86 SSE）が記録どおり、API 試験 204 件、回帰 43 ケース。

### Android のリンクの失敗と、実行ファイルが大きくなった件（原因と対処）

- 移動の後、Android の `ndk-build` が実際のリンクで失敗した（`std::mutex`・`operator new` などが見つからない）。**原因は移動ではなく、3.16（局面を 40 バイト）で `std::mutex` と Boost の表を使い始めたこと**。そのときは Android を文法の確認しかしておらず、リンクを確かめていなかった（見落とし）。
- `APP_STL := c++_static`（C++ の標準ライブラリを実行ファイルに入れる）でリンクは通ったが、**実行ファイルが arm64 で 565,056 → 794,976 バイト（+41%）、armeabi-v7a で 384,788 → 519,300 バイト（+35%）に増えた**。
- 原因（`llvm-nm -u` で確かめた）：`all.o` が使う C++ の実行時の部品は `operator new`・`operator delete`・`std::mutex`・`__cxa_atexit` だけ。`operator new` は失敗すると例外を投げる実装なので、例外の仕組み・型情報・スタックの巻き戻しなどが一緒に入る。
- 対処：表のロックを `std::atomic_flag` のスピンロックに（ヘッダーだけ、後始末なし）、表と入れ物の確保を `malloc` の確保器 `util/allocator.h`（`HeapAllocator`。失敗したら `fatal_error`、C の部分と同じ）に。これで `all.o` が使う C++ の実行時の部品は 0 になった（`llvm-nm -u` で確かめた）。
- 対処の後の大きさ：**Android arm64 576,752 バイト（C 版 565,056 より +11,696、+2.1%）、armeabi-v7a 395,884 バイト（384,788 より +11,096、+2.9%）、Windows x64 AVX-512 802,816 バイト（C 版の同じ設定 792,064 より +10,752、+1.4%）、x86 SSE 694,784 バイト（C++ への移行直後 681,984 より +12,800）**。残る増えは、3.16 の表（`boost::unordered_flat_map` の実体）と並べ替えのコード。局面のメモリの減り（649 万局面で −102 MiB）の代わりとして残す。小さくする道：表の操作を1か所の関数にまとめ、実体を1つにする（未実施）。
- 対処の後も、`base12.dat` の読み込み・negamax・fix・merge と、左右反転の book の fix・negamax の保存が C 版と一致（`P40-S2B`・`M40-S2B`）。fforum（x64・x86）、API 204 件、回帰 43 ケースも通った。

## 3.18 段階3（2026-10-16 夜）

### ファイル操作の関数（`ea2a80e`）

- DLL の Windows 版で `fopen`・`remove`・`MoveFileExA` をマクロで差し替えていた `api/libedax_utf8.h` をやめ、`util.cpp` の `file_open`・`file_remove`・`file_rename`・`file_replace` にした（DLL の Windows 版は UTF-8 → ANSI の順で名前を読む、ほかは C の関数をそのまま呼ぶ）。`all.cpp` から読まれるソースの `fopen` 38 か所・`remove` 8 か所を置き換えた。`book_set_aside` の `#ifdef` は `file_rename` に。
- マクロが C++ の標準ヘッダーの名前（`std::remove`）まで書き換える危険がなくなった。
- `tests/libedax_test.c` に、Windows で日本語（片仮名3文字）を含む UTF-8 の名前で book を保存し、その名前で見つかり読み戻せる試験を足した（206 件）。この試験が UTF-8 の扱いの誤りで失敗することは、理屈の上では確かだが、壊した版で実際に失敗させる確認はしていない。

### `search_eval_2` の読み直しをなくした（`610e304`・`8b9d1b4`、3.6 の件）

- 原因：C++ 版は `r15` を保存・復元しているのに使わず、偶奇（`n_empties & 1`）をスタックに置いていた。レジスターが足りないのではなく、MSVC の割り当ての判断の違い。
- 試したこと：(1) 偶奇を引数で渡す → 変わらず。(2) ラムダで偶奇ごとのループ → ラムダが関数として分かれ、変数をメモリ越しに読む形になり悪化（戻した）。(3) 偶奇をテンプレートの引数にした関数 `search_eval_2_moves<PARITY>` を強制インライン化（`EDAX_FORCE_INLINE`）→ スタックの読み書きがなくなり、ループの中の偶奇の判定もなくなった（`/O2` と `/GL /LTCG` の両方で確かめた）。関数は大きくなる（LTCG で 293 → 389 命令）。
- 直したあとで見つけた誤り：`/GL` なしの版で、内側の `eval_update_sse_parity` がインライン化されず、`eval_update_sse` から呼ぶ形（呼び出しが1段増える）になっていた。強制インライン化して、`eval_update_sse` の機械語は C 版と同じに戻った（`/GL` の版はもともとインライン化されていて、実行ファイルの大きさは同じ）。
- 今の探索の中心の関数の比較（C 版は `bool` を1バイトにした比較用、`/O2`）：命令の並びまで違うのは `NWS_midgame`（569 → 568）・`PVS_midgame`（497 → 497）・`PVS_root`（881 → 880）・`movelist_get_moves`（92 → 92）・`search_eval_1`（317 → 317）・`search_eval_2`（上記）。どれも命令の数は同じか1少なく、違いはレジスターの選び方と命令の順番（MSVC の C と C++ の入口が違う中間コードを作るため、と見ている）。速さが同じかは実測でしか言えない。

## 4. まだのこと

- Linux（WSL に gcc 13 が入ってから）、32ビット Linux。macOS は最後。
- Android の実行（端末がない）。
- PGO のビルド：済み（3.14）。PGO 版どうしの機械語の比較はまだ。
- 14 suite の book のバイト比較：済み（3.12）。
- 速さとメモリの実測（申請が要る）。`search_eval_2`（3.6）を含む。
- `tests/regression.cpp` の `reproducible`：済み（3.13）。
- README・RELEASE-NOTES の `.c` の記述（段階1では変えていない）。

### 4.1 申請したい試験（CPU を使う。時間は見込み）

| # | 試験 | CPU | 時間の見込み |
|---|---|---|---|
| A（済み、3.12） | 14 suite の book（C++ 版を基準と同じ `/O2 /GL /arch:AVX512` でビルド。`R1`＝`-n 32 -book-seed 1 -book-reproducible on` と `Dn1`＝`-n 1` 既定の 28 回）を記録（`repro-runs\S-*-R1`・`S-*-Dn1`）と `compare_suites.py` で比べる | 1論理CPU | 約 45 分（記録：同じ条件の 70 回が約 1 時間 50 分） |
| B（済み、3.13） | `tests/regression.cpp` の `reproducible` | 1論理CPU | 約 2 分（記録 97 秒） |
| C（済み、3.14） | Windows の PGO のビルド 5 種（学習の実行：fforum-20-39 を level 60 で1スレッド、level 18 の自己対局 4 局） | 1論理CPU | 約 15〜30 分 |
| D | 速さとメモリ（C 版と交互に、静かな時間に）：fforum-20-39 1スレッド・32スレッド、fforum-40-59 32スレッド、中盤 30 局面 level 18・21、suite F（deviate 系）、最大メモリ | 全CPU（静かなとき） | 約 1 時間 |
