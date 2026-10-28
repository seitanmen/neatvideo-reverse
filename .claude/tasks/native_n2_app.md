# ネイティブ移行 N2: ctypes 速度ブリッジ + スタンドアロン .app 梱包

作成: 2026-10-28 (統括)。N1 は完了 (native/ の 9 部品が Python とビット一致、
`make -C native verify` 11 本全件一致)。N2 は 2 段階:

- **N2a**: N1 の C++ 部品を共有ライブラリ (dylib) にして ctypes から呼ぶ速度ブリッジ。
  出力は **Python 経路とビット一致**が必須 (これが価値の根拠)。
- **N2b**: PyInstaller で repro UI をスタンドアロン .app 化 (Python 本体がなくても動く)。

順序は N2a → N2b (N2b は N2a の dylib を同梱して作る)。別セッションで実施すること。

## 共通規約

- 処理コア (`repro/nvfilter.py` / `repro/nv_aft_v5.py` / `repro/nv_depth.py`) は変更禁止
  (読み込みのみ)。`repro/nv_ui_qt/backend.py` と新規ファイル (`repro/nv_native.py` など) は
  変更してよいが、backend の既存の公開関数のシグネチャと戻り値は変えない
  (main_window が多数依存している)。
- 合格判定はビット一致 (誤差許容なし)。検証は `native/verify_*.py` / `native/verify_bridge.py` 形式。
- fast-math 禁止・`-ffp-contract=off`・定数はビットパターン (MIGRATION.md の知見を最初に読むこと)。
- 本家バイナリは読まない。
- **コミットしない**。REPORT は `sandbox/omo/wp_native_bridge/REPORT.md` (N2a) と
  `sandbox/omo/wp_native_app/REPORT.md` (N2b)。スナップショット (`readonly_snapshot_before.sha256`)
  は自分で更新せず、変更ファイルの一覧を REPORT に書く (統括が更新する)。
- 速度の計測 (1080p test_v02 n=312 付近、1 スレッド) も REPORT に残す。

---

## N2a: ctypes 速度ブリッジ (`native/libnvcore.dylib` + `repro/nv_native.py`)

### 目標

UI の処理 (プロファイル構築・AFT・フレーム処理) を、dylib 経由に差し替えても
**1 ビットも違わない**こと。速度目標 (N1 の実測から): プロファイル 128² 17→約 1.3 秒、
AFT 1080p 22〜37→約 0.5〜0.8 秒、process_frame_v5 1080p 2.1→約 0.55 秒。

### 構成

1. `native/nvcore_c.cpp` (+ `native/nvcore_c.h`): C ABI の薄いラッパ。公開関数の候補
   (必要に応じて増減してよいが、構造体は渡さずフラットなバッファ + バイト配列にする):
   - `nv_profile_build(rgb_f32, w, h, mode, area4, out_blob /*0xc300*/, out_status) -> ok`
     (repro `nv_profile.build_profile` 相当。reference_floor はホストから渡すか
     `data/ref_floor.npy` 相当の 51 語を受け取る)
   - `nv_aft_run(rgb_f32, w, h, mode, blob /*in/out 0xc300*/, out_flags /*27×3 ビン旗*/) -> ok`
     (aft_port の `auto_fine_tune` 相当。apply_aft が読むのはこの 3 種)
   - `nv_curve_noise(rgb_f32, w, h, out_xs8, out_ys8) -> ok` (genuine_noise_curve + noise_at)
   - `nv_flat_area(rgb_f32, w, h, mode, curve_fn or 曲線表, out_area5, out_status) -> ok`
   - `nv_frame_process(y_cb_cr_f32..., prof_blob_or_v5dict, params...) -> ok`
     (process_frame_v5 相当。v5 の prof dict は Python が保持するので、必要な語だけを
     平坦に渡す設計にする。パラメータは **すべて float32 化してから渡す**
     — numpy の float64 を混ぜると Python 側だけ倍精度になり食い違う (N1g/h の知見))
   - スレッド数指定 (`-j` 相当) を引数に含める (結果はスレッド数に依存しないことを検証済み)。
2. Makefile に `libnvcore.dylib` ターゲット (`-dynamiclib`、arm64 と universal のどちらでもよいが
   N2b で同梱するのは arm64)。`make portable` と同様に `-DNV_PORTABLE` 版も作れるようにする。
3. `repro/nv_native.py` (新規): ctypes ラッパ。dylib が見つからないときは
   `AVAILABLE = False` を返し、呼び出し側が Python 経路にフォールバックできるようにする。
   numpy 配列 → `ctypes` バッファの変換はコピー最小で (`np.ascontiguousarray(..., np.float32)` の
   ポインタを渡す)。
4. `repro/nv_ui_qt/backend.py` への統合: **Preferences のフラグ** (既定 ON、dylib がなければ自動 OFF)
   で `build_profile` / `run_aft` / `engine.process` を dylib 経由に切り替える。
   既存の Python 経路を壊さない (フラグ OFF で従来どおり)。
   - `engine.process` は窓 (複数フレーム) を取るので、dylib 側に窓渡し版を追加するか、
     Python で窓を作ってから 1 フレーム処理を呼ぶかは実装者の判断でよい (REPORT に記す)。
