# N3 fixes2 REPORT — G7・G8 (作業者 fix2-b)

転記: 統括 (2026-10-30)。作業者は REPORT.md に書けないため、作業者の最終報告をそのまま転記した (検証の指摘 F1〜F3 への対応を含む最新版)。統括の再確認と独立検証は末尾に追記する。

PR 用に `REPORT.md` から改名した。G2〜G6・G7/G8・G9 の 3 つの PR が同じパスに別の内容を足して衝突しないようにするため。共通の `REPORT.md` へのまとめは G1 の完了後に行う。

# 報告 (作業者 fix2-b): G7・G8 と、検証の指摘 F1〜F3 への対応

F1 (輝度面の 1 辺が 1 px で落ちる問題) を直しました。影響のある検証はすべて合格です。F2・F3 は下の本文に反映しました。コミット・stash・ブランチ操作はしていません。

## F1 の修正: 輝度面の幅か高さが 1 px のとき SIGSEGV になる

**原因 (観測)**: AddressSanitizer で 1×50・area (0,0,8) を動かすと、`nv_prof5.cpp` の `robust_sigma` で落ちていました。経路は `area_band_sigma` から `robust_sigma` を経て `mad_sigma` です。
- 高さ 1 だと、色差の切り出しが 0 行の空の面になります。その空の面の中央値を取って範囲外を読んでいました。
- area が None の場合は、Python も `find_flat_area_gradient` の `np.gradient` で `ValueError` を送出します。
- 手動の area の場合は、Python は `_b3` で「can't extend empty axis」の `ValueError` を送出します。
- 不具合は N3a からありました。G8 で Python UI からも踏むようになっていました。

**修正**
- (a) `repro/nv_native.py` の `_p5_args`: `min(H, W) < 2` なら None を返し、Python の経路に回します。docstring にもこの条件を加えました。
- (b) C 側では H か W が 2 未満なら −2 (引数エラー) を返します。場所は `nvc_build_profile_v5` の入口 (`native/nvcore_c_prof5.cpp`) と `nvp5::build_profile_v5` の入口 (`native/nv_prof5.cpp`) の 2 か所です。ヘッダの注記も直しました。
  - H,W≥2 では、空の切り出しは起きません。y0 ≤ H−h、h≥2 なので、切り出しは 1 行以上になります。だから修正は入口の判定だけで、H,W≥2 の結果は変わりません。
- (c) `verify_bridge` の g8 群に `case_g8_tiny` を加えました。
  - 入力は 1×1・1×2・2×1・1×50・50×1、比較用に 2×2・2×50 です。それぞれ area None と (0,0,8)、auto と spatial、-j 1 と -j 3 で回します。
  - 例外の型とメッセージ、または dict の全語を、nvfilter と比べます。どちらの経路に回ったか (C か Python か) も確かめます。
  - 結果は 56/56 一致です。1 px の辺はすべて Python の経路に回りました。2×2・2×50 は C の経路で一致しました。
  - ケースを分けて別プロセスにする前に、(a) が効いていることを確かめました。検証者の `tiny2.py` 40 通りが、修正後はすべて SAME (落ちない) でした。`c.cpp` の 1×50 は −2 を返します。
- (d) Qt C++ では、使い捨てのハーネス `qt_tiny.cpp` (`nvqt::build_profile` を直接呼ぶ) で確かめました。
  - 1×1・1×2・2×1・1×50・50×1 は、area None・[0,0,8] とも例外 `nvc_build_profile_v5 failed (-2)` になり、落ちません。
  - 2×2・2×50 は dict を返します。
  - UI ではこの例外を runner の失敗処理が受け、エラーとして出します (経路の確認のみ)。1 px の動画を UI に実際に読み込んでは試していません。

