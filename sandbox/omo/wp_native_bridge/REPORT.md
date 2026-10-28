# N2a: ctypes 速度ブリッジ (libnvcore.dylib + repro/nv_native.py) — REPORT

作業: 2026-10-28、worktree `.claude/worktrees/n2` (ブランチ `n2-app`、HEAD 7a4f038)。仕様は `.claude/tasks/native_n2_app.md` の N2a。コミットはしていない。AGENTS.md・MIGRATION.md・`readonly_snapshot_before.sha256` は触っていない。

## 結果の要約 (観測)
- UI のプロファイル構築 (FindFlatArea の探索)・Auto Fine-Tune・フレーム処理 (時間段・空間段) を dylib 経由に切り替えても、Python 経路と全語ビット一致 (不一致 0)。
- UI 自己試験は dylib ON / OFF とも 96/96 PASS。ON では UI プロセス内で C の呼び出し 106 回・Python への退避 0 回。
- 書き出し (CRF 0・4 フレーム) は NV_NATIVE=1 と 0 でファイルのバイト列が一致 (test.mp4・test_v02)。
- `make -C native verify` の 11 本は変更後も全件一致。警告 0。
- 速度 (1080p test_v02 n=312、1 スレッド): AFT 46.8 → 2.05 秒 (4 スレッド 0.61 秒)、1 フレーム処理 2.03 → 0.57 秒、プロファイル構築 2.38 → 1.70 秒。

## C に渡す部分と Python に残す部分の境界

| backend の呼び出し | Python 経路 (真値) | dylib 経路 | Python に残すもの |
|---|---|---|---|
| `build_profile` (area なし) | `nv.build_profile_v5` → `nv.find_flat_area` | `nv_native.build_profile_v5` → `nvc_flat_area` (N1b 曲線 + N1d 探索) | RGB 変換、64px 未満・状態 1 の代替 (`find_flat_area_gradient`)、キャッシュ (nvfilter と同じキー・大きさの専用キャッシュ)、build_profile_v5 の残り全部 (見つけた領域を area で渡して呼び、`flat_status` を後で書く。area の切り詰めは find_flat_area の結果を変えない) |
| `build_profile` (area あり) | `nv.build_profile_v5` | 同じ (探索がないので C に渡すものがない) | 全部 |
| `run_aft` / `run_in_child(task, run_aft, …)` | `nv_aft_v5.apply_aft` | `nvc_profile_build` (N1f) → `nvc_aft` (N1i) → `nvc_aft_combine` | `ui_pre_aft`、RGB 変換、dict の組み立て (Python と同じ型・キー順) |
| `temporal` | `nv.temporal_v5` | `nvc_temporal` (N1g、窓渡し) | 窓の切り出し、`t_noise`、flows キャッシュ (C が推定した分を k の昇順で dict に入れる) |
| `spatial_ui` (一様 / 成分別) | `nv.spatial_v5` | `nvc_spatial` (N1h、成分別はマスクで 1 面だけ) | `ui_spatial` オフ、Mix |

- パラメータとプロファイルは平坦な int/double 配列で渡す。値は Python float をそのまま double で渡し、C 側が演算の直前に float32 にする (MIGRATION の「API の前提」)。
- C の経路を使わない入力 (`nv_native` が None を返し、backend が Python 関数を呼ぶ。結果は構成上同一):
  - スカラーに Python の int/float 以外が混ざるとき (np.float64 は numpy が昇格させるため。np.float32 も保守的に除外)
  - 未知の similarity、Y が bs より小さいとき、Cb と Cr の形が違うとき、rv が float32 でないとき、曲線の点数が 1〜16 の外のとき
  - AFT のモード 0/1 以外、領域が整数でないかフレーム外のとき、C 側の例外
  - 退避の件数は `nv_native.STATS` に数える。

## C ABI (`native/nvcore_c.h`)
- `nvc_abi_version`: 1。
- `nvc_noise_curve`: `nvc::genuine_noise_curve`。
- `nvc_flat_area`: `nvf::find_flat_area_genuine`。曲線は内部で計算し、取り消しの問い合わせを受ける。
- `nvc_profile_build`: `nvp::build_profile`。reference_floor は Python の `nv_profile.reference_floor()` を渡す。
- `nvc_aft`: `nva::auto_fine_tune`。取り消しの問い合わせを受ける。
- `nvc_aft_combine`: `nva::combine`。
- `nvc_temporal`: `nvt::temporal_v5`。flows の入出力つき。
- `nvc_spatial`: `nvs::spatial_v5_plane` を面マスクに従って呼ぶ。

