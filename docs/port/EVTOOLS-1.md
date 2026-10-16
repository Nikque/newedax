# new edax：eval2 の道具の C++ 化の記録（2026-10-17）

書いたのは Claude Opus 5.5（`HANDOFF-cpp5.md` 6章の候補 B を受けた窓）。案は `PLAN-evtools-cpp1.md`。

- 区別：**確かめた**（この窓で実行・閲覧した）／**記録にある**（以前の資料）／**見立て**（推測）。
- CPU の使い方：ビルドと 1 論理CPU の短い試験だけ。学習・生成・対局の本番は流していない。
- 元の道具 `ev` は読むだけ（ソース・データとも書き換えていない。確かめた：今日更新されたファイルはない）。

## 1. 方針（2026-10-17）

| 項目 | 決定 |
|---|---|
| 進め方 | 道具ごとの細かい選択は聞かずに決めて、理由と一緒に報告する |
| 置き場所 | new edax のリポジトリの `tools/eval/`（案のとおり） |
| 出発点 | eval2.dat から始める（new edax に着手可能数の重みが要る） |
| C の道具の不具合 | 最新の nikque でも直っていないものは直す（`ev` の道具は nikque の版に入っていないので、全部が対象） |

## 2. 移した道具

`tools/eval/`。ソースの頭のコメントに使い方がある。C の道具の名前・選択肢・出力をそのまま保つ。

| 道具 | 元 | 役目 | 探索 |
|---|---|---|---|
| `evtool.hpp` | `evtool.h` | 記録（17 バイト）、データの場所、eval.dat 形1、46 特徴の添字。名前空間 `evtool` | — |
| `evsearch` | `evsearch.c` | 学習の中心（構成の一覧、L2 つき最小二乗、CG、`-prior`・`-teacher`・`-base`・`-dump`） | なし |
| `evtrain` | `evtrain.c` | 46 特徴だけの学習（`-window` で隣の手数を混ぜる、`-calib`） | なし |
| `w2dat2` | `w2dat2.c` | `-dump` の重み → eval.dat 形1＋`MOBW` | なし |
| `sprepair`・`sample` | 同名 | 記録の境目の修復、試験用の局面の取り出し | なし |
| `bookdump`・`bookscan` | 同名 | book 形1 → 手数ごとの記録、統計 | なし |
| `selfplay3`・`label`・`playout`・`gen` | 同名 | 自己対局、局面の値付け、最後まで打つ、完全読みの局面 | あり |
| `leafgen` | `leafgen.c`＋`srchook` | 探索で評価した局面を集める | あり（差し込み口） |
| `evcheck` | `evcheck.c` | `evtool` の特徴と Edax の評価の一致 | あり |

移さなかったもの：同じ道具の古い版（`evsearch_*.c`、`selfplay.c`・`selfplay2.c`、`w2dat.c`）、試作（`evproto.c`。機能は `evsearch` にある）、一度だけの確認（`mobcheck.c`・`mob2check.c`・`leveltest.c`）、Python の道具（exe を呼ぶだけで、C++ にしても速くならない。そのまま使える）。

### 2.1 ビルドと試験の道具

| ファイル | 使い方 |
|---|---|
| `build-evtools.ps1` | `pwsh -File build-evtools.ps1 -Out <dir> [-Tools a,b] [-Cpu n] [-Extra '<cl>']`。MSVC x64 AVX-512、`/O2 /MT`（C の道具と同じ、`/fp:fast` なし）、例外なし。探索を使う道具は `EVAL_MOBILITY=1`、`leafgen` はさらに `EVAL_LEAF_HOOK` |
| `compare_c.py` | C の道具と C++ の道具を同じ入力・1 スレッド・1 論理CPU で流し、出力（ファイル・標準出力・エラー）をバイトで比べる。`--repeat n` で交互に n 回流してサイクルと最大メモリ |
| `compare_asm_c.py` | C と C++ の機械語を関数ごとに比べる（`/FA` の一覧） |
| `test_evtools.py` | 不具合の試験 16 件（4章） |

C の道具の比べる相手は `runs\evtools-c`（`ev` のソースを `cc.cmd`・`cc4.cmd` と同じ旗でビルド。探索は `runs\wt-c`＝nikque.17 の C、または eval2.5 の C。`leafgen` は差し込み口を足した写し。手順は `runs\evtools-c\build-c.ps1`・`make-hook-src.py`）。

