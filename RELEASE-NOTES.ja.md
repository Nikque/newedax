# newedax 修正一覧

[English](RELEASE-NOTES.md) · [README](README.md) · [マニュアル](manual/ja/README.md) · [テストと実測の記録](docs/validation/newedax.ja.md)

newedax（C 版 Edax 4.5.5 nikque の eval2 系列を C++23 に移したもの）の、版ごとの利用者向けの変更点です。テストと実測の詳しい記録は、各版の末尾のリンク先にあります。

C 版から取り込んだ変更の詳細は、[C 版の修正一覧](https://github.com/Nikque/edax-reversi-AVX/blob/edax-4.5.5-fixes/RELEASE-NOTES.ja.md) を見てください。

## C 版との対応

各版は、同じ行の C 版の変更をすべて含みます。

| newedax | C 版（nikque） | C 版（eval2） |
|---|---|---|
| newedax.7 | nikque.22 | eval2.10 |
| newedax.6 | nikque.21 | eval2.9 |
| newedax.5 | nikque.20 | eval2.8 |
| newedax.4 | nikque.19 | eval2.7 |
| newedax.3 | nikque.18 | eval2.6 |
| newedax.2 | nikque.18 | eval2.6 |
| newedax.1 | nikque.17 | eval2.5 |

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
