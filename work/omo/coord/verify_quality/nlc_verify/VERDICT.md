# 検証: Noise Level Check / Temporal Noise Level (SPEC R1〜R6、2026-10-26)

独立検証者 (サブエージェント) の最終応答を、統括が要約して転記した。判定は **APPROVE** (阻害要因 0、軽微 2、参考 2)。

## 確認したこと (検証者の測定)
- 自己試験 77/77 (`st_run.log`)。py_compile と pyflakes はエラー 0。読み取り専用のスナップショットは 181/181 で、nv_aft_v5.py の更新時刻は 15:47:20 のまま。
- R1 の t_noise = 100%: 変更前の控えを別名のモジュールとして読み込んで比べ、ビット一致した (`v_r1.py`)。
  - 8 設定 × 3 フレーム × 2 本 (test.mp4・test_v02) で、`process_frame`・`_work`・`PreviewEngine.process` (全面と切り出し) を比べた。
- R1 の t_noise 0・40・137・250%: プレビュー・`process_frame`・`_work` が参照 (時間段だけ noise_level×t_noise) とビット一致した。
  - 時間段のキャッシュの鍵に t_noise が入っている。
  - 書き出し (CRF 0、−40%) と OFX の Apply (−60%) も `process_frame` と一致した。
- R2: m の式は float64 の独立版との差が 1e-5 未満で、[0,1] に収まり NaN はない。Amount 0・Radius 0 では全面 0。
  - 同じフレームを繰り返すと、test.mp4 で最小 1.0、test_v02 で平均 0.9999996 (最小 0.94。動き推定のサブ画素のずれによる)。
  - 色の式は O2 と最大誤差 3.7e-5。
- 状態の管理は 19/19 (`v_state.py`)。
  - フレームを続けて替えても、別フレームのマスクが出た回数は 0/232。
  - 排他、終了、無効化、Begin 直後の Finish も正しく動いた。
- 旧実装 (`nv_aft_v5.assist_params` を UI から呼ぶ経路、`_nlc_params`、`noise_level_check`、`GUIDES["noise"]`) は残っていない。
- OFX モードの往復は正常 (`v_ofx.py`)。

## 指摘
- **L1 (軽微):** 設定を変えたあと新しいマスクができるまで (約 100 ms + 計算時間)、古い設定のマスクが表示されたまま (232 回中 126 回)。フレームが違う画像は 0 回。Profile Check と同じ設計。
- **L2 (軽微):** Begin 中に無効になった見出し (Repeated Frames・Jitter of Details・Dust and Scratches) の文字が白いまま。`rows.Header` がタイトルの色を明示しているため。本家では灰色。
- **I1 (参考):** Begin 中に表示モードを変えると、成分のペインにもマスク画像が出る (repro 側の選択)。
- **I2 (参考):** 自己試験の NLC (b) の条件 `min > 0.999` は test.mp4 でだけ成り立つ (test_v02 は最小 0.94)。

本家との比較 (test_v02、別フレーム、平均 m): +0% は repro 0.987 / 本家 0.996、−60% は 0.524 / 0.33、−100% は 0.0000 / 0.003。