## 3. new edax 本体の変更（`5c54a96`）

| 選択 | 中身 |
|---|---|
| `EVAL_MOBILITY=1` | 形1の eval.dat の 61 手数の後ろの `MOBW`（または `WBOM`）と 61×64 の重みを読み、`search_eval_0`・`search_eval_1` で両者の着手可能数（31 で頭打ち）の重みを足す。v4.5.5-eval2 と同じ。表のないファイルは今までどおり |
| `EVAL_LEAF_HOOK` | `search_eval_0`・`search_eval_1` で評価する局面を `eval_leaf_hook` に渡す（`leafgen` 用） |

どちらも既定は無し。確かめたこと：

- 既定のビルド：quick_check（x64 AVX-512・x86 SSE の fforum-20-39 が記録どおり 77,639,567 節点、API 206 件、回帰 51 件）。x64 AVX-512 のアセンブリの一覧が変更前（`f8b4d1f`）と同じ（違うのは eval.cpp の 4 関数で `fatal_error` に渡す行番号だけ）。
- `EVAL_MOBILITY=1`：eval2.5 の eval.dat で fforum-20-39 を 1 スレッドで解くと、eval2.5 の Windows 版と 20 問すべて同じ（合計 78,733,223 節点）。既定のビルドに同じ eval.dat を渡すと、表を読み飛ばすので 96,748,964 節点（20 問すべて違う）。**eval2.dat を使うときは `EVAL_MOBILITY=1` のビルドが要る。**
- 回帰の試験 `evalmobility`（`tests/regression.cpp`）：乱数の表を付けた eval.dat（`MOBW`・`WBOM`）と表のないファイルで、`search_eval_0`・`search_eval_1` がパターンの和＋表の値（着手可能数は盤面から数え直す）と同じ。既定のビルドでは表を使わないことも確かめる。

## 4. C の道具の不具合と直し（`e04f06a`）

試験は `test_evtools.py`。16 件すべて C の道具で失敗し、C++ で通ることを確かめた。

| 試験 | 道具 | C の動き | 直したあと |
|---|---|---|---|
| src_drive | evsearch・evtrain | データの場所 `D:\...` を `:` で切り、0 件のまま学習（evtrain は「bad -src」） | ドライブ名を読む |
| base_drive | evsearch | `-base D:\...:cfg` で「cannot open D/...」 | 同上（最後の `:` で分ける） |
| src_bad | evsearch | 重みのないデータの場所を、重み 0 で学習 | 「bad -src」で止まる |
| many_instances | evsearch | 1 構成 256 か所を超えると配列の外に書く（異常終了） | 止まる |
| many_sources | evsearch・evtrain | 9 個目の `-src` で配列の外（異常終了） | 止まる |
| many_plies | evsearch | 65 個目の手数で配列の外 | 止まる（手数は 0〜60） |
| bad_ply_range | evtrain | `-plies 60-61` で重みの表の外に書く | 止まる |
| pattern_bad | evsearch | 升の欠けた行で NULL を読む、`z9`・17 升・升の数 −1 を黙って使う | 止まる |
| missing_files | 13 本 | 入力のファイル・出力のフォルダがないと異常終了 | 「cannot open / create」で止まる |
| out_of_memory | evsearch・evtrain | 確保の失敗で異常終了（40MB の制限で試す） | 「out of memory」で止まる |
| short_weights | w2dat2・evsearch -base | 途中で切れた重みのファイルを読み続ける | 止まる |
| book_form2 | bookdump・bookscan | 形2の book を形1として読む | 止まる（形2は `edax -book-convert` で形1に） |
| selfplay_random_range | selfplay3 | `-rmin` が `-rmax` より大きいと全部の手が乱数（局面 0） | 止まる |
| gen_low_ply | gen | 開始が手数 40 未満だと、開いていないファイルに書く（異常終了） | その手数のファイルを必要なときに開く |
| gen_no_training | gen | 試験用（1/32）の局面しかないと終わらない、空のファイルで 0 で割る | 止まる |
| leafgen_keep_all | leafgen | 全部使うとき、しきい値 1.0×2^64 の整数への変換が AVX-512 では 2^64−1、ほかのビルドでは 2^63 になり、約半分しか使わない（AVX2 のビルドで 19,367 局面のうち 9,652） | `keep >= 1` なら `~0`（evsearch と同じ書き方）。19,367 |

