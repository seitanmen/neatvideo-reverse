# N4a / N4b 経過 (作業者、2026-10-31)

worktree: `.claude/worktrees/n4-ofx` (branch n4-ofx、origin/main 5b26761)。コミットしない。REPORT は統括。

## 2026-10-31

- タスク `.claude/tasks/native_n4_ofx.md`、`native/MIGRATION.md` (移植の知見・C++ ファースト方針)、
  `docs/INTERLACE_PROGRESSIVE.md`、`docs/GPU_STATUS.md` を読んだ。
- `make -C native lib` で `native/libnvcore.dylib` を作成 (install name `@rpath/libnvcore.dylib`)。
- OpenFX ヘッダ: Resolve 同梱の `OpenFX-1.4/include/` から、SPDX BSD-3-Clause の 8 本だけを
  `ofx/vendor/openfx/include/` に写した (ofxCore / Property / Param / ImageEffect / Interact / Message / Memory /
  MultiThread)。ライセンス表記のない Resolve 拡張 (ofxImageEffectExt.h・ofxParamExt.h) と未使用のものは写していない。
- 設計 (決定):
  - 画素: ホストの RGBA float32 (OFX は下の行から) → 上の行からの G,B,R 平面 (gbrpf32le) →
    `nvc_depth_sanitize` (非有限値を記録) → `nvc_depth_to_internal` (full range YCbCr 4:4:4、0..255 尺度)。
    出力は `nvc_depth_from_internal` (非有限値を戻す) → RGB、α は入力のまま。
    Python 側の参照は `nv_depth.to_internal / from_internal` (`pixfmt("gbrpf32le")`) で同じ写し方。
  - 対応: float32 RGBA のみ (8/16 ビットは拒否)。タイル・多解像度なし。インターレース (フィールド順が
    upper/lower、または field 単位の render 要求) はエラーで拒否。
  - プロファイル: レンダー中にホストから Profile Frame (クリップ先頭からのオフセット) と後続 2 フレームを
    取得し、`nvc_build_profile_v5` (UI と同じ) → AFT (`nvc_profile_build_cancel` + `nvc_aft` + `nvc_aft_combine`、
    `backend._run_aft` と同じ手順) でインスタンス内に作る。基準値 (reference floor 51 語) は
    `data/ref_floor.npy` の値をソースに埋め込む (実行時にファイルを読まない)。
  - パラメータ → v5 パラメータの写像は `ofx/src/params.cpp` (`state.backend_params` と同じ式)。
- 実装 (ofx/): `src/plugin.cpp` (OFX アクション)・`src/params.{h,cpp}` (パラメータ → v5 ブロック)・`src/core.{h,cpp}` (C ABI 呼び出し)・
  `src/ref_floor.inc` (基準値 51 語)・`CMakeLists.txt`・`exports.txt`・`Info.plist.in`・`build_ofx.sh`・
  `tools/minihost.cpp` (試験用ホスト)・`tools/fsguard.c` (ファイル/プロセス呼び出しの監視、DYLD_INSERT_LIBRARIES)・`tools/verify_ofx.py`。
  `.gitignore` に ofx/ のソースの許可を追加 (コミットはしていない)。
- 観測: 本家の `FoundryOfxAllowPluginUnload` は `mov w0,#0; ret` (arm64 スライスの静的逆アセンブル、otool で読むだけ)。こちらも 0 を返す。
- 観測 (minihost --check): 3 エクスポートのみ (nm -gU)、OfxGetNumberOfPlugins 1、pluginApi OfxImageEffect / apiVersion 1 /
  com.meatvideo.denoise 1.0、Describe・DescribeInContext(Filter)・CreateInstance・DestroyInstance・Unload すべて OK。
