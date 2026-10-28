# newedax

The Othello engine [Edax 4.5.5 nikque](https://github.com/Nikque/edax-reversi-AVX) (its eval2 line) ported to C++23. It gives the same results as the C version, uses less memory for books, and is as fast or faster in most cases.

[日本語](#日本語) · [Download](https://github.com/Nikque/newedax/releases/latest) · [Manual](manual/en/README.md) · [Release notes](RELEASE-NOTES.md) · [Tests and measurements](docs/validation/newedax.en.md)

## Download and run

1. Download the ZIP from the [latest release](https://github.com/Nikque/newedax/releases/latest) and unpack it. It contains executables for Windows (x64, x86) and Android, the libedax DLLs, and the eval2 training tools.
2. Open the `bin` folder and start the executable from **inside that folder**. On Windows, start with `wEdax-x86-64.exe`; use `-v3` (AVX2) or `-v4` (AVX-512) if your CPU supports it.

The `bin/` folder of the repository holds only the data (`eval.dat` of eval2, `book.dat`, `config.ini`). Commands and settings are the same as the C version: see the [manual](manual/en/README.md).

For Linux and macOS executables, use the C version ([edax-reversi-AVX releases](https://github.com/Nikque/edax-reversi-AVX/releases)).

## Relation to the C version

- Evaluation: eval2 by default. The upstream Edax `eval.dat` also works.
- Commands, settings and results are the same as the C version (node counts, scores and lines checked on fforum-20-39 with both `eval.dat` files).
- Each newedax release contains all changes of a C release. The latest, **v4.5.5-newedax.9**, corresponds to C v4.5.5-nikque.24 / v4.5.5-eval2.12.

## Differences from the C version

- **Memory**: a book position takes 40 bytes (48 in the C version). Loading a book of 6.5 million positions takes 11 to 45 % less memory.
- **Speed** (Windows, against the C release eval2.6): `wEdax-x86-64-v4.exe` is slightly faster; the x64 standard and x86 builds are as fast or faster; **`wEdax-x86-64-v3.exe` is about 1 % slower**.
- **New file format (form 2)** for books (`NEDXBOOK`, about 33 % smaller) and evaluation files (`NEDXEVAL`), with a checksum. Convert with `-book-convert` / `-eval-convert` (both ways). Form 1 files (C version) are still read.
- **Build options**: `EVAL_MOBILITY` (default 1), `EVAL_SLOTS=64`.
- **[tools/eval](tools/eval)**: the eval2 training tools in C++ (`evsearch`, `evtrain`, `w2dat2`, `selfplay3`, ...).

## Not checked yet

Builds other than MSVC on Windows (Android: compiled and linked, not run on a device); AddressSanitizer / ThreadSanitizer; real books of billions of positions; review by a second person.

## Build

- Boost 1.92 (headers only) in `../third_party/boost_1_92_0`, or set `BOOST_DIR=<directory>`.
- Windows (Visual Studio 2022, x64 Developer Command Prompt): `build-win-v4.cmd`, or in `src`: `nmake -f NMakefile vc-x64-v3` (other targets and PGO builds: `src/NMakefile`).
- Release binaries: `tools/port/build-release.cmd <checkout> <log directory>`.
- Android: NDK r27d, `ndk-build`.

Port records (in Japanese): [docs/port](docs/port).

## License

GPL-3.0 ([LICENSE](LICENSE)). Based on Edax by Richard Delorme and [okuhara/edax-reversi-AVX](https://github.com/okuhara/edax-reversi-AVX). Boost is under the Boost Software License and is not included.

---

## 日本語

オセロのエンジン [Edax 4.5.5 nikque](https://github.com/Nikque/edax-reversi-AVX)（eval2 の系列）を C++23 に移したものです。C 版と同じ結果を出し、book のメモリが少なく、ほとんどの場合 C 版と同じか速く動きます。

[ダウンロード](https://github.com/Nikque/newedax/releases/latest) · [マニュアル](manual/ja/README.md) · [修正一覧](RELEASE-NOTES.ja.md) · [テストと実測の記録](docs/validation/newedax.ja.md)

### 使い始め方

1. [最新の Release](https://github.com/Nikque/newedax/releases/latest) の ZIP をダウンロードして展開します。Windows（x64・x86）と Android の実行ファイル、libedax の DLL、eval2 の学習ツールが入っています。
2. `bin` フォルダーを開き、**そのフォルダーの中から**実行ファイルを起動します。Windows なら、まず `wEdax-x86-64.exe` を使ってください。CPU が対応していれば `-v3`（AVX2）・`-v4`（AVX-512）が速くなります。

リポジトリの `bin/` にはデータ（eval2 の `eval.dat`、`book.dat`、`config.ini`）だけがあります。コマンドと設定は C 版と同じです（[マニュアル](manual/ja/README.md)）。

Linux・macOS 用の実行ファイルは C 版（[edax-reversi-AVX の Releases](https://github.com/Nikque/edax-reversi-AVX/releases)）を使ってください。

### C 版との関係

- 評価関数は eval2 が既定です。元の Edax の `eval.dat` も使えます。
- コマンド・設定・結果は C 版と同じです（fforum-20-39 で、2 種類の `eval.dat` のどちらでもノード数・評価値・読み筋が一致することを確認）。
- newedax の各版は、C 版のある版の変更をすべて含みます。最新の **v4.5.5-newedax.9** は、C 版 v4.5.5-nikque.24・v4.5.5-eval2.12 に対応します。

### C 版との違い

- **メモリ**：book の 1 局面が 40 バイト（C 版は 48 バイト）。649 万局面の book の読み込みで、メモリが 11〜45% 少なくなります。
- **速さ**（Windows、C 版 eval2.6 の配布物と比較）：`wEdax-x86-64-v4.exe` はわずかに速く、x64 標準版・x86 版は同じか速い。**`wEdax-x86-64-v3.exe` は約 1% 遅い**です。
- **新しいファイル形式（形式 2）**：book（`NEDXBOOK`、約 33% 小さい）と評価ファイル（`NEDXEVAL`）。チェックサム付き。`-book-convert`・`-eval-convert` で相互に変換できます。C 版の形式 1 のファイルもそのまま読めます。
- **ビルドオプション**：`EVAL_MOBILITY`（既定 1）、`EVAL_SLOTS=64`。
- **[tools/eval](tools/eval)**：eval2 の学習ツールの C++ 版（`evsearch`・`evtrain`・`w2dat2`・`selfplay3` など）。

### まだ確認していないこと

Windows の MSVC 以外でのビルド（Android はリンクまで、実機では未実行）、AddressSanitizer・ThreadSanitizer での試験、数十億局面の実際の book、別の人によるレビュー。

### ビルド

上の英語の「Build」を見てください。移植の作業記録は [docs/port](docs/port) にあります。

### ライセンス

GPL-3.0（[LICENSE](LICENSE)）。Richard Delorme 氏の Edax と [okuhara/edax-reversi-AVX](https://github.com/okuhara/edax-reversi-AVX) をもとにしています。Boost は Boost Software License で、このリポジトリには含みません。
