> 統括による転記 (2026-10-29)。作業者 n3c-qtui の最終メッセージ全文 (作業者の REPORT.md 書き込みは harness で拒否されるため)。

N3c 完了報告 (Qt C++ UI の残り: Preferences / Profile Viewer / Batch Profiler / Generic Profile / Variants / OFX モード / Bit depth、N3b 指摘 M2・L5・L6)。コミットなし。AGENTS.md・MIGRATION.md・スナップショット・repro/・repro/out/・本家バイナリには触れていない。「観測」= 実行結果、「推測」と書いたもの以外は観測。作業者の報告ファイル禁止の規約のため REPORT は本メッセージが全文 (進捗は sandbox/omo/wp_qtui_c/PROGRESS.md)。分担作業者は使わなかった (名前付き fork を起動できなかったため単独で実施)。

■ 合格条件ごとの結果 (すべて観測)
1. selftest: qtui_selftest 103 項目 (N3a 34 + N3b 32 + N3c 37) 全 PASS (init/cleanup 込み 105 件、3 回連続、各 約 18.5 秒)。ログ sandbox/omo/wp_qtui_c/log_selftest.txt。
2. 書き出しのバイト一致:
 - compare_export (エンジン単体、Python `-m nv_ui_qt.export` NV_NATIVE=1 と比較) 19/19 ALL SAME。N3b の 14 に N3c で 5 ケース追加: 4:2:2 10bit → prores (バイト一致)・4:2:2 → FFV1、gbrpf32le の NaN/±inf 入り全 6 フレーム → FFV1 (sanitize/restore 経路)、EXR 連番 4 枚 (各フレームのバイト一致)、gbrp10 → FFV1。ffmpeg/ffprobe のコマンド列も全ケース語単位で同一。mkv は復号フレーム (framemd5) + 長さ。出力 sandbox/omo/wp_qtui_c/depth/export/。
 - UI 経由: compare_n3c で Bit depth 10 の 10bit 源 → mp4 (libx264 High 10) バイト一致 55,394 B、Source depth の 4:2:2 源 → mp4 (4:2:2) バイト一致 22,357 B、32bit (gbrpf32le) → mkv 復号一致 1,486,306 B。付随の ui_params / ui_profile は文字列同一、<stem>.json (seconds 以外) 同一。N3a/N3b の compare_ui 回帰 (test.mp4) でも export_a/b バイト一致。
3. 新しい画面の値と窓 grab (compare_n3c.py = qtui_dump_n3c vs py_dump_n3c.py → compare_ui.py + 追加比較):
 - state 8 枚 (8bit / 10bit 再読込 / 10bit 源 / 4:2:2 / 32bit float の主窓 25 項目 = パネル・ステータス・Performance 表・Image information・書き出し既定、Profile Viewer (行・本文・Profile mode の保存)、Profile Viewer (プロファイルなし)、Batch Profiler ダイアログ) 全項目一致、プロファイル JSON 同一。
 - ペイン画像 21/21 バイト一致 (10bit・4:2:2・32bit float のフレームの表示、311 万画素)。
 - grab: dlg_batch / dlg_pviewer / dlg_pviewer_none / win_t8 / win_t10 / win_s10_adjust / win_s422_32 の 7 枚が 0 画素差。dlg_prefs だけ高さが違う (Python 285 / C++ 340 px): 上端から 224 行 (Python の 78.6%) と最下段のボタン列 40 行は 0 画素差、重なり部分の差は 7.51%。理由は C++ の追加 2 行と native 欄の無効表示 (相違点 1)。
 - Batch Profiler の出力 5 件 (8bit 2 本・spatial・10bit・32bit) の .nvprof.json が文字列一致。settings.ini の [prefs] 10 キーが文字列一致。
 - 並置画像: sandbox/omo/wp_qtui_c/n3c/sbs/ (左 Python・中 C++・右 差×8)、生の grab は n3c/py・n3c/cpp。
