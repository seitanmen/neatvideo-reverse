# N3b 進捗 (節目ごとに更新)

## 2026-10-29 09:15 再開
- 済: export.h のインターフェース定義、CMake に qtui_export、分担作業者 (export 担当) が export.cpp / export_cli.cpp / compare_export.py を完成
  (compare_export 14/14 一致。REPORT は export/REPORT_export.md)。qtui/build はスタブ時点でビルド済み。
- 残り: (A) nvcore_c_ui に temporal_mask / nlc_image / mean / interp を追加 + verify_bridge_ui のケース、
  (B) engine の temporal_stage 分離と mask()、(C) FilterPanel (C++)、(D) mainwindow 統合 (NLC・書き出し・Performance・Check Speed・メニュー有効化)、
  (E) selftest +30、(F) dump/py_dump/compare_ui の Adjust 拡張と比較、(G) L2/L4、(H) REPORT.md。
- 次の一手: (A)

## 09:30 (A)(B)(C)(D) 済
- nvcore_c_ui に nvc_temporal_mask / nvc_nlc_image / nvc_interp / nvc_mean_f32 を追加 (verify_bridge_ui の nlc/interp ケース一致)。
- engine.temporal_stage/mask、state.auto_complete を nvc_interp 経由に (M1)、FilterPanel (C++)、mainwindow に NLC・書き出し・Performance・Check Speed を統合。
- ビルド警告 0、既存 selftest 36/36、compare_ui (test.mp4) で win_adjust.png を含め全 grab 0 画素差。
- 次: dump/py_dump/compare_ui の Adjust 拡張 (filter 値・NLC・サムネイル・Beginner・編集/undo・書き出し)、selftest +30、L2/L4、REPORT。

## 09:55 (E)(F)(G) 済
- selftest 66 項目 (34 + 32) 全 PASS。compare_ui: test.mp4・test_v02 とも Adjust 拡張 (filter 値 10 状態・NLC 画像・サムネイル・Beginner・編集/undo・書き出し a/b) で EQUAL (grab 全 16 枚 0 画素差、書き出し mp4 バイト一致)。
- L2 (nv_lstsq.h)・L4 (profile_sig 比較)・L1 (nv_prof5.h 注記) 済。M1 (auto_complete → nvc_interp) 済。
- 実行中: make -C native verify (log_native_verify.txt)。次: verify_bridge_ui 全量、verify_bridge --quick、compare_export の再実行、REPORT.md。

## 10:05
- REPORT.md の書き込みはハーネスに拒否された (作業者の報告ファイル禁止) → 統括の指示どおり最終メッセージに全文を含める。
- 残り: make -C native verify (実行中)、verify_bridge_ui 全量 (実行中)、その後 compare_export の再実行 (qtui/build)、verify_bridge --quick、compare_t の再実行。

## 10:25 完了
- make -C native verify 11 本 ALL BIT-EXACT、verify_bridge_ui 全量 34,755/34,755、verify_bridge --quick ALL MATCH (327 s)、
  compare_export 再実行 (qtui/build) 14/14 ALL SAME、compare_t 再実行 EQUAL。最終報告へ。
