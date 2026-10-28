# N2b: PyInstaller で NeatVideoRepro.app — REPORT

作業: 2026-10-28、worktree `.claude/worktrees/n2` (ブランチ `n2-app`、N2a の未コミットの変更の上)。仕様は `.claude/tasks/native_n2_app.md` の N2b。
コミットしていない。AGENTS.md・MIGRATION.md・`readonly_snapshot_before.sha256` は触っていない。repro・native のファイルは 1 つも変更していない
(N2a の変更はそのまま)。本家バイナリは読んでいない・動かしていない。`repro/out/` には書いていない。画面は出していない
(`open` を使わず、すべて `QT_QPA_PLATFORM=offscreen` で `Contents/MacOS/NeatVideoRepro` を直接起動)。

## 結果の要約 (観測)

- `packaging/build_app.sh` で `packaging/dist/NeatVideoRepro.app` (arm64、187 MB、343 ファイル) ができる。ビルド約 1 分。
- `packaging/test_app.sh <OUT> --full` の全項目 PASS (`log_test_app.txt`、0 failure):
  - 自己試験 (バンドル内 Python): dylib 経路 96/96 (24 秒、`STATS {'native': 106, 'fallback': 0}`)、`NV_NATIVE=0` 96/96 (166 秒、
    AFT は spawn の子プロセスで実行、「AFT cancellable (child process terminated)」も PASS)。開発環境の自己試験も同時に 96/96。
  - 書き出し: 自己試験が書く 3 本 (`export.mp4` = 4 フレーム CRF 0、`export_tnoise.mp4` = 2 フレーム、`export10.mkv` = 10 ビット FFV1 3 フレーム)
    が、.app (dylib ON / OFF とも) と開発環境 (dylib ON) で一致。mp4 2 本はファイルのバイト列が一致 (509,224 B・279,293 B、
    SHA-256 は `export_hashes.txt`)。mkv は復号したフレーム (framemd5) と長さが一致し、違うのは 44 バイトだけ。この 44 バイトは Matroska の
    SegmentUID・TrackUID と、それを含む要素の CRC-32。同じファイルを開発環境の ffmpeg で 2 回 remux しても、同じ 44 バイトのオフセット
    (221〜224, 265〜280, 300〜303, 319〜326, 614〜617, 661〜668) が違う (観測)。
  - dylib: `Contents/Frameworks/libnvcore.dylib` から読み込まれ、`nv_native.AVAILABLE = True`、C の呼び出し 106 回・Python への退避 0 回。
  - ref_floor: `sys._MEIPASS/data/ref_floor.npy` (= `Contents/Frameworks/data` → `Contents/Resources/data` へのリンク) を読む。
    読み込み 0.2 ms で、ファイルとビット一致 (計算し直すと約 7 秒)。spawn の子プロセスでも同じ場所・ビット一致・0.05 秒。
  - 同梱の版: Python 3.13.5・numpy 2.1.3・scipy 1.15.3・PySide6 6.12.0 (開発環境と同じ。照合の前提どおり)。
    `nv_curve` の libSystem の pow (ctypes) も凍結環境で読み込まれる。
  - 実行時に `/opt/anaconda3` のライブラリを読まない (`DYLD_PRINT_LIBRARIES=1` で `--info` と UI の起動を見て 0 件。
    同梱の全 .so/.dylib の `otool -L` に `/opt/anaconda3` は 0 件)。
  - バンドルの中に何も書かない (試験の前後で全ファイルの SHA-256 とリンク先が同じ)。
  - `codesign --verify --deep --strict`: valid on disk / satisfies its Designated Requirement。
  - ランチャの ffmpeg 探索の順序と「見つからない」ダイアログ: 8/8 (`test_launcher.py`、開発環境の Python で)。

## 構成

### `packaging/main.py` (ランチャ、PyInstaller の入口)

すべてのプロセスで次を行ってから `multiprocessing.freeze_support()` を呼ぶ (spawn の子もこのファイルを先頭から実行するので、子でも効く):

1. ffmpeg の探索。`ffmpeg` と `ffprobe` が両方ある最初のディレクトリを `PATH` の先頭に足し、`NV_FFMPEG_DIR` に記録する (子は記録を引き継ぐ)。
   探す順は次のとおり。
   1. `$NV_FFMPEG` (実行ファイルかディレクトリ)
   2. QSettings の `prefs/ffmpeg` (読むだけ)
   3. `/opt/homebrew/bin`
   4. `/usr/local/bin`
   5. `/opt/local/bin`
   6. `PATH`

   repro は "ffmpeg" / "ffprobe" を名前で呼ぶので、repro を触らずに済む。