4. OFX モードの往復 (compare_ofx.py: qtui_ofx_drv vs py_ofx_drv.py、同じ stdin と UI 手順): stdout 26/26 ケースでバイト一致 (640×360 と奇数 321×181、--depth 8/16/32、手順: close / Apply 2 回 (2 回目は書かない) / 編集・別フレーム・Radius / Auto Profile+AFT・Noise Level・Cancel / Cancel のみ (0 B) / Adjust で Mix・Noise Cr、MAX_FRAMES 3・FPS 29.97)。各手順後のステータス文字列と stderr の OFX emit error 行も一致。16 は下位 8 ビット乱数入り、32 は全フレームに NaN/±inf (書き戻しも一致)。OFX 窓 (depth 8/32 × Auto Profile 前後): メニュー・状態 29 項目・窓 grab 0 画素差。引数エラー 8 種 (usage / no frames) は終了コード・stdout・stderr 一致。ログ sandbox/omo/wp_qtui_c/ofx/log_compare_ofx.txt。
5. メニュー 114 項目: 動画+プロファイルあり、OFX 窓 (Auto Profile 前後) のすべてで Python と文言・順序・有効/無効・ショートカットが同一、差は Help > About Qt の追加 1 件のみ (C++ 115)。N3b の「N3c 3 項目無効」の差は解消。
6. 照合: verify_bridge_depth (新規) 208,737,760/208,737,760 語ビット一致 (+ 感度試験: 参照を 4 種改変 → 4/4 検出、sandbox/omo/wp_qtui_c/depth/sensitivity.py)。verify_bridge.py --quick (depth 群を追加) ALL MATCH 324 s (curve 12/12・profile 7/7・aft 11/11・frame 32/32・cancel 23/23・fallback 9/9・prof5・ui 16,346・depth)。verify_bridge_ui 全量 34,755/34,755。make -C native verify の 11 本 (2 回に分けて実行) 全一致: quality 4000/4000・band 5706/5706・curve 1094+IsClip 4000+interp 20554+tile 25・lzma 667/667・flat 362/362・flat_cancel 676/676・model 1244/1244+ub 28/28・temporal 474/474・spatial 1287/1287・profile 39/39・aft (combine 69/69 ほか) ALL BIT-EXACT。
7. クリーンビルド警告 0: native (make clean → all) と qtui (新規ビルドディレクトリで cmake 構成から) の両方 (ログ log_native_build.txt / log_qtui_clean_build.txt)。

■ N3b 指摘への対応
- M2: nv_depth の数値 (sanitize、yuv 整数の正規化、4:2:2 の widen と戻し、RGB↔YCbCr (float64、_KRB)、clip(rint)、float 出力と restore、OFX の unpack/pack 16/32 と pack 8) を native/nv_depth.cpp + C ABI nvcore_c_depth.h へ移動。2 変数演算は nv_portable の nvp::add/sub/mul/div (被演算子順固定)、fin.max/min は FMAX/FMIN (numpy で +0 > −0 を確認)、整数出力の NaN は 0 (numpy 2.1.3 arm64 の cast で観測) を明示。qtui 側は src/core/depth.{h,cpp} がバッファを動かすだけ。照合と書き出しの再確認は上記 2・6。
- L5: spawn の pipe を FD_CLOEXEC、posix_spawn に POSIX_SPAWN_CLOEXEC_DEFAULT (+ 継承する stdout/stderr は addinherit_np)。書き出し中の lsof で reader/encoder の fd が Python 版と同じ (0..4) になった (depth/l5_fds.py)。
- L6: fixtures を qtui/tests/fixtures/ に移し .gitignore で許可 (check-ignore で追跡可を確認)。selftest は QSKIP をやめ、無ければ FAIL。
- L7・L8: 変更なし (記録)。L8 と同型: N3c の ofxRoundTrip / ofxApplySameAndCancel / bitDepth10ExportEqualsProcessFrame の期待値は C++ の同じ関数で作る (Python との一致は compare_ofx / compare_n3c が担う)。

