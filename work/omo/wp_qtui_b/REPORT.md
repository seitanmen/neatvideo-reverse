# N3b REPORT: Qt 6 C++ UI の Adjust and Preview ページと書き出し (2026-10-29)

作業場所は worktree n3a (ブランチ n3a-qtui、N3a の未コミットの変更の上に積んだ)。コミットはしていない。AGENTS.md・MIGRATION.md・スナップショット・repro/・repro/out/・本家バイナリには触れていない。「観測」= 実行して得た数値、「推測」= コードを読んでの判断。特記のないものは観測。進捗の記録は `sandbox/omo/wp_qtui_b/PROGRESS.md`、書き出しエンジンの詳細は分担作業者の `sandbox/omo/wp_qtui_b/export/REPORT_export.md`。

## 分担
- 本体 (私): nvcore_c の追加関数と照合、engine の時間段の分離、FilterPanel、mainwindow の統合 (Noise Level Check・Performance・Check Speed・書き出しの呼び出し)、selftest、dump / py_dump / compare_ui の Adjust 拡張、N3a の指摘 M1 / L1 / L2 / L4。
- 分担作業者 (fork、export 担当、担当ファイルを分けた): `qtui/src/core/export.cpp`・`qtui/tests/export_cli.cpp`・`qtui/tools/compare_export.py` (`export.h` のインターフェースは私が先に決めた)。

## 1. 変更・新規ファイル
### native/ の影響範囲 (W2a との突き合わせ用)
- N3b の変更: `nv_ui.h/.cpp` (末尾に `temporal_mask`・`nlc_image`・`np_interp`)、`nvcore_c_ui.h/.cpp` (`nvc_temporal_mask`・`nvc_nlc_image`・`nvc_interp`・`nvc_mean_f32`)、`nv_prof5.h/.cpp` (公開の `mean_f32` 包み、L1 の注記)、`nv_lstsq.h` (L2: 対応範囲の記述の訂正。コード不変)、`verify_bridge_ui.py` (`nlc`・`interp` のケース)。既存関数の挙動は不変、ABI 版 1 のまま、Makefile は N3a のまま。
- N3a からの変更 (N3b では触っていない): `Makefile`・`nv_curve.*`・`verify_bridge.py` と新規 `nv_ui.*`・`nvcore_c_ui.*`・`nv_prof5.*`・`nv_lstsq.*`・`nvcore_c_prof5.*`・`verify_bridge_ui.py`・`verify_prof5.py`・`verify_bridge_prof5.py`。
### qtui/
- 新規: `src/widgets/filter_panel.{h,cpp}`、`src/core/export.{h,cpp}` (.h 私、.cpp export 担当)、`tests/export_cli.cpp` (`qtui_export`)、`tools/compare_export.py`。
- 変更: `src/app/mainwindow.{h,cpp}` (FilterPanelStub → FilterPanel、Noise Level Check、Performance 表、Check Speed、Performance tab、Reset Spatial、書き出し (process_video / cancel / closeEvent の後始末)、メニュー Export / Performance tab / Noise Level Check を有効化)、`src/core/engine.{h,cpp}` (`temporal_stage` 分離と `mask()`)、`src/core/nvcore.{h,cpp}` (`nlc_image`・`mean_f32` の包み)、`src/core/state.cpp` (M1: `auto_complete` を `nvc_interp` 経由に)、`src/app/viewer.h` (試験用 `image(src)` アクセサ)、`src/core/video.{h,cpp}` (`tool_path` 追加、export 担当)、`tests/selftest.cpp` (+32 項目、L4)、`tests/dump.cpp`・`tools/py_dump.py`・`tools/compare_ui.py` (Adjust ページの比較)、`CMakeLists.txt` (`qtui_export`)。
### sandbox/omo/wp_qtui_b/
`PROGRESS.md`、`export/` (export 担当の REPORT・ログ・テストクリップ・出力)、`cmp/` (両 UI のダンプと並置画像 `sbs_t/`・`sbs_v02/` 各 16 枚。1080p の生ペイン画像 2×1.3 GB は比較後に削除、`README_v02_panes.txt`)、`compare_t.txt`・`compare_v02.txt`、`fixtures/` (Python 版 UI が書いたプリセット・プロファイル、selftest が読む)、`log_native_verify.txt`・`log_verify_bridge_ui.txt`・`log_verify_bridge_quick.txt`・`log_compare_export_rerun.txt`。

