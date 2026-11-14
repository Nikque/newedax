# newedax 修正一覧

[English](RELEASE-NOTES.md) · [README](README.md) · [マニュアル](manual/ja/README.md) · [テストと実測の記録](docs/validation/newedax.ja.md)

newedax（C 版 Edax 4.5.5 nikque の eval2 系列を C++23 に移したもの）の、版ごとの使う人に関わる変更点です。テストと実測の詳しい記録は、各版の末尾のリンク先にあります。

C 版から取り込んだ変更の詳細は、[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md) を見てください。

## C 版との対応

各版は、同じ行の C 版の変更をすべて含みます。

| newedax | C 版（nikque） | C 版（eval2） |
|---|---|---|
| newedax.13 | nikque.27 | eval2.15 |
| newedax.12 | nikque.27 | eval2.15 |
| newedax.11 | nikque.26 | eval2.14 |
| newedax.10 | nikque.25 | eval2.13 |
| newedax.9 | nikque.24 | eval2.12 |
| newedax.8 | nikque.23 | eval2.11 |
| newedax.7 | nikque.22 | eval2.10 |
| newedax.6 | nikque.21 | eval2.9 |
| newedax.5 | nikque.20 | eval2.8 |
| newedax.4 | nikque.19 | eval2.7 |
| newedax.3 | nikque.18 | eval2.6 |
| newedax.2 | nikque.18 | eval2.6 |
| newedax.1 | nikque.17 | eval2.5 |

---

## v4.5.5-newedax.13

**新しい評価関数：64 枠で学習し直したものと、48 枠の版。** 配布物は 2 つです。`edax-4.5.5-newedax.13-64.zip`（既定の 64 枠）と `edax-4.5.5-newedax.13-48.zip`（48 枠：64 枠から形を 2 つ外したもの。1 節点あたりが速い）。それぞれに専用の `eval.dat`（107,612,578 バイト）が入っています。リポジトリの `bin/data` に残っている eval2 の `eval.dat` は、これらの版には使えません。同じ持ち時間（1 局 30 秒ずつ）で、64 枠の版は eval2（v4.5.5-newedax.12）に **+60.2 Elo**（95%：+47.9〜+72.6）、Egaroucid（大会用の枝、1 局 120 秒、8 スレッド）に **+23.2 Elo**（+3.0〜+43.6）でした。ProbCut の既定は `refit` になりました。

**変更点**

- 評価関数（既定の版、`EVAL_SLOTS` 64）：Edax 4.5 の 13 群と追加の 13 群（`M X W C`、3^10 マスまたは 3^9 の重み）、15 個の形と着手可能数の重みを 64 枠に置きました。形は 2026-11-05〜11-13 に、評価の誤差と Lv18・Lv24 の対局で探しました（形とマスは下の表）。重みは `tools/eval/evsearch`（手数 2〜53）で学習し直しました。
- 48 枠の版（`EVAL_SLOTS=48`）：64 枠と同じソース・同じ形式の `eval.dat` で、形 `h1825_145` と `h1723_10`（16 枠）を外したもの（特徴の差分更新は 256 ビットのベクトル 4 本のかわりに 3 本）。重みはこの 2 つを外して学習しました。同じ時間で約 11% 多く読みますが、評価の落ちのほうが大きく、同じ時間で 64 枠に −12.1 Elo（−17.3〜−6.9）でした。
- `EVAL_SLOTS` 48 は eval2 の意味ではなくなりました。eval2 の評価関数（46 パターン）は組み立てられません。
- ProbCut：既定を `refit` にしました（`-probcut-model refit`、`bin/config.ini` の `probcut-model = refit`）。この重みで `standard` と比べた結果：64 枠で `standard` は −1.0 Elo（−5.9〜+3.8、4,378 局）、48 枠で −7.7（−14.8〜−0.7、2,204 局）。
- 探索：待っているスレッドが眠る前に見回るようにし、深さ 9 から分けます。AVX-512 の版は、手数ごとの重みを 1 つの表から 1 つずつ読みます。
- 道具（`tools/eval`）：`evshape`、`evsearch -mute`・`-method sd`、`w2dat2 -interp` と追加の群、`selfplay3 -h`、`selfplay3` と `label` の探索の refit、`pcsigma`。
- 評価ファイル：64 枠・48 枠の版は、追加の群の重みの無いファイル（eval2 や Edax 4.5 の `eval.dat`）を読むと警告を出します。
- 試験（`tests/regression.cpp`）：`evalslots` が AVX-512 の版の表も読み、ファイルの追加の群の重みを数に入れ、差分更新をすべての枠で確かめるようにしました。形式 1 のファイルの試し（`evalform2`・`evalform2mob`・`evalbadconv`・`evalmobility`）は、形式 2 の `eval.dat` の Edax 4.5 の 13 群から形式 1 のファイルを作って使います。

