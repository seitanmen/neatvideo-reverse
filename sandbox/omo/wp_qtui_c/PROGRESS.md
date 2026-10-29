# N3c 進捗 (節目ごとに更新)

## 2026-10-29 開始
- 読んだもの: MIGRATION.md の知見、native_n3_qtui.md、N3a/N3b の REPORT・VERDICT、Python UI (main_window・menus・main・ofx_bridge・backend・nv_depth・selftest)、qtui の mainwindow / nvcore / engine / export / video。
- 分担: 分担作業者 `n3c-depth` (fork) = M2 (nv_depth の数値を native/nv_depth + nvcore_c_depth へ、verify_bridge の depth 群)、L5 (CLOEXEC)、
  `qtui/src/core/depth.{h,cpp}`・`export.{h,cpp}` (read_frames_fmt を公開)。私 = UI 側 (Preferences・Profile Viewer・Batch Profiler・OFX モード・高ビット深度の読み込み・
  selftest・dump/compare_ui・L6・メニュー)。
- 次の一手: fork を起動 → Frame の float 化 (nvcore.h)・Preferences・Profile Viewer。

## 11:10 (fork は使わず単独で進める: 名前付きの fork を作れなかった)
- 済: M2 (native/nv_depth.{h,cpp}・nvcore_c_depth.{h,cpp}、verify_bridge_depth.py 208,737,760 語一致 + 感度試験 4/4 検出、verify_bridge.py に depth 群)、
  qtui/src/core/depth.{h,cpp} (PixFmt を export.h から移動)、export.cpp は depth.h 経由、read_frames_fmt / read_filter 公開。
  compare_export 19/19 ALL SAME (N3c で 4:2:2 prores/ffv1・float RGB NaN/inf・EXR 連番・gbrp10 を追加)。L5 (CLOEXEC、lsof で Python と同じ fd)。
  L6 (fixtures → qtui/tests/fixtures、.gitignore で許可)。Frame の float 化、open_video の Bit depth 対応、Preferences・Profile Viewer・Batch Profiler、
  OFX モード (ofx.{h,cpp}、qtui_ofx_drv、py_ofx_drv.py、compare_ofx.py: 27 ケース stdout 全一致)。
- 残り: selftest の N3c 項目と 96 項目対応表、compare_ui の拡張 (新画面の値と grab、Bit depth 10 の読み込み)、メニュー 114 の有効/無効、
  make -C native verify 11 本、verify_bridge (quick)、verify_bridge_ui、クリーンビルド警告 0、REPORT (最終メッセージ)。

## 13:00 完了
- selftest 103 項目 (34 + 32 + 37) 全 PASS (105 件、3 回連続)。compare_n3c EQUAL (既知の差: Preferences の C++ 追加 2 行と native 欄、About Qt)、
  compare_ofx ALL SAME (26 ケース + 窓 2 深度 × 2 状態 + 引数 8)、compare_export 19/19、N3a/N3b の compare_ui 回帰 (test.mp4) EQUAL、
  make -C native verify 11 本全一致 (2 回に分けて実行)、verify_bridge --quick ALL MATCH (324 s)、verify_bridge_ui 34,755/34,755、
  verify_bridge_depth 208,737,760 語一致、native / qtui のクリーンビルド警告 0。
- REPORT は最終メッセージに全文 (作業者の報告ファイルは書かない規約)。
