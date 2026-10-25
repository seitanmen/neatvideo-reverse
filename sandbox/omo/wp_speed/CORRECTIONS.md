# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_speed/indep/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| src/nv_model_special.py:468 (2026-10-20 まで) | U8 = image_u8(A) を 5 パスのループの外へ移す | A は各パスの最後の pattern_remove で変わるので不可。本家の結果と 232/753 件で不一致 (組 4 の特殊語 1306〜1750)。統括がループ内に戻した版 (`coord/verify_speed/src_fix/`) を repro/ にマージし、src もこれと同じにした (2026-10-20) |
| MERGE_PLAN.md:15-16・:19 | 「ad319c の U8 計算のループ外移動」「すべて参照版とビット一致」「上記 5 ファイルを repro/ に上書き」 | U8 の項は誤り。マージ元は `coord/verify_speed/src_fix/` (統括の 1 行修正込み) |
| REPORT.md:5・:22・:41・:60-61 | 「全件ビット一致を維持」「最終コードで実行」、最終数値の欄が空 | 最終版は上の不具合があった。修正版は verify_model_cases 753/753・verify_profile 76/76 (ポインタ 6 語以外一致)、独立検証で model_full 154 面・1,261,568 語一致。時間は exp_results.txt の speed 節 |