64 枠の形（枠：使う像の数）：

| 形 | マス | 枠 |
|---|---|---|
| `c10`（Edax） | a5 a4 a3 a2 a1 b2 b1 c1 d1 e1 | 4 |
| `s100`（Edax） | b2 a1 b1 c1 d1 e1 f1 g1 h1 g2 | 4 |
| `xm` | b1 c1 d1 e1 f1 g1 c2 d2 e2 f2 | 4 |
| `xw` | b2 c2 d2 e2 f2 g2 c3 d3 e3 f3 | 4 |
| `f208_1` | a1 b1 c1 f1 g1 h1 a2 c2 f2 h2 | 4 |
| `f203_77` | c1 e1 d2 f2 e3 g3 f4 h4 g5 h6 | 4 |
| `f1_47` | g5 h5 g6 h6 e7 f7 h7 e8 f8 g8 | 4 |
| `c8_b3b6` | c1 c2 b3 c3 c4 c5 b6 c6 c7 c8 | 4 |
| `d5a_124` | d1 e2 d3 e3 f3 e4 f4 g4 f5 h5 | 4 |
| `f1144_5` | a1 b1 c1 g1 a2 b2 c2 a3 b3 a7 | 4 |
| `d7x_b2g7a8` | b1 b2 c2 d3 e4 f5 g6 g7 h7 a8 | 4 |
| `p504` | a1 h1 b2 c3 d4 e5 f6 g7 a8 h8 | 2 |
| `d3z_e2g4b5d7` | c1 b2 e2 a3 g4 b5 h6 d7 g7 f8 | 2 |
| `h1825_145`（48 枠には無い） | c1 d1 g1 h1 b2 d2 f2 g2 h2 f3 | 8 |
| `h1723_10`（48 枠には無い） | g5 f6 e7 f7 g7 d8 e8 f8 g8 h8 | 8 |

**互換性**

- 探索の結果は変わります（評価関数と ProbCut の既定）。book の形式は変わりません。前の版で学習した book は読めますが、その値は前の評価関数でつけたものです。
- `eval.dat`：それぞれの版の `eval.dat` は配布物の ZIP（`bin/data/eval.dat`）にあり、git にはありません。リポジトリから組み立てた版にも必要です。`bin/data` の eval2 の `eval.dat` と Edax 4.5 の `eval.dat` は、64 枠・48 枠の版には使えません（警告を出して読みますが、評価が正しくなりません）。
- 64 枠と 48 枠の `eval.dat` は同じ形式ですが、それぞれ自分の版のためのものです。
- libedax：API は v4.5.5-newedax.12 と同じです。ライブラリを同梱するプログラムは、同じ版の `eval.dat` も同梱してください。
- `-probcut-model standard` で、前の ProbCut の形になります。

**測った結果**（64 枠の版、refit、PGO の組。B が新しい版、95% の幅）：

| 相手 | 条件 | 局数 | Elo |
|---|---|---|---|
| eval2（v4.5.5-newedax.12、standard） | 1 局 30 秒ずつ | 1,102 | +60.2（+47.9〜+72.6） |
| 11/05 の重み（64 枠、refit） | 1 局 30 秒ずつ | 2,204 | +11.2（+3.7〜+18.7） |
| 元の `eval.dat` の Edax | 両方 Lv18 | 4,378 | +77.5（+69.9〜+85.2） |
| Egaroucid（大会用の枝 `dev-lazysmp-ggs-b`） | 1 局 120 秒、8 スレッド | 180 | +23.2（+3.0〜+43.6） |
| 48 枠の版（refit） | 1 局 30 秒ずつ | 4,378 | +12.1（+6.9〜+17.3） |

**確かめていないこと**：Android の実機、Windows ARM64、book の速さ、Windows の MSVC と Android の NDK 以外での組み立て。