## 2. 合格条件ごとの結果
### 2.1 書き出しの一致 (Python 版 NV_NATIVE=1 とバイト一致)
(a) UI の操作列経由 (`qtui_dump` / `py_dump.py` の adjust_steps: 開く → Auto Profile+AFT → 選択 → Build Profile+AFT → Adjust で編集 → Reset Filter → `process_video`):
| 入力 | 操作 | 結果 |
|---|---|---|
| test.mp4 | export_a: 編集後 (amt_Y 50、Mid 60 の共通行、bamt_Cr_low 30、Mix 有効 25、t_noise 60、t_radius 1 → undo 2 / redo 1)、CRF 0、start 300、4 f | バイト一致 818,365 B。ui_params / ui_profile 文字列同一、side file (seconds 以外) 同一 |
| test.mp4 | export_b: Reset Filter 後の既定、CRF 16、12 f | バイト一致 159,549 B |
| test_v02.mp4 | export_a (同上の編集)、CRF 0、4 f | バイト一致 3,960,307 B |
| test_v02.mp4 | export_b (既定)、CRF 16、12 f | バイト一致 484,499 B |
(b) エンジン単体 (`compare_export.py`、私の build `qtui/build/qtui_export` で再実行 `log_compare_export_rerun.txt`、14/14 ALL SAME): test.mp4 / test_v02 既定 CRF 0 4 f バイト一致 (512,082 / 2,447,586 B)、変則 3 種 (成分別 bamt_Cr_medium 60+bamt_Cb_low 30+amt_Y 50+bnoise_Y_high 130 / Mix 25+noise_level 150+threshold 3+t_noise 60 / radius 1+spatial off+noise_Cb 150) バイト一致、音声あり (test.mp4+正弦波 AAC クリップ) CRF 16 audio copy 24 f バイト一致 324,556 B (-ss/-itsoffset/-copypriorss 経路)、MPEG-TS mp2→aac (discont) バイト一致、audio aac / none バイト一致、範囲なし 180 f バイト一致、`--depth 10` FFV1 .mkv は復号フレーム (framemd5) + ファイル長一致 674,147 B、追加で 12 ビット x265 mp4 バイト一致、16 ビット / 32 ビット float FFV1 mkv 復号一致。全ケースで ffmpeg/ffprobe のコマンド列が Python と語単位で同一。
(c) selftest: `exportLosslessEqualsProcessFrame` (成分別、start 100、4 f の復号 = temporal+finish の clip(rint) とバイト一致、ui_params/ui_profile の内容 = state)、`exportTNoiseDiffersFromDefault`、`exportCancelRemovesFiles` (取り消しで mp4・json・ui_params・ui_profile が残らない)。

### 2.2 Adjust ページの表示 (compare_ui、`compare_t.txt` test.mp4 155 f 選択 (40,60,128,160) / `compare_v02.txt` test_v02 86 f 選択 (0,832,128,128))
| 比べたもの | test.mp4 | test_v02 |
|---|---|---|
| 状態 5 つ × 19 項目 + プロファイル JSON | 一致 | 一致 |
| Filter Settings の値 10 状態 × 143 項目 (全行の表示文字列・enabled・hidden・reset・スライダー位置、共通 Frequencies 行 10、ヘッダ 30 の文字列と折り畳み、タブ有効、Begin/Finish、ツールバー文言、Performance HTML、プリセット名、undo index/文言、サムネイル radius、params) | 一致 | 一致 |
| ペイン画像 (7 モード×明るさ 3×Prepare/Profile Check/Adjust、Preview Area、編集後、Noise Level Check のマスク 2 枚 (t_noise +0%/−60%)、サムネイル 3 枚) | 232/232 バイト一致 (52,768,512 画素) | 232/232 (474,861,312 画素) |
| 窓 grab 13 枚 (Prepare 3、Profile Check、Adjust、Preview Area、Spatial/General タブ、Beginner、編集後、Noise Level Check、Performance) + ビューア 3 + タイムライン | 16/16 差 0 画素 | 16/16 差 0 画素 |
| 保存したプリセット/プロファイル、ui_params/ui_profile | 文字列同一 | 同一 |
| メニュー 114 | 差 4 = N3c の 3 項目 (Profile Viewer / Batch Profiler / Preferences) 無効 + About Qt | 同左 |

### 2.3 Undo/Redo と JSON
dump の編集 7 手 → undo 2 → redo 1 の各状態が一致。selftest: ドラッグ 1 手併合 (`sliderDragMergesUndo`)、Frequencies 行 3 成分 1 手 + "mixed" (`frequenciesRowWritesThree`)、Mix Enable は mix 0 なら 25 と 1 手 (`mixEnableStartsAt25`)、Reset Spatial (`resetSpatialOnlySpatialKeys`)、履歴メニュー (`undoHistoryMenu`)、直接入力と ⋈ (`exactValueEntryAndReset`)。Python 版が書いた `fixtures/py_preset.nvpreset.json`・`py_profile.nvprof.json` を C++ 版で読み再保存した文字列がもとと同一 (`presetProfileFilesFromPython`)。