- 観測 (照合、`python3 ofx/tools/verify_ofx.py --pure --mutate`、ログ `logs/verify_full_pure.txt`): 5 場面 24 レンダー ×
  参照 2 種 (NV_NATIVE=1 のブリッジ / NV_NATIVE=0 の Python) = 48 比較で 0/66,248,704 語が不一致。
  端のフレーム (窓が足りない t=0,1,末尾)、パラメータ 6 組 (プロファイルの作り直し・行列 3 種・モード 3 種・AFT なし・Mix・
  時間 / 空間オフ)、半径 1〜5、HDR・負値・NaN/±inf、1080p、クリップ先頭時刻 100・原点 (10,20)・負の rowBytes を含む。
  空振りの確認: プラグイン側 Threshold 4.0001 で 5/5 レンダーが不一致、参照側の入力 1 語 1 ulp で 2/5 が不一致。
- 途中で見つけた問題と修正: minihost が native/libnvcore.dylib をリンクしていたため、dyld がそれを再利用し、バンドル内の
  Frameworks/libnvcore.dylib が使われていなかった (内容は同一)。minihost のリンクを外し (基準値の確認は dlopen でバンドル内の
  ものを使う)、照合をやり直した。DYLD_PRINT_LIBRARIES でバンドル内の libnvcore が読まれることを確認。
  build_ofx.sh の install_name_tool は PATH 上の Anaconda の cctools-port だったので `xcrun -f install_name_tool` に変更。
- 観測 (ファイル非使用): fsguard (open/openat/fopen/stat/access/popen/system/fork/exec*/posix_spawn*/dlopen/stdin の read など
  をインターポーズ) は、全場面でプラグインのセッション中にホスト自身の dlopen(バンドル) 1 件だけを記録。陽性対照
  (`minihost --fsguard-selftest`) は 5 件を記録。静的: MeatVideo.ofx の import にファイル・プロセス系の関数なし。
  libnvcore.dylib は fopen/fread を import するが、呼び出し元は `nvc_reference_floor(path≠NULL)` だけで、プラグインは
  nvc_reference_floor を import していない。
- クリーンビルド (`rm -rf ofx/build && ofx/build_ofx.sh`) の警告 0 (`logs/build_clean.txt`)。
- 既存の検証 (観測):
  - `make -C native verify` exit 0、11 本すべて一致 (quality 4000/4000・band 5706/5706・curve ALL BIT-EXACT・lzma 667/667
    (本家 blob の比較はこの worktree にキャッシュがなくスキップ)・flat ALL BIT-EXACT・flat_cancel 676/676・model 1244/1244・
    temporal 474/474・spatial 1287/1287・profile PASS・aft ALL BIT-EXACT)。ログ `logs/native_verify.txt`。
  - qtui: `cmake --build qtui/build_main` 警告 0。`QT_QPA_PLATFORM=offscreen NV_TEST_VIDEO=test.mp4 qtui_selftest` 117 passed / 0 failed
    (初回は qtui_selftest だけをビルドしたため、qtui_ofx_drv を起動する OFX 3 項目が失敗。全ターゲットをビルドして解消)。
    `~/.config/NeatRepro/nv_ui_qt.ini` の mtime・サイズは実行前後で同じ (NV_UI_QT_SETTINGS を scratchpad に向けた)。
  - C ABI は変更なし (native/・reference/ は無変更、NVC_ABI_VERSION = 3、nv_native.ABI_VERSION = 3) なので verify_bridge は省略。
- 注意: `.gitignore` は `!work/` がないため work/omo/wp_native_ofx/ の新規ファイルは無視される (既存の追跡ファイルは影響なし)。

## 2026-10-31 第 2 回 (VERDICT.md の L-1〜L-4・I-1・I-2・I-4 の修正)

- L-1: Render の失敗はすべて kOfxStatFailed + 永続メッセージ (インターレース・field render・画像の Field・8/16 ビット・
  64 px 未満・プロファイルのフレームがない)。kOfxStatErrUnsupported は plugin.cpp から消えた。
  GetClipPreferences を追加し、Source/Output とも RGBA・Float を要求する (ホストが変換できれば変換する)。
  Describe の説明文にも、プログレッシブの RGBA float32、64×64 以上だけを受け付けることを書いた。