[試験と測定](docs/validation/newedax.ja.md#v455-newedax13)

## v4.5.5-newedax.12

C 版と同じく、起動のときにほかのプログラムが開いていた book のファイルを、壊れたものとして置き換えることがなくなりました。見直しで見つけたほかの不具合も直し、動きを変えずにコードを整理しました。評価の道具（`tools/eval`）は、長すぎるファイル名とメモリの確保の失敗を、誤りとして止めます。

**変更**

- C 版 v4.5.5-nikque.27 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque27)）：起動のときに開けない book のファイルは 2 秒ほど待って開き直し、それでも開けなければそのファイルには保存しない（`.damaged` も作らない）こと、読めなかった book のファイルの印、libedax の `edax_book_file_status`、`book import`・`book merge`・`book deviate` の仲間の `export`・`book moveinfo`・`book reducecount`・時間の `5:`・`config.ini` の場所・`weval` の表と統計・libedax などの直し、動きを変えないコードの整理。
- 評価の道具（`tools/eval`）：フォルダーの名前から作るファイルの名前が入れ物に収まらないときは、`file name too long` と出して終了コード 1 で止まります（10 本の 22 か所。前は入れ物の外に書き、`sprepair`・`bookdump`・`w2dat2` は落ちるか、壊れた名前で続けていました）。`leafgen`・`w2dat2` は、メモリを確保できないときに `out of memory` で止まります。ファイルを開けないとき・メモリが足りないときの止め方を、同じ関数（`xfopen`・`xmalloc`・`xcalloc`・`die`）にまとめました（文と終了コードは同じ）。
- 試験：`tests/libedax_test.c` は 281 項目（61 項目増。うち 28 項目は Windows だけ）、`tests/book-analyze.cpp` は 3 つ増、`tests/regression.cpp` は C 版と同じ場面を足しました。`tests/regression.cpp` の `evalmobility` は、移動力の重みを持つ newedax の `eval.dat` でも通ります（移動力の重みのない評価のファイルを、61 手の重みだけを書いて作ります。前は、その `eval.dat` では必ず失敗していました）。
- 手引き：4.1・6.6・7.3・8.4・8.5・12.2・12.4・13.1 に書きました（C 版と同じ文。12.2 の試験の項目数のほか）。

**互換性**

- 探索の結果・`eval.dat`・book のファイル形式は変わりません。
- 答えが v4.5.5-newedax.11 と違うのは、上の場合だけです。book のエラーの文に入るソースの行番号は変わりました。
- libedax：設定の book のファイルを起動のときに開けなかったときは、どの保存もそのファイルを置き換えません（`edax_book_save_checked`・`edax_book_keep_unread_file` は 0）。book を自分で保存するプログラムは、初期化の後に `edax_book_file_status` を確かめてください。
- 評価の道具は、長すぎる名前のときのほかは、newedax.11 の道具と同じ入力で同じ出力です。

**確認**：C 版と同じく、直す前と後で、試験用の book（1 スレッドと 4 スレッド）の book のコマンド 76 通りの書いたファイルと答え、解析の命令 20,046 個の答え、ほかのコマンド 25 場面の出力（時間と book のソースの行番号のほか）が同じでした。libedax の試験 281 項目と解析の単体試験が通り、gcc 11（WSL）の組み立ては警告が前と同じで、そのライブラリは 251 項目（Windows だけの項目のほか全部）が通りました。答え 6,880 個が C 版 v4.5.5-nikque.27 と同じです。評価の道具は、newedax.11 の道具と同じ入力で 26 事例の出力が同じでした（そのソースから組み立てたものとも、配布物のものとも。入力に実 book は使っていません）。

**未確認**：実 book での時間とメモリ、Android の実機、評価の道具のメモリが足りないときの道（読んで確かめた）。

[検証記録](docs/validation/newedax.ja.md#v455-newedax12)

---

## v4.5.5-newedax.11

C 版と同じく、`book deviate`・`book deviate2`・`book deviate3` に、book を広げずに、たどる手順をテキストファイルに書き出す `export` を足しました。

**変更**

- C 版 v4.5.5-nikque.26 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque26)）：`export <ファイル> [leaf|ends|all]`、libedax の `edax_book_deviate_export`、`@cancel id=N state=none export=1` の印。
- 手引き：7.3 と 12.4 に書きました（C 版と同じ文）。