**残る差 (記録)**: 1×1 で手動の area を渡したときだけ、両 UI の結果が違います。
- Python (OFF・ON とも) は dict を返します。Qt C++ はエラーになります。
- UI からは起きないと考えます。Build Profile は 32 px 未満の選択を受け付けず、Auto Profile (area None) は両方ともエラーになるためです。
- 検証者は「1×1 は OFF が ValueError」と書いていましたが、それは area None の場合でした。

**再検証**
- lib と qtui を再ビルドし、警告は 0 でした。
- `verify_bridge --only g8,prof5,cancel,frame`: g8 152/152、prof5 110/110、cancel 49/49、frame 72/72 で、全語一致しました (281 秒)。
- `verify_prof5`: 630/630。
- Python 自己試験: 96/96 (ON・OFF とも)。
- `qtui_selftest`: 105/105。

## G7: AFT の取り消しを前段でも効かせる

**遅れの原因 (観測)**: AFT の UI 経路は `run_in_child → nv_native._apply_aft → profile_blob → aft_blob` です。このうち `profile_blob` (中身は `nvc_profile_build`、N1f の BuildNoiseModel) に取り消しの問い合わせがありませんでした。Qt C++ 版の `run_aft` も同じ関数を呼んでいて、同じ遅れがありました。

**変更**
- `nvm::model_full` に問い合わせを入れました。位置は、段 × 方向の組ごと、族 A・B・C の方向ごと、`FUN_00ad319c` の各回と ad1ae0 の前です。答えが真なら新しい戻り値 `CANCELLED = 3` で抜けます。
- `nvp::build_profile` と `tile_dev` は、6 面それぞれの前、各タイルの前、LZMA の前に問い合わせます。複数スレッドから呼んでも 1 つずつ順番に問い合わせ、一度真になったらそれ以降は問い合わせません。
- C ABI に `nvc_profile_build_cancel(..., nvc_cancel_fn, out)` を追加しました。戻り値 3 が取り消しで、既存の `nvc_profile_build` は変えていません。
- `NVC_ABI_VERSION` を 2 に上げ、`nv_native.ABI_VERSION` も 2 にしました。古い dylib は読み込まれず、Python 経路に切り替わります。
- `nv_native.profile_blob` は取り消されると `Cancelled` を送出し、`_apply_aft` がそこに cancel を渡します。`backend` の公開 I/F は変えていません (docstring だけ更新)。
- Qt C++ の `qtui/src/core/nvcore.cpp` の `run_aft` も、CancelScope を使って `nvc_profile_build_cancel` を呼ぶようにしました。

**相違点 (確認してほしい)**
- **取り消し関数の形**: タスク文は `bool(*)(void*)` + userdata でしたが、既存の nvcore_c の規約 `int(*)(void)` に合わせました。
- **問い合わせの位置**: タスク文の「領域探索と曲線の各タイル」ではなく、上記のモデル・タイル・LZMA に入れました。遅れの原因がそちらだったためです。

**遅れの実測** (test_v02 n=312、負荷平均 7.8〜20)

| 経路 | 領域 | 変更前の最大 (1 / 4 スレッド) | 変更後の最大 (1 / 4 スレッド) |
|---|---|---|---|
| Python `run_in_child` | 128² | 1.03 / 0.44 秒 | 0.014 / 0.029 秒 |
| 同上 | 216×512 | 5.48 / 1.93 秒 | 0.045 / 0.15 秒 |
| 同上 | 512² | 12.6 / 4.31 秒 | 0.168 / 0.13 秒 |
| Qt C++ `run_aft` (使い捨てハーネス、同じ 3 領域) | 128²〜512² | 未測定 | 0.086 秒 (全条件の最大) |

- **問い合わせの間隔**: 最大 0.331 秒でした (512²・1 スレッド、約 6.26 万回)。これが遅れの上限になります。
- **UI から通しで (PySide6、offscreen)**: ウィンドウの Cancel (close) を押しました。
  - Auto Profile の後の AFT の最中: 1.19 秒から 0.001 秒になりました。
  - 512² の Build Profile の後の AFT の最中: 13.1 秒から 0.002〜0.096 秒になりました。
  - プロファイル構築そのものの最中: 前後とも 0.004 秒以下でした。