- L-2: in->mu の中では、プロファイルの有無と key の比較、コピーだけを行う。clipGetImage とプロファイル作成はロックの外で行う。
  InstanceChanged は世代番号 gen を進め、作成中に無効化されたプロファイルは、そのレンダーでは使うが保存しない。
  試験: minihost --reenter (ホストが Source の clipGetImage の中から、同じスレッドで GetFramesNeeded と InstanceChanged
  (Rebuild Profile) を呼ぶ)。16〜27 回の再入でデッドロックせず、出力は通常のレンダーとバイト同一。
  各レンダーがプロファイルのフレームを宣言し直して取り直すことも確認した (無効化された作成結果は保存されない)。
- L-3: プロファイル作り直しの条件 profile_key (設定 + pf + pf での Source の RoD) を GetFramesNeeded と Render の両方で使う。
  minihost の宣言検査は、画像が返らなかった要求も含めるようにした。--range で途中にクリップ範囲を変える場面でも declared=1。
- L-4: 近傍が取得できない (clipGetImage が Failed) ときは、その側の窓をそこで打ち切る (クリップ端と同じ扱い)。
  プロファイルの後続フレームも、最初の欠けで打ち切る。プロファイルのフレームそのものが無ければ、メッセージ付きで失敗する。
  参照側は、同じフレームの並びを backend.process_frame / backend.build_profile(frames, 0) に渡す。これは UI の端の扱い
  (PreviewEngine.window・profile_window) で、クリップがそこで終わっている場合に当たる。
  A-missing 場面 (欠け 2,5、範囲 [1,6] への変更を含む 10 レンダー) で、ネイティブ・Python 参照とも一致。
- I-1: abort は kOfxStatFailed を返し、メッセージは出さない (既存のメッセージも消さない)。方針として plugin.cpp の render の注釈に書いた。
  verifier の minihost_v (MH_ABORT_AT=1..8) で確認: 中断したレンダーは status 1 でメッセージなし、画像はすべて解放される。
  直後の再レンダーは中断なしの出力と一致し、作成後の中断ではプロファイルが再利用される。
- I-2: Mix パラメータは UI の表示値 (既定 100 = フィルタ結果のみ)。内部の mix = 100 − Mix (rows.py の "inv")。
- I-4: Color Matrix は Auto (既定) / BT.709 / BT.601 / BT.2020。Auto はタグなしの nv.matrix_from_tags で、720 行以上なら BT.709、
  それ未満は BT.601。160×720 → bt709、160×719 → bt601 で、両参照とビット一致 (`logs/verify_matrix_720.txt`)。
- 付随の発見 (観測): 失敗の経路で fsguard が dlopen(/usr/lib/libstdc++.6.dylib) を記録した。逆トレースによると、macOS の libc++abi が
  std::runtime_error の破棄時に行う GCC 互換の確認 (__refstring_imp::compute_gcc_empty_string_storage) だった。
  プラグインの例外を std::string を持つ独自型 mvofx::Error にして解消した (失敗の経路でも記録はバンドルの dlopen の 1 件だけ)。
- 結果:
  - `verify_ofx.py --pure --mutate`: 7 場面 37 レンダー × 2 参照 = 74 比較で 0/90,210,304 語。
  - 拒否 6 種はすべて status 1 + メッセージ。mutation 1 は 5/5、mutation 2 は 1/5 で検出。
  - verifier/run_extra.py --pure: 40 比較で 0/10,507,232。
  - クリーンビルドの警告 0。fsguard は全場面・全拒否でバンドルの dlopen の 1 件だけ。
  - native/・qtui/・reference/ は無変更 (前回の make verify・qtui selftest の結果がそのまま有効)。
