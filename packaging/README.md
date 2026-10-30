# MeatVideo.app の梱包 (N2b)

repro の Qt UI (`python3 -m nv_ui_qt.main`) を PyInstaller でスタンドアロンの `.app` にする。
Python・numpy・scipy・PySide6 と N2a の `native/libnvcore.dylib` を同梱するので、配布先に Python は要らない。
ffmpeg は同梱しない (GPL)。仕様は `.claude/tasks/native_n2_app.md` の N2b、作業記録は `work/omo/wp_native_app/REPORT.md`。

## 必要な環境

- ビルド: Apple Silicon (arm64) の Mac、Xcode Command Line Tools (clang・make・codesign)。
  Python 3.13 と numpy 2.1.3・scipy 1.15.3・PySide6 6.12・PyWavelets が入った python3 (開発環境の anaconda)。
  この版は Python 経路と C++ 経路のビット一致の照合 (`native/MIGRATION.md`) の前提で、同梱される版も同じになる
  (`build_app.sh` が違えば警告する)。
- 実行: macOS 26 以降の arm64 (同梱の PySide6 6.12 の wheel と libnvcore.dylib の最小 OS が 26.0)。
  ffmpeg と ffprobe (8.x で確認)。

## ビルド

```sh
packaging/build_app.sh          # → packaging/dist/MeatVideo.app (約 190 MB、約 1 分)
```

- 初回は `packaging/.venv` (`--system-site-packages`) を作り、PyInstaller 6.22.3 だけを入れる。ベース環境には何も入れない。
- PyInstaller はベースの python3 に `PYTHONPATH=<venv の site-packages>` を付けて動かす (conda の上の venv では
  PyInstaller の numpy フックが conda の情報を使えないため)。`pyi_build.py` は、anaconda に残っている旧い `enum34` の
  メタデータで PyInstaller が止まるのを、標準ライブラリの enum が優先されている場合だけ通す。
- `make -C native lib` で dylib を作ってから梱包し、最後に `codesign --force --deep -s -` (アドホック署名) と
  `codesign --verify --deep --strict` をする。
- 中身: `nvrepro.spec`。repro の全モジュール (tkinter の旧 UI `nv_ui.py` と v1〜v4・検証用スクリプトを除く) と AFT の移植
  (`reference/genuine/analysis/noise/aft_port/src/nv_aft.py`)、`data/ref_floor.npy`、`libnvcore.dylib` (Contents/Frameworks)。
  anaconda に入っている未使用のパッケージ (scipy の任意の import から辿られる dask・jax・torch・matplotlib・sphinx など) と、
  QtQuick・QtPdf を引き込む Qt のプラグイン (仮想キーボード・PDF 画像) は除外している。

## 確認

```sh
packaging/test_app.sh [OUTDIR] [--full]     # 既定 OUTDIR = packaging/build/test_app
```

画面は出さない (`QT_QPA_PLATFORM=offscreen` でバンドルの実行ファイルを直接起動する)。確認すること:
署名の検証、`--info` (同梱の numpy/scipy の版、dylib を Contents/Frameworks から読んだこと、ref_floor.npy を
計算し直さずに読むこと、spawn の子プロセスでも同じこと)、起動時間 3 回、バンドル内 Python での自己試験
(`nv_ui_qt.selftest`、dylib 経路。`--full` では `NV_NATIVE=0` でも)、書き出しが開発環境 (dylib 経路) の書き出しと一致すること
(mp4 はバイト単位、mkv は Matroska のランダムな UID があるので復号したフレームと長さ)、ランチャの ffmpeg 探索、
試験の前後でバンドルの中身が変わらないこと。開発環境の python3 (`PYTHON=`) と `test.mp4` (`VIDEO=`) を使う。

## 実行ファイルの入口 (`main.py`)

| 引数 | 動作 |
|---|---|
| (なし) / `<video>` | UI (`nv_ui_qt.main` の standalone と同じ) |
| `--ofx W H ...` | OFX ホストモード (`nv_ui_qt.main` のまま) |
| `-m nv_ui_qt.export ...` | UI が起動する書き出しプロセス (`worker.ExportProcess` は `sys.executable -m nv_ui_qt.export` を実行する) |
| `--selftest VIDEO OUTDIR` | 自己試験 (既定 offscreen)。最後に `nv_native.STATS` を出す |
| `--info` | 診断 (JSON) |

すべてのプロセス (multiprocessing の spawn の子を含む) で、先頭で ffmpeg の場所を `PATH` の先頭に足し、
`nv_profile.reference_floor` の既定の場所をバンドル内の `data/ref_floor.npy` に向けてから `multiprocessing.freeze_support()` を呼ぶ。
repro のファイルは変更していない。

## ffmpeg

`ffmpeg` と `ffprobe` が両方ある最初のディレクトリを使う。順: 環境変数 `NV_FFMPEG` (実行ファイルかそのディレクトリ) →
設定ファイル `~/.config/NeatRepro/nv_ui_qt.ini` の `[prefs]` の `ffmpeg=<path>` (UI に欄はない。手で書く) →
`/opt/homebrew/bin` → `/usr/local/bin` → `/opt/local/bin` → `PATH`。Finder から起動したアプリの `PATH` は
`/usr/bin:/bin:/usr/sbin:/sbin` だけなので、Homebrew の ffmpeg は既定の候補で見つける。見つからなければ起動時にダイアログを出す
(動画は開けない)。

## 実行時に書くもの

設定は `~/.config/NeatRepro/nv_ui_qt.ini` (開発環境の UI と同じファイル)。一時ファイルはシステムの一時ディレクトリ。
書き出しは選んだ場所とその横の JSON。バンドルの中には書かない (`test_app.sh` で確認)。

## 配布するとき

- アドホック署名だけで、Developer ID の署名も公証 (notarization) もしていない。ダウンロードした (quarantine 属性の付いた) `.app`
  は Gatekeeper に止められる。初回は Finder で右クリック → 開く。macOS 15 以降でこれが効かないときは、一度開こうとした後に
  システム設定 → プライバシーとセキュリティ → 「このまま開く」。ターミナルなら `xattr -dr com.apple.quarantine MeatVideo.app`。
- 配布するなら zip にする (`ditto -c -k --keepParent MeatVideo.app MeatVideo.zip`)。シンボリックリンクと署名が保たれる。
- 配布先には ffmpeg / ffprobe を別に入れてもらう (`brew install ffmpeg`)。
- 結果のビット一致の照合は、このビルド環境と同じ版の numpy/scipy (同梱) と、実行する macOS の libSystem の libm
  (atan2・sincos・pow・tanf などを Python も C++ も同じものを呼ぶ) に依存する。macOS 26.6.1 以外の OS 版での照合はしていない。