2. `nv_profile.reference_floor` の既定引数をバンドル内の `data/ref_floor.npy` に向ける。凍結環境では repro のモジュールの `__file__` が
   `Contents/Frameworks/nv_profile.pyc` になるので、元の既定 (`<repro>/../data`) は `Contents/data` になり、存在しない。
   既定引数は定義時に評価されるので `__defaults__` を差し替える。

入口:
- 引数なし / `<video>`: UI (`nv_ui_qt.main` の standalone の 5 行と同じ。`QApplication.instance()` があればそれを使う)。
  ffmpeg が無ければ、窓を作る前に `QMessageBox.critical` を出し、動画の引数は捨てる (開くと ffprobe で落ちるため)。
  CLI の動画パスは絶対パスにする (下記)。
- `--ofx ...`: `nv_ui_qt.main.main` をそのまま呼ぶ。
- `-m nv_ui_qt.export ...`: `export.main`。`worker.ExportProcess` は `sys.executable -m nv_ui_qt.export` を起動するが、凍結環境の
  `sys.executable` はこのランチャで `-m` を解釈しないので、ここで振り分ける。
- `--selftest VIDEO OUTDIR`: `nv_ui_qt.selftest.main` (既定 offscreen)。最後に `nv_native.STATS` を出す。書き出しが失敗したときは、
  UI がステータスバーにしか出さないエラー文を標準出力にも出す (`MainWindow._export_done` をこのプロセスでだけ包む)。
- `--info`: 診断の JSON。版、dylib の場所、ref_floor (場所・時間・ビット一致)、libSystem の pow、ffmpeg、spawn の子での同じ値。
- `NV_LAUNCH_TIMING=<epoch 秒>`: 起動時間の測定用。窓を出した後の最初のイベントループの回転で経過時間を出して終了する。
- 開発環境で `python3 packaging/main.py ...` としても動く (repro と aft_port を sys.path に足す)。

### `packaging/nvrepro.spec`

- onedir の `.app` (`target_arch="arm64"`, `console=False`, `bundle_identifier="local.nvrepro.NeatVideoRepro"`, `LSMinimumSystemVersion` 26.0)。
- `pathex`: `repro/` と `neatvideo_ofx_614/analysis/noise/aft_port/src`。
  `nv_aft_v5` は aft_port の `nv_aft` を sys.path 経由で import するので、後者は必須。
- `hiddenimports`: repro の `*.py` から次を除いたもの、`nv_aft`、`nv_ui_qt` の全サブモジュール、`PySide6.QtTest` (自己試験が使う)。
  除いたもの: tkinter の旧 UI `nv_ui`・`denoise_video`・`denoise_video_v2`・`neat_repro`・`validate_v2/v4/v5`。
- `binaries`: `native/libnvcore.dylib` → `Contents/Frameworks/` (= `sys._MEIPASS`。`nv_native._candidates` の最初の候補)。
  `datas`: `data/ref_floor.npy` → `data/`。
- `excludes`: anaconda に入っていて、任意の import から辿られるだけのパッケージ。
  - 経路の例: scipy の array-API 補助が dask・jax・torch・cupy を名指しする。dask から distributed・bokeh・panel・pyarrow…と辿る。
  - 経路の例: `scipy._lib._docscrape` が sphinx を試す。
  - 経路の例: `numpy.__config__` が yaml を、`scipy._lib._testutils` が psutil を試す。

  除かないと 796 MB だった。経路は `build/nvrepro/xref-nvrepro.html` を BFS して確認した。
- Qt のプラグイン `libqtvirtualkeyboardplugin` (QtQuick・QtQml を引き込む) と `libqpdf` (QtPdf) を除外 (261 MB → 187 MB)。
  残りのバイナリでこれらのフレームワークを参照するものがないことを `otool -L` で確認した。
  載っているプラグインは次のとおり。
  - platforms: cocoa・offscreen・minimal
  - styles: macstyle
  - imageformats: 11 種
  - iconengines: svg
  - tls / networkinformation / generic

### `packaging/build_app.sh` と `packaging/pyi_build.py`

- `packaging/.venv` は `python3 -m venv --system-site-packages`。中に入れたのは PyInstaller 6.22.3 (altgraph・macholib・pyinstaller-hooks-contrib 2026.9 が付いてくる) だけ。
  ベースの anaconda には何も入れていない (`/opt/anaconda3/bin/python3 -m pip list` に pyinstaller なし)。
- PyInstaller はベースの python3 を `PYTHONPATH=<venv の site-packages>` 付きで動かす。venv 自体の python で動かすと、
  conda の上の venv では `sys.prefix` が venv になり、PyInstaller の numpy フックが `conda_support` を import できずに落ちた
  (`ImportError: cannot import name 'conda_support'`)。