**互換性**

- `export` を付けないコマンドは v4.5.5-newedax.10 と同じです。探索の結果・`eval.dat`・book のファイル形式は変わりません。

**確認**：C 版と同じ書き出しの試験（試験用の book 3 つで 4,545 通り）を流しました。

**未確認**：実 book での時間とメモリ、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax11)

---

## v4.5.5-newedax.10

C 版と同じく、うみがめ数の `exact=0` が付く値が、組み立てた環境によらず同じになるようにしました。

**変更**

- C 版 v4.5.5-nikque.25 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque25)）。
- 手引き：12.4 に書きました（C 版と同じ文）。

**互換性**

- `exact=0` が付く値は、v4.5.5-newedax.9 と変わることがあります（どちらも上限です）。ほかの答えは同じです。
- 探索の結果・`eval.dat`・book のファイル形式は変わりません。

**確認**：C 版と同じ試験（`tests/book-analyze.cpp`、`umigame-order` を含む）が全部通り、数は C 版と同じでした。

**未確認**：実 book での速さ、GUI での動き、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax10)

---

## v4.5.5-newedax.9

GUI の候補手の表のうみがめ数・局面の数と、`book levels`・`book missing` を、C 版と同じく、前の局面の答えから引き継いで、同じ計算をし直さないようにしました。

**変更**

- C 版 v4.5.5-nikque.24 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque24)）。
  - うみがめ数：手の後の局面の網を数えるとき、その中の 1・2 手先の局面の値も覚え、その局面が今の局面になったときに網を作り直しません。
  - 局面の数（`part=count`）と `book levels`：局面ごとの答えを、book が変わるまで覚えます。
  - `book missing`：覚えている精度の段階から、聞いた範囲で何も見つからないとわかるときは、歩かずに 0 本と答えます。
- 手引き：12.4 に上の 3 つを書きました（C 版と同じ文）。

**互換性**

- `book moveinfo`・`book levels`・`book missing` の答えは v4.5.5-newedax.8 と同じです。ただし、その局面だけから数えると `exact=0` が付くうみがめ数が、前の局面で覚えたちょうどの値（`exact=0` なし、より小さいか同じ値）になることがあります。
- 探索の結果・`eval.dat`・book のファイル形式は変わりません。

**確認**：C 版と同じ試験（`tests/book-analyze.cpp`）で、引き継いだ答えは、その局面だけから求めた答えと全部の手で同じでした。v4.5.5-newedax.8 と同じ命令（試験用の book 2 つ、条件 5 つ）を流した `@` の行は、newedax.8 とも C 版とも同じでした。

**未確認**：実 book での速さ、GUI での動き、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax9)

---

## v4.5.5-newedax.8

GUI の候補手の表の精度（`book moveinfo` の `stage=`）を、C 版と同じく、前後の局面の答えから引き継いで、歩く量を減らしました。book のコマンドを `book` なしで打ったときに、打ち方を案内します。手引きと文書を書き直しました。

**変更**

- C 版 v4.5.5-nikque.23 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque23)）。
  - 覚えている今の局面の段階と、手の後の局面の先の局面の段階からわかる所は、歩かずに決めます。答え（`stage=`・`missing=`）は v4.5.5-newedax.7 と同じです。
  - 最上位で book のコマンドの名前（`leaf expand 30 30`・`fill2 2` など）を打つと、警告に `(book command: type "book leaf expand 30 30" or "b leaf expand 30 30")` を添えます。最上位のコマンド・設定・`base` のコマンドと同じ名前には添えません。
- 手引き：C 版と同じ見直し（言い回し、book のコマンドを全部 `book …` の形で書く、9.4 の保存の時間「18〜28 秒」、4 章の `refit` の説明から C 版の修正一覧の nikque.5 へのリンク）。12.4・13.2 に上の 2 つの変更を書きました。
- 文書：README と修正一覧を書き直し、テストと実測の記録を `docs/validation/` へ移し、`README-NIKQUE.*` を消しました。配布物には `README-NIKQUE.*` の代わりに `README.md` が入ります。`config.ini` の初めの説明の行は、手引きの 4 章を指します。

**互換性**