共通の扱い:
- 構造体は渡さない。C++ 例外は ABI を越えない (負の値を返し、Python が退避する)。
- スレッド数は `nv_native.THREADS` (既定 min(4, CPU)、環境変数 `NV_NATIVE_THREADS`)。結果は -j 1 / 3 で同一を確認した。時間・空間の核は 1 スレッド。
- ctypes は呼び出し中に GIL を解放する。取り消しは CFUNCTYPE のコールバック。
- C++ 側の静的状態は mutex 付きキャッシュと thread_local だけ。4 スレッドからの同時呼び出しも一致した。
- ビルド: `make -C native lib` → `libnvcore.dylib` (arm64、`@rpath`)。`make -C native lib_portable` → `libnvcore_p.dylib` (-DNV_PORTABLE)。既存と同じフラグで警告 0。
- 探索順: `$NV_NATIVE_LIB` (設定時はそれだけ) → `sys._MEIPASS` → `<exe>/../Frameworks` → `repro/../native/`。ABI の版が違えば使わない。

## backend への統合
- 公開関数のシグネチャと戻り値は不変 (`run_aft` は `_run_aft` の薄い包み)。
- `set_native` / `native_enabled` / `native_available` を追加した。`set_native` は環境変数 `NV_NATIVE` を書くので、書き出しプロセスと spawn のワーカーも従う (検証で確認)。起動時に `NV_NATIVE=0` なら強制 OFF。
- main_window の最小変更:
  - `PREFS["native"] = True`
  - 起動時に `set_native`
  - Preferences のチェックボックス (ライブラリが無ければ無効表示)
  - accept 時に保存と `set_native`
- `run_in_child(task, run_aft, …)`: native 有効時は子プロセスを使わず worker スレッドで実行する。使えない入力は従来どおり子プロセスで実行する。
- キャンセル:
  - AFT のタイルループで `task.cancelled` を問い合わせる (Python の `_run_tiles` と同じ回数。複数スレッドでも問い合わせとタイルの確保を mutex で 1 つにする)。真なら `backend.Cancelled`。
  - 前段の BuildNoiseModel は問い合わせない。その前後で確認するので、遅れはその 1 回分 (観測 0.60〜0.61 秒)。
  - FindFlatArea は N1d/F1 の取り消しをそのまま使い、状態 5 で ProfileCancelled。
- `nv_ui_qt/export.py` は変更していない (既存の `backend._work` 経由で C の核を使う)。

## N1 コードの変更 (振る舞い不変)
- `nv_spatial`: 面ごとの本体を `spatial_v5_plane` に抜き出した (演算は不変)。
- `nv_aft`: 任意引数の取り消しコールバックを足した (既定は空。`cancel_at` 方式は不変)。
- 変更前の控えは `sandbox/omo/wp_native_bridge/orig/` (backend.py・main_window.py・Makefile も)。

## 検証

### verify_bridge.py
比較は全葉で行う: dict はキーの順序まで、型、float は 64 ビット、配列は全要素のビット、bytes は全バイト。フレームは `read_frames` で読む (test_v02 n=306..320、test.mp4 n=298..308)。

| 群 | 内容 | 結果 |
|---|---|---|
| curve | `noise_curve` と `find_flat_area` (各 -j 1/3)。test_v02 n=310/313/316、test.mp4 n=300/304 | 20/20 |
| profile | auto ×5 (test_v02 3 フレーム、test.mp4 2 フレーム)、spatial、temporal ×2、手動長方形 [40,60,216,512]。状態 0/2 を含む | 9/9 |
| aft | `run_aft` の dict (モード 0/1)、BuildNoiseModel・AFT の 0xc300 バイト (各 -j 1/3) | 26/26 (999,732 語・バイト) |
| frame | 下記 | 72/72 (306,334,381 語) |
| cancel | 下記 | 23/23 |
| fallback | 下記 | 9/9 |
| export | ON/OFF のファイルバイトとデコード結果 (575,920 B・2,773,364 B) | 4/4 |
| portable dylib `--quick` | curve・profile・aft・frame・cancel | 85/85 |

frame 群の内訳:
- test_v02 n=310..316 を既定パラメータで。
- n=310/313/316 で 7 変種: noise_level ±、threshold、pyramid、成分別の空間設定 + t_noise、mix + radius 1 + ultra-low、temporal 0、int のパラメータ + bs 8。
- Manual Fine-Tune の曲線、時間段単体 (flows が空のときと半分与えたとき)。
- PreviewEngine (crop・時間段キャッシュ・mask・assist)。
- test.mp4 の既定と成分別、4 スレッドからの同時呼び出し。

cancel 群の内訳:
- FindFlatArea の取り消し k = 1, 120, 240, 241, 242, 247, なし × -j 1/3 (状態・領域・問い合わせ回数)。
- AFT の取り消し k = 1, 100 × -j 1/3 (回数が Python と同じ)。取り消しなしの回数 = タイル数 1980。
- `run_in_child` の取り消し 3 通り。