- `pyi_build.py`: anaconda のベースに旧い `enum34` のメタデータ (`enum34-1.1.10.dist-info`) があり、PyInstaller はこれを見つけると止まる。
  標準ライブラリの `enum.py` が優先されている (= site-packages の enum は import も同梱もされない) ときだけ、その検査を通す。
  ベース環境から enum34 を消す案は、ベースを変えない規約のため取らなかった。
- 最後にアドホック署名 (`codesign --force --deep -s -`) と `codesign --verify --deep --strict`。

### `packaging/test_app.sh` と `packaging/test_launcher.py`

README.md の「確認」の節のとおり。`test_launcher.py` は偽の ffmpeg/ffprobe (実行しないシェルスクリプト) を OUTDIR に作って、探索の順と
ダイアログの文面を開発環境の Python で確かめる (タイマーでダイアログを閉じる。offscreen)。

## 測定

- サイズ (`du -sh`): 187 MB。内訳:
  - `Contents/Frameworks` 170 MB: PySide6 53 MB (Qt のフレームワーク 8 個)、scipy 36 MB、libopenblas 21 MB × 2 (下記)、numpy 6.7 MB、libpython 5.8 MB
  - `Resources` 8.4 MB
  - `MacOS` 8.7 MB (PYZ を含む実行ファイル)

  conda の numpy フックが numpy の依存の動的ライブラリ (ncurses・readline・libgfortran など) も集めている。
- 起動時間 (offscreen、負荷平均 3.4〜4.2)。シェルで起動直前の時刻を渡し、窓を構築・表示した後の最初のイベントループの回転までを測った (`NV_LAUNCH_TIMING`)。

  | 条件 | 窓まで | プロセスの起動から終了まで |
  |---|---|---|
  | .app、動画なし (3 回) | 1.00 / 1.05 / 1.08 秒 | 1.04 / 1.09 / 1.12 秒 |
  | .app、`test.mp4` を渡したとき (2 回) | 1.13 / 1.17 秒 | (動画の読み込みはその後に worker で進む) |
  | 開発環境の `python3 packaging/main.py`、動画なし (2 回) | 0.77 / 0.79 秒 | – |

  いずれもキャッシュの温まった状態。ビルド直後・初回の起動 (Gatekeeper の検査を含む) は測っていない。
  Finder からの起動は offscreen でなく cocoa なので、値は違い得る。
- 自己試験の時間: .app の dylib 経路 24 秒、`NV_NATIVE=0` 166 秒 (N2a の開発環境の値 23 秒・165 秒とほぼ同じ)。

## 凍結環境で見つけて直した問題 (ランチャ側で対処。repro は無変更)

1. 書き出しプロセス: `sys.executable -m nv_ui_qt.export` は凍結環境では `-m` が効かない。ランチャで振り分けた。
2. 相対パスの動画: 書き出しプロセスは作業ディレクトリを `main_window.REPRO` で起動する。凍結環境ではこれが `Contents/Frameworks` なので、
   相対パスで渡した動画を ffprobe が開けなかった。最初の自己試験の失敗はこれで、`[export failed] ... CalledProcessError: ffprobe ... '../../../test.mp4'`。
   ランチャで CLI の動画・出力先のパスを絶対パスにした。UI のファイルダイアログは絶対パスを返すので、通常の操作では起きない。
   開発環境でも、repro/ 以外から相対パスで起動すれば同じことが起きるはず (推測。未確認)。repro の既存の挙動なので触っていない。
3. ref_floor の場所 (上記)。
4. ダイアログの題: macOS の `QMessageBox` は題を表示しない (offscreen でも `windowTitle()` が空だった)。本文の先頭に見出しの文を入れた。

## ユーザーが手で行う確認 (未実施)

1. Finder で `packaging/dist/NeatVideoRepro.app` をダブルクリックする。ローカルでビルドしたものには quarantine が付かないので、そのまま開くはず。
   別のマシンにコピーしたものは、右クリック → 開く (macOS 15 以降は、システム設定 → プライバシーとセキュリティ → このまま開く)。
2. (a) 動画を開く: Effect > Open Input Video… (⌘O) で `test.mp4` を選ぶ。読み込みの進捗が出て、サムネイルが並ぶこと。
3. (b) Auto Profile: Prepare Noise Profile ページで Auto Profile。領域の枠と Quality が出ること。Advanced Mode の Auto Fine-Tune も 1 秒前後で終わること (dylib 経路)。
4. (c) プレビュー: Adjust and Preview ページで Filtered の表示が出ること。ビューアを押している間は元画像になること。
5. (d) 書き出し: 範囲を選んで (または全編で) Apply し、保存先を選ぶ。Preferences の CRF を 0 にして 4 フレーム書けば、
   開発環境 (`python3 -m nv_ui_qt.main`) の同じ操作の出力とバイト比較できる。進捗バーが進み、完了の表示が出ること。