■ Python 96 項目 → C++ 項目 (N3c 新規は ★)
1 表示式 → normalIdentityAt50 (+ verify_bridge_ui・compare_ui のペイン) / 2 → selectionColours / 3 → menusInGenuineOrder・toolbarsInGenuineOrder・★menusAllEnabledN3c / 4 → viewerModePerPage / 5 → selectionByMouse・selectionMoveAndClear / 6 → buildProfileSizeRules・notUniformAndTriangle / 7 → ★componentPanesEveryMode / 8・10 → profileCheckToggle / 9 → ★pcheckRecomputesOnNoiseLevel / 11 → ★profileViewerDetails・★profileViewerWithoutProfile / 12 → ★panelNoScrollBarAndOrder (暗い情報欄・ボタン順の画素は compare_ui の grab) / 13 → ★selectionPercentVsMain / 14 → nlcBeginShowsMask / 15 → temporalMaskEdges / 16 → nlcRecomputesOnTemporalSettings・nlcAnotherFrameDropsImage / 17 → tNoiseScalesTemporalOnly / 18 → exportTNoiseDiffersFromDefault / 19 → nlcThreeEntriesAndAssist・nlcEndsOnPrepareAndWithoutProfile / 20 NLC R3 パネル状態 → 対応項目なし (理由: compare_ui の filter_nlc.json 143 項目 (enabled/hidden) で Python と一致を確認済み) / 21–26 g の値 → 対応項目なし (理由: g は libnvcore の nv_quality 内で UI に公開されない。native/verify_quality.py 4000/4000 が担う) / 27・28・30–32 → qualityPanel / 29 → ★aftBinFlagsStored / 33・34・36・37 → ★qualityFreshResetAndNone / 35 → resetFineTuneCompleteness / 38・39 → ★profileViewerDetails / 40 → loadAndAutoProfile / 41 → adjustPreviewFiltered / 42 → defaultsMapToV5・defaultValuesShown / 43 process_frame == process_frame_v5 → 対応項目なし (理由: C++ に Python 経路がない。verify_bridge frame 32/32 と N1g/N1h が担う) / 44 成分別帯域 → exportLosslessEqualsProcessFrame (bamt_Cr_medium) + verify_bridge frame / 45 → bandAvailability420 / 46 → sliderDragMergesUndo / 47 → applyButtonRefreshesPreview (部分的: 世代とプレビューの再計算) / 48 → ★everyControlChangesBackend / 49 → ★naControlsDisabled / 50・51 → sliderDragMergesUndo・exactValueEntryAndReset・undoHistoryMenu / 52・53 → presetSaveLoad・presetDirtyStarAndReset・presetProfileFilesFromPython / 54 → ★resetUsesDefaultPreset / 55 → loadAndAutoProfile / 56 → undoRedoProfile / 57 → aftTwiceEqualsOnce / 58・59 → ★staleAftDiscardedAndButtons / 60 → ★aftCancellable / 61・65 → profileSaveLoad / 62 → autoCompleteBins・autoCompleteThroughCore / 63 → manualFineTune / 64 → genericProfileDefaultOrReset・profileCheckToggle (Reset Profile) / 66・87 → buildProfileSizeRules・buildProfileFromSelection / 67 → mostNoisyFrame / 68 → ★saveFramePng・splitsArrangePanes / 69・70 → previewArea / 71 → filterPanelLayout / 72 → frequenciesRowWritesThree / 73 → mixEnableStartsAt25 / 74 → profileNoiseLevel / 75 → holdShowsOriginal / 76 → splitsArrangePanes / 77 → thumbnails・thumbnailWindowFollowsRadius / 78 → beginnerAdvanced・beginnerHidesAdvancedRows / 79 → sliderDragMergesUndo / 80 → ★wheelUndoSteps / 81 → ★sidePanelsFitAtMinWidth / 82 → ★profileJsonNonFiniteRejected / 83 → ★naControlsDisabled・★variantsDisabled / 84 → ★trackRangesAndDefaults / 85 → buildProfileSizeRules (Auto↔Build の文言) / 86 → ★batchProfilerRuns (出力 = frame N//3 の build_profile と一致)・★batchProfilerDialog / 88・90 → exportLosslessEqualsProcessFrame / 89 矩形プロファイルでの書き出し → exportLosslessEqualsProcessFrame (ui_profile == effective) の部分対応 (矩形は buildProfileFromSelection で確認、両者を 1 項目では結んでいない) / 91 → ★bitDepth10Reload / 92 → ★bitDepth10ExactValues / 93 → ★bitDepth10StatusBar / 94 → ★bitDepth10Preview / 95 → ★bitDepth10ExportFilter / 96 → ★bitDepth10ExportEqualsProcessFrame。
N3c のその他の新規: ★preferencesDialogRows、★preferencesAcceptWritesKeys (OK で prefs/<key> 10 キー、Cancel は不変)、★genericProfileDefaultOrReset、★depthConversionsRoundTrip、★ofxWindowSetup、★ofxArgsAndNoFrames、★ofxRoundTrip (8/16/32)、★ofxApplySameAndCancel、★bitDepthMemoryCapAndFloat (メモリ上限 = float32 のバイト数、32 = gbrpf32le full range 4:4:4)。menusEnabledForN3b は N3c の期待 (3 項目有効) に更新。

■ 仕様 (Python 版) との相違点
1. Preferences: 課題の一覧 (auto AFT・ffmpeg パス) に合わせ、Python の 9 行の後に C++ だけの 2 行「Auto Fine-Tune after Auto / Build Profile」(prefs/auto_fine_tune。Python は既定値だけで行がない) と「ffmpeg (directory or binary)」(prefs/ffmpeg、N2b のランチャが読むキー。空ならキーを消す、OK で探索し直す) を足した。「Use the native core」は C++ に Python 経路がないので チェック済み・無効 (prefs/native は書き換えず Python 版用に保持、ツールチップで説明)。それ以外の行・キー・既定値・範囲・ツールチップは Python と同一。
2. OFX モードは同じ MeatVideoQt (main.cpp の --ofx 分岐、src/app/ofx.cpp) で、ffmpeg 未検出のダイアログは出さない (Python の OFX 経路も ffmpeg 不要)。Python の _ofx_main は注入後に display() を呼ばないので、最初の操作 (フレーム・プロファイル・設定の変更) まで画面が空 — C++ も同じにした (観測: OFX 窓 grab が 0 画素差)。
3. OFX の引数解析は Python の int()/float() に合わせたが、"1_000" (Python は可) と 16 進浮動小数 (C++ の stod は可) は一致しない (端の入力、未対応)。
4. (Python と同じ挙動の記録) Preferences の mem_gb 欄は 0.25..64 なので、0.25 未満の保存値で OK を押すと 0.25 になる (両 UI 同じ。selftest は戻している)。
5. N3a どおり Help > About Qt を追加。

■ 変更・新規ファイル
native/ (N3c の影響範囲): 新規 nv_depth.{h,cpp}、nvcore_c_depth.{h,cpp} (nvc_depth_sanitize・nvc_depth_to_internal・nvc_depth_from_internal・nvc_ofx_unpack・nvc_ofx_pack)、verify_bridge_depth.py。変更 Makefile (LIB_SRC / LIB_HDR に nv_depth.cpp・nvcore_c_depth.cpp を追加のみ)、verify_bridge.py (depth 群のループ登録と既定の --only に depth)。既存関数の挙動・ABI 版は不変 (11 本と verify_bridge で回帰なし)。N3a/N3b 由来の未追跡ファイルは触っていない。
qtui/: 新規 src/core/depth.{h,cpp} (PixFmt を export.h から移動)、src/app/ofx.{h,cpp}、tests/ofx_drv.cpp (qtui_ofx_drv)、tests/dump_n3c.cpp (qtui_dump_n3c)、tests/dump_util.h、tests/fixtures/py_preset.nvpreset.json・py_profile.nvprof.json、tools/compare_ofx.py・py_ofx_drv.py・compare_n3c.py・py_dump_n3c.py。変更 src/core/export.{h,cpp} (nv_depth 部を depth.h 経由に、read_frames_fmt・read_filter(VideoInfo) を公開、L5)、src/core/nvcore.{h,cpp} (Frame に float32 面 fY/fCb/fCr と at()、frame_from_planes、noisiest_frame)、src/core/engine.cpp (Frame::at)、src/app/mainwindow.{h,cpp} (Preferences・Profile Viewer・Batch Profiler、Bit depth の読み込み・ラベル・Performance・Image information・書き出し既定、OFX の set_ofx_frames/set_ofx_mode と close フック、メニュー有効化、timers_idle)、src/main.cpp (--ofx)、tests/selftest.cpp (+37、L6)、CMakeLists.txt (2 ターゲット)、tools/compare_export.py (+5 ケース)。.gitignore (qtui/tests/*.h と fixtures を許可)。
sandbox/omo/wp_qtui_c/: PROGRESS.md、ログ (log_*.txt)、depth/ (sensitivity.py・l5_fds.py・export/・変更前の export.{h,cpp}.before)、n3c/ (py・cpp のダンプ、sbs/、log_compare_n3c.txt)、ofx/ (ダンプとログ)、regress_t/ (N3a/N3b シナリオの回帰、生ペインは削除 README あり)。

■ 質問
1. Preferences の C++ 追加 2 行と native 欄の無効表示でよいか (Python と同一の見た目を優先するなら、追加 2 行を別の場所へ移す)。
2. OFX モードの「最初の操作まで画面が空」は Python 由来の挙動として揃えたが、両方で直すべきか (repro の変更が要る)。
3. N3a/N3b の未追跡ソース (mainwindow・nvcore・engine・selftest・main・CMakeLists・compare_export) の N3c 直前の控えは export.{h,cpp} 以外にない。N3a/N3b を先にコミットしてから N3c を載せる運用にするか。

■ 未解決・注意
- 実画面での確認は未実施 (すべて offscreen)。手順: cmake -S qtui -B qtui/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build qtui/build -j 2 → qtui/build/MeatVideoQt.app/Contents/MacOS/MeatVideoQt test.mp4 で Tools > Preferences (Bit depth 10 で再読込)、Profile > Profile Viewer… (⇧⌘I)、Tools > Batch Profiler…、Generic Profile を Python 版 (cd repro && python3 -m nv_ui_qt.main ../test.mp4) と並べる。OFX: ffmpeg -i test.mp4 -frames:v 5 -f rawvideo -pix_fmt yuv420p - | MeatVideoQt --ofx 640 360 30 > out.yuv (Apply / 閉じる)。
- nv_depth の x86_64 / NV_PORTABLE 版は未照合 (nvp 演算経由なので一致する見込み = 推測)。照合の前提は従来どおり numpy 2.1.3 / scipy 1.15.3 / Python 3.13 (arm64)、Qt 6.12.0、ffmpeg 8。
- L7 (取り消し時の最後の 2 行) は未変更。
- 再現コマンド: cd repro && NV_NATIVE=1 python3 ../qtui/tools/compare_ofx.py OUT / NV_NATIVE=1 python3 qtui/tools/compare_n3c.py OUT / cd repro && QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 ../qtui/tools/compare_export.py OUT --exe ../qtui/build/qtui_export / python3 native/verify_bridge_depth.py / QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest。


---

## 追補: L-1 の修正 (統括による転記、作業者の報告全文)

L-1 を直しました。Python 版との一致を確かめ、ほかの動作は変えていません。コミットはしていません。以下で「観測」とあるのは実行した結果です。

■ 変更 (変更前の控え: sandbox/omo/wp_qtui_c/ofx/ofx.cpp.before_L1、l1/selftest.cpp.before_L1)
1. qtui/src/app/ofx.cpp
 - 1 フレームのバイト数を 64 ビットで計算し、乗算と加算のあふれを `__builtin_*_overflow` で検査するようにしました (`frame_size64`)。
 - W か H が INT_MAX を超える場合と、フレームの大きさが 64 ビットに収まらない場合は「完全なフレームが読めない」扱いです。stdin を終わりまで 1 MiB ずつ読み捨て、`no frames received on stdin` を stderr に出して終了コード 1 にします。バッファは確保しません。
 - `read_exact` は n バイトを先に確保するのをやめ、届いた分だけ 1 MiB ずつ伸ばす方式に変えました。入力より大きいフレームでも、確保量は入力の量で止まります。Python の read と同じ振る舞いです。
 - `py_int`: 64 ビットを超える 10 進整数を ±LLONG_MAX に飽和させるようにしました。以前は usage を出していましたが、Python の int には上限がないためです。
 - 範囲内の W/H の通常の経路 (int 化とフレームの組み立て) は変えていません。
2. qtui/tests/selftest.cpp: `ofxArgsAndNoFrames` に 5 ケースを足しました。L-1 の 3 例に加え、`99999999999999999999 120` と `3037000500 3037000500 24 0 --depth 32` (64 ビットを超えるフレームの大きさ) です。どれも exit 1・stdout 空・"no frames" であることを確かめます。`ofx_run` には timeout を付け、超えたら kill します。
3. qtui/tools/compare_ofx.py: 引数のケースに同じ 5 つを足しました。timeout は 60 秒で、超えた場合は TIMEOUT として記録します。

■ 結果 (観測)
- L-1 の 3 例は手動で実行し、`/usr/bin/time` で RSS と所要時間を測りました。
  - `--ofx 4294967296 120` (stdin: /dev/null と test.mp4 の両方): no frames、exit 1、0.10 秒、RSS 52 MB。
  - `--ofx 2147483648 1 24 2`: no frames、exit 1、0.09 秒。
  - `--ofx 4294967496 120 24 2`: no frames、exit 1 (以前は 200×120 として処理していた)。
- compare_ofx ALL SAME。
  - 26 ケースの stdout と窓の比較は前回と同じく一致しました。
  - 引数ケースは 13 件です。L-1 の 3 件は exit・stdout・stderr の行がすべて一致しました。
  - 追加した 2 件 (64 ビット級) は exit 1・stdout 空が一致しました。ただし stderr が違います (観測): Python は `stdin.read` で `OverflowError: cannot fit 'int' into an index-sized integer` の traceback を出し、C++ は "no frames" を出します。既知の差として扱い、compare_ofx はこれを判定して表示します。境界は、Python が Py_ssize_t (2^63−1) を超えたとき、C++ が 64 ビットのあふれと W/H が int を超えたときです。2^63 から 2^64 の間はどちらも exit 1・出力なしになります。
  - ログ: sandbox/omo/wp_qtui_c/l1/log_compare_ofx.txt。
- 検証者の ind_ofx.py の引数部分 17 件 (出力先は sandbox/omo/wp_qtui_c/l1/ind_args/、ログは l1/log_ind_ofx_args.txt) を再実行しました。
  - `arg_W_2pow32` が SAME (exit 1/1、no frames) になりました。
  - 残る DIFF は L-2 の 3 件 (fps 1e400 / 1e-400、Unicode 数字) で、記録のみとして直していません。
  - 検証者の 4 件目 (fps 1e-310 と 0x10) は、このスクリプトの引数ケースに入っていないので再実行していません。
- qtui_selftest 105 件 (103 項目) 全 PASS (ログ: l1/log_selftest.txt)。
- qtui のクリーンビルド (新しいディレクトリで cmake の構成から) 警告 0 (ログ: l1/log_qtui_clean_build.txt)。native には手を入れていません。

■ 未解決
- L-2 の 3 件と、上に書いた 64 ビットを超えるフレームの大きさでの stderr の差は、記録のみです。