fallback 群の内訳:
- 子プロセスで `NV_NATIVE_LIB=<なし>` と `NV_NATIVE=0`。
- np.float64 / np.float32 のパラメータ (退避 2 回ずつ、結果一致)。
- `set_native` が spawn の子に伝わる。similarity 'bogus' は両経路とも同じ ValueError。

### 自己試験 (`selftest_stats.py` = selftest.main と STATS の表示)
- ON: 96/96、23 秒、`{'native': 106, 'fallback': 0}`。「AFT cancellable」も PASS。
- OFF (`NV_NATIVE=0`): 96/96、165 秒。
- ラッパなしの素の selftest でも ON/OFF で 96/96。

### make -C native verify (11 本、変更後に再ビルド)
- quality 4000/4000
- band 5706/5706
- curve 1094 + IsClip 4000 + interp 20554 + tile 25
- lzma 667/667 (本家 blob はこの worktree に無く skipped)
- flat 362/362
- flat_cancel 676/676
- model 1244/1244 + ub 28/28
- temporal 474/474
- spatial 1287/1287
- profile 39/39
- aft: shuffle 4097・sim 3000・finish 3000・aft 67/67 (-j 1/-j 3)・combine 69/69

すべて一致。

## 速度 (負荷平均 3.1〜3.4、Python AFT は 1 回、他は 2〜3 回の最良)

| 段 | Python [s] | native 1 スレッド [s] | 倍率 | native 4 スレッド [s] |
|---|---|---|---|---|
| find_flat_area | 1.570 | 0.879 | 1.8x | 0.336 |
| build_profile (auto) | 2.382 | 1.698 | 1.4x | 1.114 |
| run_aft (128²) | 46.821 | 2.048 | 22.9x | 0.613 |
| process_frame (既定) | 2.031 | 0.567 | 3.6x | - |
| process_frame (成分別) | 2.320 | 0.590 | 3.9x | - |

## 仕様との相違点と理由
1. `nvc_profile_build` は AFT の経路で使い、`build_profile` では FindFlatArea だけを C にした。UI が呼ぶのは v5 の dict を作る `build_profile_v5` で、残り (約 0.8 秒) は C の部品がない。
2. `nv_frame_process` は作らず、時間段と空間段に分けた (PreviewEngine の時間段キャッシュ、backend の成分別設定と Mix のため)。
3. N1i に取り消しコールバックを足し、N1h から面ごとの関数を抜き出した。どちらも回帰なし。
4. `verify` には verify_bridge を入れず、`make -C native verify_bridge` で別に実行する (動画・ffmpeg・PySide6 と約 8 分が要るため)。

## 変更・新規ファイル
- 新規: `native/nvcore_c.h`、`native/nvcore_c.cpp`、`native/verify_bridge.py`、`repro/nv_native.py`
- 変更:
  - `native/Makefile` (lib・lib_portable・verify_bridge、clean)
  - `native/nv_aft.{h,cpp}`
  - `native/nv_spatial.{h,cpp}`
  - `repro/nv_ui_qt/backend.py`
  - `repro/nv_ui_qt/main_window.py` (Preferences のフラグ 1 個)
- ビルド物 (追跡外): `native/libnvcore.dylib`、`native/libnvcore_p.dylib`
- 作業記録: `sandbox/omo/wp_native_bridge/` の `log_*.txt`・`selftest_stats.py`・`smoke.py`。`orig/`・`n1_verify/`・`tmp/` は追跡外 (.gitignore の `wp_*/**`)。
- 処理コア 3 ファイル・本家バイナリ・`repro/out/` には触れていない。

## 未解決事項・注意
- build_profile_v5 の FindFlatArea 以外 (約 0.8 秒/1080p) は Python のまま。速くするには `noise_curve_spatial`・`noise_curve_temporal`・`area_band_sigma` の新たな移植と照合が要る。
- AFT の取り消しは BuildNoiseModel の間 (128² で約 0.5〜1.6 秒) は効かない。大きな手動領域ではもっと長い (未測定)。
- 時間・空間の核は 1 スレッド。
- 照合はこの環境 (numpy 2.1.3・scipy 1.15.3・Python 3.13・arm64) が前提。x86_64・universal の dylib は未作成。
- numpy のスカラーが将来 UI のパラメータに混ざると、黙って Python に戻る (結果は同じで遅くなるだけ。`nv_native.STATS` で確認できる)。
- `.app` での探索 (`sys._MEIPASS`・`Frameworks`) と `ref_floor.npy` の凍結環境での扱いは N2b で確認すること。
