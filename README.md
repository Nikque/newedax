# newedax

The Othello engine [Edax 4.5.5 nikque](https://github.com/Nikque/edax-reversi-AVX) (its eval2 line) ported to C++23, with a new evaluation function since v4.5.5-newedax.13: 64 feature slots (and a 48-slot build), trained again. At the same time it is about 60 Elo stronger than the eval2 evaluation; it uses less memory for books than the C version.

[日本語](#日本語) · [Download](https://github.com/Nikque/newedax/releases/latest) · [Manual](manual/en/README.md) · [Release notes](RELEASE-NOTES.md) · [Tests and measurements](docs/validation/newedax.en.md)

## Download and run

1. Download a ZIP from the [latest release](https://github.com/Nikque/newedax/releases/latest) and unpack it: `edax-…-64.zip` (the default, 64 slots) or `edax-…-48.zip` (48 slots: faster per node, weaker at the same time). It contains executables for Windows (x64, x86) and Android, the libedax DLLs, the training tools, and the `eval.dat` of its build.
2. Open the `bin` folder and start the executable from **inside that folder**. On Windows, start with `wEdax-x86-64.exe`; use `-v3` (AVX2) or `-v4` (AVX-512) if your CPU supports it.

The `bin/` folder of the repository holds only the data (`book.dat`, `config.ini`, and the `eval.dat` of eval2, which is not for the builds since v4.5.5-newedax.13: they read it with a warning, but their evaluation is then wrong; use the `eval.dat` of a release ZIP). Commands and settings are the same as the C version: see the [manual](manual/en/README.md).

For Linux and macOS executables, use the C version ([edax-reversi-AVX releases](https://github.com/Nikque/edax-reversi-AVX/releases)).

## Relation to the C version

- Evaluation: since v4.5.5-newedax.13, a new evaluation of 64 slots (or 48), with its own `eval.dat`; the default probcut model is `refit`. Up to v4.5.5-newedax.12 it was eval2, with the same results as the C version.
- Commands and settings are the same as the C version; the search results differ (the evaluation).
- Each newedax release contains all changes of a C release. The latest, **v4.5.5-newedax.13**, contains C v4.5.5-nikque.27 / v4.5.5-eval2.15.

## Differences from the C version

- **Memory**: a book position takes 40 bytes (48 in the C version). Loading a book of 6.5 million positions takes 11 to 45 % less memory.
- **Strength** (v4.5.5-newedax.13, 64 slots, 30 s a game a side): +60.2 Elo against eval2 (v4.5.5-newedax.12); +23.2 Elo against Egaroucid (tournament branch, 120 s a game, 8 threads). See the [release notes](RELEASE-NOTES.md).
- **Speed** (eval2, up to v4.5.5-newedax.12; Windows, against the C release eval2.6): `wEdax-x86-64-v4.exe` is slightly faster; the x64 standard and x86 builds are as fast or faster; **`wEdax-x86-64-v3.exe` is about 1 % slower**.
- **New file format (form 2)** for books (`NEDXBOOK`, about 33 % smaller) and evaluation files (`NEDXEVAL`), with a checksum. Convert with `-book-convert` / `-eval-convert` (both ways). Form 1 files (C version) are still read.
- **Build options**: `EVAL_SLOTS` 64 (default) or 48; `EVAL_MOBILITY` (default 1).
- **[tools/eval](tools/eval)**: the training tools in C++ (`evsearch`, `evtrain`, `w2dat2`, `selfplay3`, ...).

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

オセロのエンジン [Edax 4.5.5 nikque](https://github.com/Nikque/edax-reversi-AVX)（eval2 の系列）を C++23 に移したものです。v4.5.5-newedax.13 から、評価関数を 64 枠（と 48 枠の版）で学習し直した新しいものにしました。同じ持ち時間で eval2 の評価関数より約 60 Elo 強く、book のメモリは C 版より少なくて済みます。

[ダウンロード](https://github.com/Nikque/newedax/releases/latest) · [マニュアル](manual/ja/README.md) · [修正一覧](RELEASE-NOTES.ja.md) · [テストと実測の記録](docs/validation/newedax.ja.md)

### 使い始め方

1. [最新の Release](https://github.com/Nikque/newedax/releases/latest) の ZIP をダウンロードして展開します。`edax-…-64.zip`（既定の 64 枠）か `edax-…-48.zip`（48 枠：1 節点あたりは速いが、同じ時間では弱い）です。Windows（x64・x86）と Android の実行ファイル、libedax の DLL、学習ツール、その版の `eval.dat` が入っています。
2. `bin` フォルダーを開き、**そのフォルダーの中から**実行ファイルを起動します。Windows なら、まず `wEdax-x86-64.exe` を使ってください。CPU が対応していれば `-v3`（AVX2）・`-v4`（AVX-512）が速くなります。

リポジトリの `bin/` にはデータ（`book.dat`、`config.ini`、eval2 の `eval.dat`）だけがあります。eval2 の `eval.dat` は v4.5.5-newedax.13 からの版には使えません（警告を出して読みますが、評価が正しくなりません）。Release の ZIP の `eval.dat` を使ってください。コマンドと設定は C 版と同じです（[マニュアル](manual/ja/README.md)）。

Linux・macOS 用の実行ファイルは C 版（[edax-reversi-AVX の Releases](https://github.com/Nikque/edax-reversi-AVX/releases)）を使ってください。

### C 版との関係

- 評価関数：v4.5.5-newedax.13 から、64 枠（または 48 枠）の新しい評価関数で、専用の `eval.dat` を使います。ProbCut の既定は `refit` です。v4.5.5-newedax.12 までは eval2 で、C 版と同じ結果でした。
- コマンドと設定は C 版と同じです。探索の結果は評価関数が違うので変わります。
- newedax の各版は、C 版のある版の変更をすべて含みます。最新の **v4.5.5-newedax.13** は、C 版 v4.5.5-nikque.27・v4.5.5-eval2.15 を含みます。

### C 版との違い

- **メモリ**：book の 1 局面が 40 バイト（C 版は 48 バイト）。649 万局面の book の読み込みで、メモリが 11〜45% 少なくなります。
- **強さ**（v4.5.5-newedax.13、64 枠、1 局 30 秒ずつ）：eval2（v4.5.5-newedax.12）に +60.2 Elo、Egaroucid（大会用の枝、1 局 120 秒、8 スレッド）に +23.2 Elo。[修正一覧](RELEASE-NOTES.ja.md)を見てください。
- **速さ**（eval2、v4.5.5-newedax.12 まで。Windows、C 版 eval2.6 の配布物と比較）：`wEdax-x86-64-v4.exe` はわずかに速く、x64 標準版・x86 版は同じか速い。**`wEdax-x86-64-v3.exe` は約 1% 遅い**です。
- **新しいファイル形式（形式 2）**：book（`NEDXBOOK`、約 33% 小さい）と評価ファイル（`NEDXEVAL`）。チェックサム付き。`-book-convert`・`-eval-convert` で相互に変換できます。C 版の形式 1 のファイルもそのまま読めます。
- **ビルドオプション**：`EVAL_SLOTS` 64（既定）か 48、`EVAL_MOBILITY`（既定 1）。
- **[tools/eval](tools/eval)**：学習ツールの C++ 版（`evsearch`・`evtrain`・`w2dat2`・`selfplay3` など）。

### まだ確認していないこと

Windows の MSVC 以外でのビルド（Android はリンクまで、実機では未実行）、AddressSanitizer・ThreadSanitizer での試験、数十億局面の実際の book、別の人によるレビュー。

### ビルド

上の英語の「Build」を見てください。移植の作業記録は [docs/port](docs/port) にあります。

### ライセンス

GPL-3.0（[LICENSE](LICENSE)）。Richard Delorme 氏の Edax と [okuhara/edax-reversi-AVX](https://github.com/okuhara/edax-reversi-AVX) をもとにしています。Boost は Boost Software License で、このリポジトリには含みません。