- **取り消さないときの結果**: 変わりません。`profile_blob` は 128² で Python の `nv_profile.build_profile` と、344×216 で旧 `nvc_profile_build` と全バイト一致しました。取り消しを k 回目に真にすると、ちょうど k 回目で止まります。
- **問い合わせのコスト**: 同じ負荷で測った範囲では誤差程度でした。4 スレッドの 512² では約 +5% に見えます。

## G8: プロファイル構築の完全 C 化

**方針**: 既存の `nvc_build_profile_v5` (N3a) は、UI が使う出力をすべて返していました。そのため新しい関数は作らず、`nv_native.build_profile_v5` をこれに繋ぎました。
- FindFlatArea のキャッシュは従来どおり使います。キャッシュにあれば、その領域を手動の正方形として渡し、flat_status を後から書きます。
- C で同じ結果を保証できない入力は従来の経路に回します。対象は、float32 で有限の値でない面、輝度の辺が 2 px 未満 (F1 で追加)、他の色差の形、levels が 5 以外、未知の mode / matrix、要素数が 3・4 以外の area、C 側のエラーです。

**速度化** (どれも結果は不変)
- `nv_prof5`
  - 大きな分位点は、値が非負かつ NaN なしのときだけ、ヒストグラムで絞ってから nth_element にしました。
  - `_dead(Y)` は 3 回計算していたのを 1 回にしました。
  - ブロック統計は、面全体をコピーせず、ブロックごとに取り出すようにしました。
  - 時間方向の選択は 1 回の走査にしました。
- `nvn::b3` を、演算順を固定した NEON (.4S のインラインアセンブラ) にしました。反射のいらない区間だけで、portable ビルドはスカラーのままです。

**`nvn::b3` の NEON 化が及ぶ範囲 (F3)**: `nvn::b3` と `atrous` は、`nv_spatial` (spatial_v5) と `nv_temporal` (時間段) も使います。つまりこの変更は、プロファイル構築だけでなく、フレーム処理と書き出しの経路にも効きます。それでも結果はビット不変です。
- 作業者の検証: verify_spatial 1287/1287、verify_temporal 474/474 (どちらも NaN の払い出し値まで比較)。
- verify_bridge: frame 72/72 (3.06 億語)、export 4/4。
- 検証者のプローブ: b3 と atrous について、1 シードあたり 365 万語を HEAD 版と比べました。入力は NaN・inf・非正規数・±0 を含み、3 シードとも一致しました。

**速度 (F2: ばらつきも含めて)**: 条件は 1080p test_v02 n=312・auto・キャッシュなしで、元の 2.38 秒と同じです。
- `verify_bridge --bench` の最良値: 1 スレッドで Python 2.374 秒からネイティブ **1.174 秒**、4 スレッドで 0.532 秒。
- ただし負荷下ではばらつきます。検証者が HEAD 版の経路と交互に測った値 (負荷平均 8.5〜10.2) は、変更後 1 スレッドで 1.19〜1.44 秒でした。**6 回中 2 回は 1.3 秒を超えました**。同じ条件で HEAD 版の経路は 1.74〜1.92 秒です。
- 4 スレッドは 0.52〜0.56 秒で、HEAD 版の経路は 1.19〜1.25 秒でした。
- 作業者の交互測定も同じ傾向で、変更後 1 スレッド 1.26〜1.50 秒、HEAD 版の経路 1.77〜2.02 秒でした。

**照合**
- verify_bridge の g8 群は、F1 の追加後に 152/152 です。中身は次のとおりです。
  - 実写 6 フレーム × 2 領域 × -j 1 / 3。フレームは test_v02 n=120/455/690 と test.mp4 n=40/520/860 で、他の照合スクリプトの範囲外です。
  - 変種: 10 ビット読み込み、4:4:4、奇数サイズ、full range、BT.2020、pair が more[0] と別、pair・more なし、キャッシュに当たる 2 回目。
  - Python 経路に回るべき入力 (uint8、NaN、levels 4、未知の mode、5 要素の area、1 px の辺) は、実際に Python 経路に回り、結果も一致することを確かめました。
  - 取り消し: 結果と問い合わせ回数が Python と一致しました。
