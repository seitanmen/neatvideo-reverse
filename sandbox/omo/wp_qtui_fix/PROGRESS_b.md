# PROGRESS (作業者 fix2-b: G7・G8)

- 2026-10-30 09:50 着手。worktree n3fix-b (df46dda)。libnvcore を再ビルド (警告 0)。
- 現状確認 (観測): nv_native.build_profile_v5 は探索だけ C (nvc_flat_area)、残りは Python。
  nvc_build_profile_v5 (N3a、nv_prof5) が UI の出力 dict の全キーを out_i/out_d で返す (verify_prof5.to_dict)。
  2.38 秒は N2a REPORT の「build_profile (auto)、1080p test_v02 n=312、1 スレッド」(verify_bridge --bench の行)。
- G7 の遅れの位置 (観測): AFT の UI 経路は backend.run_in_child → _run_aft → nv_native._apply_aft →
  profile_blob (nvc_profile_build = N1f の BuildNoiseModel、取り消しなし) → aft_blob (取り消しあり)。
- 10:00 G7 実装: nvm::model_full / nvp::build_profile / tile_dev に取り消しの問い合わせ (nv_model.h・nv_profile.h)、
  C ABI nvc_profile_build_cancel (戻り値 3 = 取り消し)、NVC_ABI_VERSION 2。nv_native.profile_blob / _apply_aft が cancel を渡す。
- 10:00 G8 実装: nv_native.build_profile_v5 → nvc_build_profile_v5 (FFA キャッシュは維持、再現範囲外は従来経路)。
- 10:10 G7 測定 (Python UI 経路、run_in_child、負荷 7.8〜10): 変更前の遅れ 最大 12.6 s (512²・1 スレッド) / 1.03 s (128²)。
  変更後 最大 0.168 s (24 条件)。
- 10:15 qtui の run_aft も nvc_profile_build_cancel に (CancelScope)。qtui ビルド警告 0。
- 10:16 verify_bridge に g8 群 (96 件) と cancel 群の G7 項目を追加 → g8 96/96、cancel 49/49 一致。
  verify_bridge_prof5 の参照を native OFF (Python) に変更 (G8 後は ON = C 自身になるため)。
- 10:20 nv_prof5 の高速化 (結果不変): 大きな分位点 (trim_rms) をビットのヒストグラム選択に、_dead(Y) の 3 回計算を 1 回に、
  ブロックの全面コピーをブロック単位の取り出しに。verify_prof5 630/630、HEAD の C と 246/246 同一 (非有限入力を含む)。
- 10:50 make -C native verify: 11 本すべて一致 (ビルド警告 0)。ログ scratchpad/fix2b/make_verify.log
- 11:00 verify_bridge 全群一致 (g8 96/96・cancel 49/49・export 4/4 ほか、850 s)。ログ scratchpad/fix2b/vb_full.log
- 11:08 自己試験 Python 96/96 (ON・OFF)、qtui_selftest 105/105、compare_ui (test_v02) EQUAL (差は既知の Help > About Qt のみ、書き出し 2 本バイト一致)
- 11:25 追加の高速化: 時間方向の選択を 1 回の走査に、nvn::b3 を順序固定の NEON (.4S) に。verify_spatial 1287・verify_temporal 474・verify_prof5 630・HEAD の C と 246/246 一致
- 11:30 最終コードで全検証を開始 (final.sh: ビルド・make verify・verify_bridge --export・自己試験×2・qtui_selftest・compare_ui・compare_export)
- 12:09 最終検証すべて合格: ビルド警告 0 (lib・lib_portable・qtui)、make verify 11 本一致、verify_bridge 全群一致
  (g8 96/96、cancel 49/49、export 4/4、928 s)、Python 自己試験 96/96 (ON・OFF)、qtui_selftest 105/105、
  compare_ui EQUAL (既知の About Qt のみ)、compare_export 19/19 ALL SAME。
- 12:12 verify_bridge --bench: build_profile Python 2.374 s → native 1 スレッド 1.174 s (4 スレッド 0.532 s)。
- 作業終了。変更前のファイルの控えは orig_b/。
- 12:47 F1 修正 (1 px の辺: nv_native._p5_args で Python 経路へ、nvc_build_profile_v5 / nvp5::build_profile は H,W<2 で -2)。再検証開始
- 12:56 F1 再検証: verify_bridge g8 152/152・prof5 110/110・cancel 49/49・frame 72/72、verify_prof5 630/630、自己試験 Python 96/96 (ON・OFF)・qtui 105/105、lib・qtui 警告 0。Qt C++ の build_profile は 1 px の辺で例外 (落ちない)