ほかに：eval.dat 形2（`NEDXEVAL`）や eval.dat でないファイルを形1として読んでいた所（`evtool` の `et_read`）を断る形に。値の要る選択肢が最後にあると範囲の外を読んでいた所を「missing value」に。1 手数の記録の数は `int` のまま、2^31−1 を超える前に止める。

C との違いは、誤った入力のときの動きと、`label`・`gen` に足した `-eval`（既定は今までどおり `eval_orig.dat`）だけ。正しい入力の出力は C と同じ（5章）。

## 5. 結果が同じことの確かめ（確かめた）

`compare_c.py`：24 件すべてで、C++ の出力（ファイル・標準出力・エラー出力）が C と 1 バイトも違わない（経過時間の表示を除く）。

| 件 | 道具 | 中身 |
|---|---|---|
| E1〜E4 | evsearch | 46 パターン＋mob に `-prior`・`-lambda-prior`、全 11 数値特徴に `-teacher -alpha`、`-base`、`-notie`・低い手数 |
| T1・T2 | evtrain | `-window 1 -calib 0`、`-eval -test 16` |
| W1〜W3 | w2dat2 | mob・mobp・なし（`ev\pr\Rm_l100` の重み） |
| S1・P1・B1・B2 | sprepair・sample・bookdump・bookscan | 境目を崩した記録、試験用の局面、book の頭 2 万局面 |
| G1・G1e | selfplay3 | 20 局 level 4 空き 10 から完全読み。上流の eval.dat（nikque.17 の C と）、eval2.dat（eval2.5 の C と） |
| L1・L1e | label | 300 局面 level 6 |
| O1・O1e | playout | 30 局面 level 4 空き 14 から完全読み |
| N1・N1e | gen | 5 本、手数 41〜44 を完全読み |
| F1・F1e | leafgen | 10 局面 level 6、1/50 |
| K1 | evcheck | 300 局 |

ほかに：

- `w2dat2` で `ev\pr\Rm_l100` の重みから作った eval.dat は、eval2.5 に入っている `eval.dat` と、版の番号（試験で渡した 4.5.5 と公開の 3.3.0）の 3 バイト以外同じ。公開した eval2 の eval.dat は道具で作り直せる。
- 再ビルドした C の `evsearch.exe` は、`ev` にある exe と日付の 6 バイト以外同じ（同じ MSVC 14.44）。

## 6. 速さ・メモリ・大きさ（C の道具との比べ）

### 6.1 機械語（CPU を使わない確かめ）

- `evsearch`：データ全体を回す繰り返し（`run_config` の OpenMP の各部分）は C と同じ機械語（1 つは足し算の順を入れ替えただけの同じ命令）。`run_config` の本体では、C++ だけが要素ごとの計算（`p[i] = z[i] + beta * p[i]` の形）をベクトル命令にしている（結果は同じ）。
- `evtrain`：`apply_A`・`residual`・`errors`・`reduce` は同じ。
- 直し（4章）で増えた命令は、確保・ファイル・選択肢の確かめと、記録を足す関数の「入れ物を広げる」側だけ。

### 6.2 時間（1 論理CPU、C と C++ を交互に。プロセスの CPU サイクル）

同じ機械語どうしでも 1 回ごとに数% 揺れる（再ビルドした C の `evsearch` と元の exe で、2 本目が毎回 6〜8% 遅く出て、順を逆にすると消えた）。そのため交互に流し、最後は組ごとの比（同じ回の C と C++）で見た。

