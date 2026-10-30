# PROGRESS (fix2-a): G3 -> G4 -> G5 -> G6 -> G2
worktree: .claude/worktrees/n3fix-a (branch n3fix-a, base df46dda)

- 2026-10-30 開始。VERDICT_n3c.md と independent/ind_ofx.py は main ツリーにも n3a worktree にも存在しない
  (coord_n3c/independent/ui/build だけ)。L-2 のケースは wp_qtui_c/REPORT.md と l1/log_ind_ofx_args.txt から取る。
- [ ] build native lib / qtui
- [x] build (native lib, qtui build_main)。selftest 基準 105/105 (変更前)
- G3/G6 実装 (2026-10-30 午前):
  - qtui/src/core/pynum.{h,cpp} 新規 (int()/float() の Python 準拠解析、Nd 67 区間、traceback 出力)。
    ファズ (scratch pyfuzz/fuzz.py、80 万文字列 3 シード) 不一致 0
  - ofx.cpp: 引数解析を pynum に、frame_size を 128 bit、最初の read の例外 (OverflowError 2 種 / MemoryError = malloc(33+fs) の試行) を traceback で
  - ofx_frame / ofx_process を ofx.h に公開 (emit の中身は不変)
  - pyobj: py_float_str (inf/nan の str 表記) を追加し OFX のラベル・info.fps・export の sigma 表示に使用
  - nvcore.cpp: build_profile (1 px 辺で np.gradient の文言)、temporal (動きブロック未満: Python の nvfilter.temporal_v5 と同じ例外文 / 近傍なしは素通し式)
  - 比較ツール: qtui/tools/compare_ofx_args.py (50 ケース)、compare_emit_sizes.py + tests/emit_sizes.cpp (qtui_emit_sizes)
  - compare_emit_sizes 全量 4810 ジョブ ALL SAME (ログ a/logs/)、変異 (rv×1.5) で 440 件検出
  - 変更前の控え: sandbox/omo/wp_qtui_fix/a/before/
- G3/G6 検証: compare_ofx_args 50/50 SAME (a/logs/log_compare_ofx_args.txt)、compare_ofx ALL SAME (a/logs/log_compare_ofx.txt)
- G5: selftest に init() + ensure_window・make_v10・ensure_v10_at_10・show_filtered_adjust・build_rect_profile・ensure_nlc_on。
  qtui/tools/selftest_each.py で単独実行 (1 回目 95/105 → 修正後 10/10)
- G4: #7 #9 #96 を Python 同等に強化、項目 20 = nlcPanelYellowAndDisabled 新規、pc_cached() (mainwindow) 追加。
  全体 108/108 PASS。デモ a/g4_demo/ (旧 PASS / 新 FAIL を 3 件とも確認、ソース復元済み)
- 次: G2 (ffmpeg 欄)、最後に共通 4 検証
- G2 実装: repro/nv_ui_qt/ffmpeg_path.py 新規 (探索・PATH 前置・文言)、main.py 起動時 setup + 未検出ダイアログ、packaging/main.py は共通化、
  両 UI の Preferences に AFT 行 (Python 新規) と ffmpeg 行 (欄 + Browse… + 無効パス警告で閉じない)、両自己試験に項目追加
- Python 自己試験: NV_NATIVE=1 97/97 (exit 0)、NV_NATIVE=0 97/97 ×4 (うち 2 回は全 PASS 出力後に終了時ハング)。
  HEAD (git archive) でも NV_NATIVE=0 の終了時ハングを 1/2 回で再現 → 既存問題 (viewer の ThreadPoolExecutor が
  display.hist_levels の np.dot (OpenBLAS exec_blas) で止まり、終了時の join が戻らない)。ログ a/logs/log_py_selftest_native0_HEAD_exit_hang.txt
- 2026-10-30 最終検証 (すべて完了):
  Python 自己試験 97/97 (NV_NATIVE=1・0 とも exit 0、a/logs/log_py_selftest_native{1,0}.txt)、qtui_selftest 110/110 ×2、
  selftest_each 108/108、make -C native verify 11 本 ALL BIT-EXACT、verify_bridge ALL MATCH (687 s)、
  compare_export 19/19 (ON/OFF)、compare_n3c EQUAL (Preferences 11 行一致、差は native 欄のみ)、test_launcher 12/12、
  qtui クリーンビルド警告 0。追跡済み pyc 2 件は HEAD の内容に戻した。
- 注意: repro/nv_ui_qt/ffmpeg_path.py は .gitignore の repro/** に掛かる (マージ時 git add -f が必要)
- 2026-10-30 検証指摘 (coord_fix2a/VERDICT_g2_6.md) への対応: L2 行番号 (main.py 157/34/73・ofx_bridge 110/39、Python 側に注記行、
  compare_ofx_args に traceback のフレーム照合と実 python3 -m の照合、変異で検出)、I5 (selftest_each の 0 件・未知名で失敗)、
  I3 (再現 a/i3/log_before_fix.txt → ffmpeg_path の search_path で修正 log_after_fix.txt、test_launcher 13/13)、L1/L3 は REPORT の記述。
  再確認: qtui_selftest 110/110、compare_ofx_args 50/50 + 実 python 一致、compare_emit_sizes 4810 一致、Python 自己試験 ON 97/97