- `book moveinfo` の答えは v4.5.5-newedax.7 と同じです。
- 知らないコマンドの警告の行は、book のコマンドの名前のときだけ、後ろに案内が付きます（前の部分は同じです）。
- 探索の結果・`eval.dat`・book のファイル形式は変わりません。

**確認**：C 版と同じ試験（`tests/book-analyze.cpp`）で、引き継いだ段階は局面ごとに歩いた段階と全部の手で同じ（作り物の book 200 冊と試験用の book 2 つ）。`book moveinfo` の答えは、v4.5.5-newedax.7 とも C 版とも同じでした（試験用の book 2 つ、棋譜 2 局の各局面、基準 3 つ）。

**未確認**：実 book での速さ、GUI での動き、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax8)

---

## v4.5.5-newedax.7

`book reduce`・`book reducecount` を、C 版と同じく速く、省メモリにしました。

**変更**

- C 版 v4.5.5-nikque.22 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque22)）。
  - `book reducecount` が速くなり、直後の `book reduce` は数えた結果を使い回します。削減後は使わなくなったメモリをすぐに解放します。
  - 新しい設定 `book-reduce-index`・`book-reduce-reuse`・`book-reduce-shrink`・`book-reduce-walk` を追加しました（どれも既定のままで使えます）。
- コマンドの答えは C 版 v4.5.5-nikque.22 と同じです。

**互換性**

- 残る局面と局面ごとの中身は v4.5.5-newedax.6 と同じです。
- 探索の結果・`eval.dat`・book のファイル形式は変わりません。

**未確認**：newedax での速さ（29 GB の実 book での `book reducecount` の速さは C 版でだけ測っています）、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax7)

---

## v4.5.5-newedax.6

`book reduce`・`book reducecount` に、C 版と同じ不具合の修正と組み直しを入れました。

**変更**

- C 版 v4.5.5-nikque.21 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque21)）。
  - `book reducecount` が `n-tasks` のスレッド数で並列に動くようになりました。
  - 壊れた book で `book reducecount` が終わらない不具合と、誤った入力でも局面を消していた不具合を直しました（今はエラーまたは警告を出し、book を変えません）。
- コマンドの答えは C 版 v4.5.5-nikque.21 と同じです。

**互換性**

- 正常な book では、残る局面と `book reduce` の後に保存する book は v4.5.5-newedax.5 と同じです。
- 探索の結果・`eval.dat`・book のファイル形式は変わりません。

**未確認**：29 GB の実 book、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax6)

---

## v4.5.5-newedax.5

book を小さくするコマンド `book reduce` と、事前に数を確かめる `book reducecount` を追加しました。

**変更**

- C 版 v4.5.5-nikque.20 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque20)）。
  - `book reduce X Y` は、`book deviate3 X Y` がたどる範囲の局面だけを残し、ほかを消します。`book reducecount` は、残る数・消える数を数えます（book は変えません）。
- コマンドと答えは C 版 v4.5.5-nikque.20・v4.5.5-eval2.8 と同じです。
- 使い方：[マニュアル 8.4](manual/ja/08-book-maintenance.md)・[12.4](manual/ja/12-integration.md)

**互換性**：探索の結果・`eval.dat`・book のファイル形式は v4.5.5-newedax.4 と変わりません。

**未確認**：新しいコマンドの速さ、29 GB の実 book、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax5)

---

## v4.5.5-newedax.4

GUI のプログラム向けに、book を解析するコマンドを追加しました。

**変更**

- C 版 v4.5.5-nikque.19 の変更を取り込みました（[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque19)）。
  - 新しいコマンド：`book moveinfo`・`book missing`・`book levels`・`book bestlines`・`book findline`、`stop id=N`。
- コマンドと答えは C 版 v4.5.5-nikque.19・v4.5.5-eval2.7 と同じです。
- 新しいコマンドは C 版より遅いものがありません（1,914 万局面の book、1 スレッドで、初手からの局面の数は C 版の 5.92 秒に対して 5.03 秒）。book だけの作業メモリは C 版の 1,235 MB に対して 880 MB です。
- Windows の実行ファイルのビルド方法は v4.5.5-newedax.3 と同じです（`wEdax-x86-64-v4.exe` は PGO なし、ほかの 4 種は PGO）。
- 使い方：[マニュアル 12.4](manual/ja/12-integration.md)