### 2.4 selftest
`qtui_selftest` 66 項目 (34 + 32) 全 PASS (init/cleanup 込み 68 件、約 7.5 秒)。N3b 32 項目: filterPanelLayout、defaultValuesShown、sliderDragMergesUndo、frequenciesRowWritesThree、mixEnableStartsAt25、presetDirtyStarAndReset、resetSpatialOnlySpatialKeys、beginnerHidesAdvancedRows、bandAvailability420、performanceTable、checkSpeed、performanceTabAction、menusEnabledForN3b、tNoiseScalesTemporalOnly、temporalMaskEdges、nlcBeginShowsMask、nlcRecomputesOnTemporalSettings、nlcAnotherFrameDropsImage、nlcThreeEntriesAndAssist、nlcEndsOnPrepareAndWithoutProfile、exportFilterDefaults、exportLosslessEqualsProcessFrame、exportTNoiseDiffersFromDefault、exportCancelRemovesFiles、exportWithoutProfileOrVideo、presetProfileFilesFromPython、applyButtonRefreshesPreview、autoCompleteThroughCore、thumbnailWindowFollowsRadius、exactValueEntryAndReset、choiceAndEnableRows、undoHistoryMenu。L4: `buildProfileSizeRules` 末尾を `profile_sig()` 前後比較に。

### 2.5 照合とビルド
| 照合 | 結果 |
|---|---|
| `verify_bridge_ui.py` 全量 (nlc 526+520、interp 8,016+7,918 を含む) | 34,755/34,755 |
| `verify_bridge.py --quick` | ALL MATCH (curve 12/12・profile 7/7・aft 11/11・frame 32/32・cancel 23/23・fallback 9/9・prof5・ui、327 s) |
| `make -C native verify` 11 本 | 全件 ALL BIT-EXACT、exit 0 |
| qtui クリーンビルド (Release) | 警告 0 |

## 3. 仕様 (Python 版) との相違点
- 書き出しは別プロセスでなく同プロセスのスレッド (`ExportJob`)、フレーム並列はスレッドプール (nvcore カーネルに可変の静的状態なし)。取り消しは cancel フラグ + spawn した ffmpeg への SIGKILL。部分出力の削除は Python UI 同様 mainwindow 側。`<stem>.json` の `seconds` は実測。`--flicker` 未実装 (UI から未使用)。
- UI は N3a のまま 8 ビット (yuv420p) のみ読む。`depth_used` は "8" 固定、保存ダイアログ既定は MP4。Preferences・Profile Viewer・Batch Profiler は N3c (メニュー無効、パネルの Preferences... はステータスバーに N3c と表示)。書き出しの jobs/crf/audio は設定ファイルの prefs/* (Python 版と共有)。
- Performance 表の Frame Bitdepth は読み込み済みなら "8-bit (yuv420p)" 固定 (8 ビットでは Python と同じ文字列)。
- Noise Level Check のマスク平均 (ステータス) は `nvc_mean_f32` (numpy float32 pairwise 和) で両 UI 同じ文字列。
- `qtui_export` は `settings()` (mainwindow.h) で ffmpeg の探索設定を読む。

## 4. 質問
1. 1080p ダンプの生ペイン画像 (2×1.3 GB) は比較後に削除した。検証者が再生成する前提でよいか (手順 §6)。
2. `qtui_export` が `settings()` に依存する点はこのままでよいか。
3. N3a の質問 (設定ファイル共有、polyfit の OpenBLAS 固定) は未回答のまま前提を変えていない。

## 5. 再現手順
```
cmake -S qtui -B qtui/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build qtui/build -j 2 ; make -C native lib
QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest
W=$PWD; QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/py_dump.py $W/test.mp4 $W/OUT_P 40,60,128,160 0.05   # 絶対パスで (Python 版の書き出しは repro/ を cwd にする)
W=$PWD; QT_QPA_PLATFORM=offscreen qtui/build/qtui_dump $W/test.mp4 $W/OUT_C 40,60,128,160 0.05
python3 qtui/tools/compare_ui.py OUT_P OUT_C SBS_DIR
cd repro && QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 ../qtui/tools/compare_export.py OUT_E --exe ../qtui/build/qtui_export
cd native && QT_QPA_PLATFORM=offscreen python3 verify_bridge_ui.py --jobs 2 && python3 verify_bridge.py --quick && make verify
```
実画面の確認 (未実施): `qtui/build/MeatVideoQt.app/Contents/MacOS/MeatVideoQt test_v02.mp4` で起動し、Adjust and Preview で Filter Settings、Temporal タブの Noise Level Check Begin、General タブの Check Speed、Effect > Export Full Video... を Python 版 (`cd repro && python3 -m nv_ui_qt.main ../test_v02.mp4`) と並べて見る。

## 6. 未解決・注意
- 照合の前提: numpy 2.1.3 / scipy 1.15.3 / Python 3.13 / OpenBLAS 0.3.29 (arm64)、Qt 6.12.0、ffmpeg 8。
- 書き出しエンジンの未照合経路: prores、EXR 連番、4:2:2 widen/戻し、gbrp 整数 RGB、NaN/inf の sanitize/restore、奇数サイズ crop、回転タグ、音声 2 本以上、音声が copy も aac も不可の警告経路 (コードは Python を写したが照合は 14 ケースのみ)。
- L1 は `nv_prof5.h` に注記 (挙動不変)。L3 (`on_area` が 512 超を切り詰めない) は Python 版と同じまま。
- Q_OBJECT ヘッダ追加で cmake の再構成が必要だった (N3a と同じ)。
- REPORT.md 本体はハーネスの制約で書けなかった (PROGRESS.md に記載)。