- `verify_bridge_prof5` の参照は native OFF (Python) にしました。110/110 一致です。
- `verify_prof5` 630/630。HEAD の C との比較 246/246 (非有限入力を含む) でも一致しました。

## 共通の検証

**F1 修正前の最終コード (12:09) で全部を通した結果**
- ビルド警告は 0 です (lib・lib_portable・qtui)。
- `make -C native verify`: 11 本すべて一致。
- `verify_bridge --export`: 全群一致 (928 秒)。
- Python 自己試験: 96/96 (ON・OFF)。`qtui_selftest`: 105/105。
- `compare_ui`: 一致しました。差は既知の Help > About Qt だけで、書き出し 2 本もバイト一致です。
- `compare_export`: 19/19 一致。

**F1 修正後に、影響のある分だけ回した結果**: F1 の節の「再検証」に書いたとおりです。

## 確認したいこと
- **ABI 版数**: 2 に上げたのでマージ時に確認してください。並行している作業者が nvcore_c に関数を足す場合は、版数の調整が必要です。
- **プロファイル構築の後段**: 曲線・帯域の計算 (1 スレッドで約 0.35 秒) には問い合わせを入れていません。Python も FindFlatArea の後は問い合わせず、入れると問い合わせ回数が Python とずれるためです。この部分の遅れは 1 秒未満です。
- **Qt C++ の自己試験**: `aftCancellable` は 5 秒未満を確かめるだけの弱い検査のままです。並行して G4・G5 の担当が selftest.cpp を編集するので、触っていません。

Files are in /Users/seitan/Desktop/work/reverse/.claude/worktrees/n3fix-b:
- native: nv_model.{h,cpp}, nv_profile.{h,cpp}, nvcore_c.{h,cpp}, nvcore_c_prof5.{h,cpp}, nv_prof5.{h,cpp}, nv_ndimage.cpp, verify_bridge.py, verify_bridge_prof5.py
- repro: nv_native.py, nv_ui_qt/backend.py (docstring only)
- qtui: src/core/nvcore.cpp
- sandbox/omo/wp_qtui_fix: PROGRESS_b.md (progress log), orig_b/ (pre-change copies)

Logs and measurement scripts are in /private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/fix2b/:
- f1_vb.log, f1_vp5.log, f1_all.log (re-verification after F1)
- final.log (full run before F1)
- qt_tiny.cpp (Qt C++ 1-px check)
- lat.py, gap.py, ui_cancel.py, qt_lat.cpp (cancel latency)
- bench_bp.py (speed)

---
## 統括の再確認 (2026-10-30)
- `make -C native lib` の後 `python3 native/verify_bridge.py --only g8,cancel,prof5`: g8 96/96 (6830 語)・cancel 49/49 (199,807 語)・prof5 110/110・aft 2/2、ALL MATCH (216 秒)。ログ `sandbox/omo/coord_fix2b/rerun_vb.log`
- `NV_NATIVE=1` の Python 自己試験 96/96。ログ `sandbox/omo/coord_fix2b/rerun_st_on.log`
- 独立検証: `sandbox/omo/coord_fix2b/VERDICT_g78.md` — Round 1 REQUEST CHANGES (F1 必須・F2/F3 報告修正) → Round 2 **APPROVE** (2026-10-30)
- F1 修正後 (Round 2): `make -C native lib` 警告 0、`verify_bridge --only g8` 152/152 (8488 語、1 px 辺のケースを含む)、ALL MATCH (57 秒)。ログ `sandbox/omo/coord_fix2b/rerun2_vb_g8.log`