6. Preferences の「Use the native core (libnvcore.dylib; same results, faster)」が有効でチェックされていること。
7. ffmpeg が無い場合のダイアログは、このマシンでは `/opt/homebrew/bin` が既定の候補にあるので出せない。表示は `test_launcher.py` で確かめた。
8. 設定は `~/.config/NeatRepro/nv_ui_qt.ini` (開発環境の UI と共用) に書かれる。

## 仕様との相違点と理由

1. ffmpeg の探索の先頭の「Preferences の設定」: 既存の Preferences に ffmpeg の欄はない。欄を足すと main_window の変更になる。
   代わりに環境変数 `NV_FFMPEG` と、設定ファイルの `prefs/ffmpeg` キー (手で書く。ランチャは読むだけ) を先頭にした。
   ダイアログは既存の UI のエラー表示ではなく、ランチャの `QMessageBox.critical`。既存の UI には ffmpeg が無い場合の表示がなく、
   `probe` の FileNotFoundError は Qt のスロットで握りつぶされる。
2. 同梱は「repro/*.py (全モジュール)」から、凍結アプリから起動できない旧スクリプト 7 本を除いた。
   除いたのは tkinter UI の `nv_ui.py` と、v1〜v4・検証用の `denoise_video.py`・`denoise_video_v2.py`・`neat_repro.py`・`validate_v2/v4/v5.py`。
   入れると tkinter・PIL が付いてくる。
3. 仕様の検証 3 の「`open` での起動」はしていない (画面を奪わない規約)。上の手順をユーザーに任せる。
4. `.gitignore` に `packaging/` の許可リストを足した。対象は `*.py`・`*.spec`・`*.sh`・`README.md` で、`.venv/`・`build/`・`dist/` は除外。
   ランチャ以外の `*.py` (`pyi_build.py`・`test_launcher.py`) もあるので、`main.py` 単体ではなく `*.py` を許可した。
5. `LSMinimumSystemVersion` は 26.0。同梱の PySide6 6.12 の拡張モジュール・Qt と libnvcore.dylib の `minos` が 26.0 だから
   (`otool -l`。libpython・numpy・scipy は 11.1、Qt のフレームワークは 14.4)。

## 変更・新規ファイル

- 新規:
  - `packaging/main.py`
  - `packaging/nvrepro.spec`
  - `packaging/build_app.sh`
  - `packaging/pyi_build.py`
  - `packaging/test_app.sh`
  - `packaging/test_launcher.py`
  - `packaging/README.md`
- 変更: `.gitignore` (packaging の許可リスト 7 行)
- 作業記録:
  - `sandbox/omo/wp_native_app/` の REPORT.md・`log_build.txt`・`log_test_app.txt`・`export_hashes.txt`
  - `test_app_out/`: ログ・`info.json`・バンドルのハッシュ一覧。書き出しの動画と自己試験の出力は消した
- 追跡外のビルド物: `packaging/.venv/`・`packaging/build/`・`packaging/dist/NeatVideoRepro.app`
- repro・native・処理コア 3 ファイル・本家バイナリ・`repro/out/` には触れていない。
  `find repro native -newer packaging/.venv/pyvenv.cfg` で 0 件 = 作業開始後に更新されたファイルなし。

## 未解決事項・注意

- Finder からの起動 (cocoa)、実際の UI 操作、別のマシン (Python なし) での起動は未確認。
  Python の無い環境の代わりに、次の 2 点を見た。
  - `DYLD_PRINT_LIBRARIES` で anaconda のライブラリを読まないこと
  - repro のモジュールがバンドル内から読まれること (`--info` の `repro_file`)
- ビット一致は同梱の numpy/scipy と、実行する macOS の libSystem (libm) に依存する。照合したのは macOS 26.6.1 (25G76) だけ。
- libopenblas が 2 つの実体 (`libopenblas.dylib` と `libopenblasp-r0.3.29.dylib`、同一内容 21 MB) で入っている。
  conda の numpy フックの集め方による。片方をリンクにすれば 21 MB 減るが、試していない。
- PyInstaller の起動に 2 つの回避策 (`pyi_build.py` の enum34、ベース python + PYTHONPATH) が要る。
  原因はこの anaconda の環境で、enum34 が入っていない環境ならどちらも不要。
- 起動時に Qt が `Populating font family aliases took ~190 ms` と警告する ("Sans Serif" の代替探し、開発環境でも同じ)。
- N2a の未解決事項はそのまま。