| 道具（件） | 直す前（`f145d16` ほか） | 直したあと（`e04f06a`、6 回） | 最終（`set_grow` を分けたあと、12 回、組ごとの比） |
|---|---|---|---|
| evsearch E1 | 0.973 | 0.998 | **0.974**（12 組のうち 11 組で C++ が速い） |
| evsearch E2 | 0.997 | 1.010（12 回：組ごと 1.004、5/12） | （E2 の読み込みは E1 と同じ道） |
| evtrain T1 | 0.984 | 1.006（12 回：組ごと 1.012、4/12） | **0.990**（8/12） |
| evtrain T2（-eval） | — | — | **0.977**（10/12） |
| w2dat2 W1 | 0.998 | 1.013（12 回：組ごと 0.958、10/12） | — |
| sprepair S1 | 1.011（40 回。起動だけでは C 55.5M・C++ 54.1M、仕事の大部分が起動） | — | — |
| sample・bookdump・bookscan | 0.986・1.018・0.992 | — | — |
| selfplay3 G1 | 1.002（12 回） | 1.006 | — |
| label L1 | 1.000 | 1.000 | — |
| playout O1 | 1.000（12 回） | 1.020（12 回：組ごと 0.980、8/12） | — |
| gen N1・leafgen F1・evcheck K1 | 0.997・0.992・0.986 | 0.952・0.998・— | — |

- `evtrain` が直したあとに組ごと 1.012 になった原因を調べた：記録ごとに呼ぶ `set_push` に、入れ物を広げる側の確かめを足したため、関数の入口で退避するレジスタが増えた。広げる側を別の関数（インライン化しない `set_grow`・`data_grow`）に分けて、0.990・0.977 に戻った。`evsearch` の `push` は 34 命令になり（C は 56）、読み込みの関数に取り込まれた。
- 直したあとの 12 回の E2・T1・W1・O1 の最初の組は、計測の窓（論理CPU 0）と重なったので止め、論理CPU 10 で流し直した値。

### 6.3 メモリと大きさ

- 確保したメモリ（private の最大）は、どの道具も C と同じ（E1：C 90,580〜90,628 KiB、C++ 90,596〜90,604 KiB。playout：両方 40,964〜40,996 KiB。evtrain T1：両方 631,688〜631,712 KiB）。
- 作業メモリ（working set の最大）：段1の道具は同じ（E1：C 86,348〜86,492 KiB、C++ 86,264〜86,328 KiB。evtrain T1：C 103,848〜103,936、C++ 103,792〜103,872 KiB）。**探索を使う道具は C++ が 16 KiB 多い**（playout：C 44,684、C++ 44,700 KiB、4 回とも）。原因：実行ファイルが大きく、読み込まれるページが 4 つ増えるため（確保量は同じ）。
- 実行ファイルの大きさ（直したあと、バイト）：

| 道具 | C | C++ | 差 |
|---|---|---|---|
| evsearch | 213,504 | 220,672 | +7,168 |
| evtrain | 215,040 | 218,112 | +3,072 |
| w2dat2 | 188,416 | 190,976 | +2,560 |
| sprepair・sample・bookdump | | | +1,024 |
| bookscan | 155,648 | 155,136 | −512 |
| selfplay3 | 345,088 | 353,792 | +8,704 |
| label | 343,040 | 349,184 | +6,144 |
| playout・leafgen・evcheck | | | +7,168 |
| gen | 359,424 | 367,616 | +8,192 |

原因：直しで足した確かめ（段1）と、new edax 本体が C より大きいこと（book・eval の形2、CRC32C など。new edax 本体も x64 AVX-512 で C 版より +29,184 バイト）。直す前の段1は +1,024〜−512 バイトだった。

## 7. 測っていない・確かめていないこと

- 複数スレッドでの出力（C も書き終えた順にファイルへ足すので、順が実行ごとに変わる。`evsearch` の OpenMP の `reduction` は足す順が決まっていない）。比べは 1 スレッドだけ。
- 大きい量（1 手数 1000 万局面以上）での時間と最大メモリ。データの並びは C と同じなので、メモリは同じになる見込み（見立て）。
- AVX-512 以外のビルド（`leafgen` の AVX2 だけ試した）、x86、Linux・ARM でのビルド。
- 学習・生成の本番。

## 8. 次の候補

- 道具の出力を new edax の形に：`w2dat2` の後継で eval.dat 形2（着手可能数の表を形2に持たせるには、形2の設計を足す必要がある）、64 枠（パターンが決まってから）。
- `evsearch` に隣の同じ偶奇の手数を混ぜる窓（`HANDOFF-eval4.md` 5.1 の 0-6）。
- 公開データ v0002 の棋譜を記録に変える道具（同 0-2）、指されなかった手を記録する自己対局（同 0-5）。
- `bookdump` が形2の book を直接読む（今は形1に変換してから）。