**互換性**：探索の結果・`eval.dat`・book のファイル形式は v4.5.5-newedax.3 と変わりません。

**未確認**：29 GB の実 book、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax4)

---

## v4.5.5-newedax.3

Windows の実行ファイルのビルド方法を変えて、1 スレッドの速さを C 版に合わせました。プレリリースではない、最初の正式版です。

**変更**

- Windows の実行ファイルは、`wEdax-x86-64-v4.exe` が PGO（実行の記録を使う最適化）なし、ほかの 4 種が PGO でビルドしたものになりました（`tools/port/build-release.cmd`）。v4.5.5-newedax.2 はすべて PGO なしでした。
- `wEdax-x86-64-v4.exe` は PGO なしの方が速いため、v4.5.5-newedax.2 と同じ機械語のままです。DLL 3 種と Android も PGO なしで、機械語は変わりません。
- 速さ（C 版 v4.5.5-eval2.6 との比）：1 スレッドの中盤で、`wEdax-x86-64-v4.exe` は C 版より約 3〜4% 速くなっています（0.961〜0.969）。`wEdax-x86-64.exe`・`wEdax-x86-sse.exe`・`wEdax-x86.exe` は C 版とほぼ同じか速いです。
- **`wEdax-x86-64-v3.exe` は C 版より約 1% 遅いままです**（1 スレッドの中盤、32 スレッドの `book learn`・`fill` 系）。AVX-512 の CPU では `wEdax-x86-64-v4.exe` を使ってください。
- 最大メモリは C 版と同じか少なくなっています（`book import` 614 → 545 MiB）。C 版が作った 3,473 万局面の book の読み込みでは、最大メモリが 3,243 → 1,667 MiB です。
- eval2 のツールのうち `w2dat2`・`label` は、C のツールより約 0.7% 遅いです。ほかは同じか速いです。
- 計測の記録：`docs/port/MEASURE-2.md`。

**互換性**

- 探索の結果は v4.5.5-newedax.2 と同じです（v4.5.5-eval2.6 とも同じ）。
- `eval.dat`・book のファイル形式は変わりません。

**注意**：PGO は、ビルドのたびに機械語が少し変わります。配布する `-v3`・`wEdax-x86-sse.exe`・`wEdax-x86.exe` は、配布するものそのものを C 版と比べ直してあります。

**未確認**：`wEdax-x86-64-v3.exe` と `w2dat2`・`label` が遅い原因、Android の実機。

