# 閉じてもアプリが終了しない問題の修正 — 独立検証 (2026-10-26)

検証者の最終応答から、統括が要約を転記した。

- 修正: `repro/nv_ui_qt/widgets/filter_panel.py:145` に `h.add(nl)` を 1 行加えた。変更前のファイルは `sandbox/omo/coord/backup/filter_panel.py.20261026_pre_nlcheck`。
- 判定: **APPROVE** (ブロッカー 0)

## 観測

- `quit_check.py`:
  - 修正前は、Beginner・Advanced とも親のない BeginRow が見え、閉じてから 8 秒で TIMEOUT になった。
  - 修正後は、親のない部品は見えず、`exec returned rc=0 after 6.0s`。
- 全部品の走査 (`scan*.py`、187 状態):
  - 修正前は、Advanced のすべての状態で BeginRow が見えていた。
  - 修正後は、どのモードでも見える親なしの部品は 0 個。窓扱いの部品は QMenu と QFrame だけで、どれも表示されていない。
- 行の位置: Advanced の「Temporal Noise」欄の先頭 (Noise Level の前) にあり、Beginner では隠れる。
- Begin ボタン: noiseLevelCheck が True と False を出し、`_nlc_active` もそれに従う。
- スクリーンショット: `shot_window_adv_temporal.png`・`shot_tabs_adv_temporal.png`。
- selftest: 55/55。
- OFX の往復 (8/16/32 ビットと非有限値の場合): 結果は前と同じで、閉じるとプロセスが終わる。

## 低優先の潜在要因 (今回の不具合とは無関係)

`widgets/rows.py:115` の `self.deco = QLabel()` は、deco がない行ではレイアウトに入らず、親のない非表示のラベルが 22 個残る。いずれ表示すれば同じ問題になり得る。