5. **検証** (`native/verify_bridge.py`):
   - 端から端: 実写 3 フレーム (test_v02 n=310..316) について、
     (a) Python 経路の `build_profile_v5` + `apply_aft` + `process_frame_v5` の出力と、
     (b) dylib 経路の出力 (プロファイル blob・曲線・AFT 済み曲線・処理済み 3 面) を
     **全語ビット比較**。パラメータは既定 + 変則 (noise_level ±、threshold 変更) の 2 種以上。
   - N1 の verify スイートが通ること (libnvcore は N1 のコードをそのまま使うので回帰なし)。
   - UI の自己試験: `QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest ../test.mp4 /tmp/...`
     を dylib ON/OFF の両方で 96/96 (または現行の全項目) PASS。
   - 書き出しビット一致: dylib ON で 4 フレーム書き出し → OFF の出力とバイト一致
     (既存の自己試験の export チェックと同じ方法)。

### 注意

- backend の `run_in_child` (AFT の spawn) は dylib 経路では子プロセス不要になる
  (高速化の効果の 1 つ)。ただしキャンセル挙動 (task.cancelled) は維持すること。
- dylib のロードパス: 開発時は `native/libnvcore.dylib`、.app では `Contents/Frameworks/`。
  `repro/nv_native.py` は両方を探す。
- GIL: 重い呼び出しの前に解放してよい (ctypes は既定で解放する)。UI が固まらないことを確認。

---

## N2b: PyInstaller で .app 化 (`packaging/`)

### 目標

Python をインストールしていない macOS (arm64) で動く `NeatVideoRepro.app`。

### 構成

1. `packaging/nvrepro.spec` (PyInstaller spec) と `packaging/build_app.sh`:
   - エントリ: `python3 -m nv_ui_qt.main` 相当の薄いランチャ (`packaging/main.py`)。
     **先頭で `multiprocessing.freeze_support()` を呼ぶ** (spawn を使う箇所があるため必須)。
   - 同梱: `repro/*.py` (全モジュール)、`repro/nv_ui_qt/`、`data/ref_floor.npy`、
     `native/libnvcore.dylib`、PySide6・numpy・scipy。
   - **同梱しない**: ffmpeg (GPL なので)。実行時に探す (優先順: Preferences の設定 →
     `/opt/homebrew/bin/ffmpeg` → `/usr/local/bin/ffmpeg` → PATH)。
     見つからないときは分かりやすいダイアログ (既存のエラー表示を使う)。
2. **凍結 (frozen) 環境の落とし穴を潰す** (実動作確認つき):
   - `denoise_video_v5` はモジュールとして import 済み (`export.py` が `import denoise_video_v5`)。
     子プロセスは spawn で `multiprocessing` を使っているので、spec の `multiprocessing`
     フックと freeze_support で動くことを確認する。
   - `sys.path` の操作 (`nv_ui_qt/__init__` が repro/ を追加する) が凍結環境で壊れないこと
     (`sys._MEIPASS` 配下に repro/ を置く)。
   - PySide6 のプラグイン (platforms/styles/imageformats) が spec のカレントディレクトリに載ること。
   - `data/ref_floor.npy` の探索を `sys._MEIPASS` 対応に (repro 側の修正が必要なら
     新しい小モジュールか backend の定数で)
   - 作業用ディレクトリ・設定 (`QSettings`) が通常どおりユーザ領域に書かれること。
3. **検証** (`packaging/test_app.sh` + REPORT):
   - .app を起動 (Finder ダブルクリック相当: `open`) して、(a) 動画を開く、(b) Auto Profile、
     (c) プレビュー、(d) 4 フレーム書き出し、ができること。UI 操作は統括かユーザーが行うので、
     ワーカーは CLI からの起動確認 (`.../MacOS/NeatVideoRepro ../test.mp4`) と
     **バンドル内 Python での自己試験** (dylib 経路で 96/96) までを自動化し、残りを REPORT に記す。
   - 書き出しのバイト一致: .app の書き出し == 開発環境 (dylib ON) の書き出し。
   - `codesign --force --deep -s -` のアドホック署名を入れる (Gatekeeper のローカル起動用)。
   - サイズと起動時間を REPORT に記す (PySide6 込みで 300〜500 MB 見込み)。
4. README (`packaging/README.md`): ビルド手順・必要環境 (PyInstaller・arm64)・ffmpeg の要件・
   配布時の注意 (未署名なので初回は右クリック→開く)。

### 注意

- PyInstaller の版は最新安定でよいが、**arm64 のインストール環境で作る** (universal は不要)。
- .gitignore: `packaging/build/`・`packaging/dist/` を除外 (`*.spec`・`build_app.sh`・
  `test_app.sh`・README は許可)。既存の `native/` の許可リストの書き方を踏襲する。
- スナップショット・AGENTS.md・MIGRATION.md は触らない (統括が更新)。

---

## 貼り付け用プロンプト

```
1. .claude/tasks/native_n2_app.md の「N2a」を実行してください (ctypes 速度ブリッジ)。
2. 同ファイルの「N2b」を実行してください (PyInstaller で .app 化)。
```

(1 と 2 は別セッション。各セッション冒頭で native/MIGRATION.md の「移植の重要な知見」も
読ませること。完了後、統括が検証・マージ・コミットを行う。)