[検証記録](docs/validation/newedax.ja.md#v455-newedax3)

---

## v4.5.5-newedax.2

v4.5.5-newedax.1 のクロスチェックで見つかった不具合を直しました。1 スレッドで C 版より遅い実行ファイルが残るため、まだプレリリースです。

**変更**

- 実行ファイルとライブラリに、ビルドしたときのフォルダーが入らなくなりました（C 版 v4.5.5-nikque.18 と同じ直し。[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md#v455-nikque18)）。エラーの文言はソースのファイル名だけになります。
- ZIP の `tools/eval` に `vcomp140.dll`（MSVC の OpenMP の実行時ライブラリ）を入れました。7 つのツールが使います。
- ツールのスクリプトは、入力のフォルダーを引数で受け取ります（`--ev`・`--book40`・`-Inputs`。既定値なし）。
- Windows の実行ファイルは、v4.5.5-newedax.1 と同じく PGO なしです。
- 速さ（32 スレッドの `wEdax-x86-64-v4.exe`）：v4.5.5-newedax.1 と同じで、C 版 v4.5.5-eval2.6 と同じか速いです（book のコマンド 7 種で 0.959〜1.001 倍の時間）。最大メモリは同じか少ないです。

**不具合の修正**（eval2 のツールと `-eval-convert`）

- `bookdump`・`bookscan`：石が 4 つ未満の局面がある book で、手数が負になっていました（C のツールにも同じ不具合）。そのような book は受け付けません。
- ツールの設定で同じスカラーを 2 回書くと 2 回数えていました。パターンに同じマスが 2 つあると、対称の置き換えが壊れていました。どちらも受け付けません。
- 形式 2 の 5 バイトの数で、5 バイト目の上位 4 ビットを見ていませんでした（2^32−1 は読み、2^32 は受け付けません）。
- `build-evtools.ps1` は、ビルドできなかったツールがあっても終了コード 0 でした。1 を返します。
- `-eval-convert`：54 手数より少ない形式 1 のファイルを変換し、どちらの形式でも読めないファイルを作っていました。形式 2 で手数×重みの数が 32 ビットに入らないとあふれていました。どちらも受け付けません。

**互換性**：探索の結果は v4.5.5-newedax.1 と同じです（v4.5.5-eval2.6 とも同じ）。

**注意**：1 スレッドでは、`wEdax-x86-64-v4.exe` 以外の 4 種が C 版（PGO のビルド）より遅いです（中盤で最大 11.6%、`wEdax-x86-sse.exe`）。v4.5.5-newedax.3 で直しています。

**未確認**：既定のビルド（eval2）の強さ、Windows の MSVC と Android の NDK 以外のビルド、Android の実機、AddressSanitizer・ThreadSanitizer、数十億局面の実 book、別の人による読み直し。

[検証記録](docs/validation/newedax.ja.md#v455-newedax2)

---

## v4.5.5-newedax.1

new edax の最初の版です。C 版 v4.5.5-nikque.17 を C++23 に移植し、eval2 の系列（v4.5.5-eval2.5）の評価関数を既定にしました。別の人によるソースの確認と、既定のビルドの速さの計測がまだのため、プレリリースです。

**変更**

- 既定のビルドは、ビルドの選択肢 `EVAL_MOBILITY` が 1、`bin/data/eval.dat` が eval2 のものです。
- コマンドと選択肢は C 版と同じです。
- book と評価ファイルの形式 2（`NEDXBOOK`・`NEDXEVAL`）を追加しました。book は約 33% 小さくなり、CRC-32C がつきます。形式 1 も読めます。
- 形式を変えるオプション：

| オプション | 働き |
|---|---|
| `-book-format` | book のファイル形式を選ぶ |
| `-book-convert` | book の形式を変換する（両方向） |
| `-eval-convert` | 評価ファイルの形式を変換する（両方向） |

- book の局面がメモリ上で 40 バイトになりました（C 版は 48 バイト）。649 万局面の book の読み込みで、メモリが 11〜45% 少なくなります。
- 新しいビルドの選択肢：`EVAL_MOBILITY=0`（着手可能数の重みを読まない）、`EVAL_SLOTS=64`（64 枠。より強い評価関数の土台。既定は 48）。
- `tools/eval`：eval2 の評価関数の学習ツールの C++ 版（14 本。ZIP には x64 AVX-512 用だけ）。
- 速さ：C 版 v4.5.5-nikque.17 と同じビルドの指定（PGO なし）で比べて、C 版より遅い条件は見つかりませんでした（`docs/port/MEASURE-1.md`）。
- 配布版（MSVC 2022 `/O2 /GL`、**PGO なし**。Android は NDK r27d）：
  - Windows：`wEdax-x86-64-v4.exe`・`wEdax-x86-64-v3.exe`・`wEdax-x86-64.exe`・`wEdax-x86-sse.exe`・`wEdax-x86.exe`、`libedax-x64-v4.dll`・`libedax-x64-v3.dll`・`libedax-x64.dll`。
  - Android：`aEdax-arm64-v8a`・`aEdax-armeabi-v7a`・`libedax-arm64-v8a.so`・`libedax-armeabi-v7a.so`。
  - Linux・macOS・Windows ARM64 の実行ファイルはありません（ソースからのビルドは README の「Build」）。
  - PGO なしにしたのは、`book deviate` 系が PGO なしの方が約 1.5〜2% 速かったためです（C 版でも同じ）。ほかの条件では、PGO のあり・なしを直接は比べていません。

**互換性**

- 探索の結果は C 版と同じです。eval2 の `eval.dat` で v4.5.5-eval2.5 と、Edax 4.5 の `eval.dat` で v4.5.5-nikque.17 と同じです。
- 形式 1 の book と `eval.dat` はそのまま読めます。

**未確認**：既定のビルド（eval2）の速さと強さ、Windows の MSVC と Android の NDK 以外のビルド、Android の実機、メモリの誤りを検出するビルドでのテスト、数十億局面の実 book、別の人によるソースの確認。

[検証記録](docs/validation/newedax.ja.md#v455-newedax1)
