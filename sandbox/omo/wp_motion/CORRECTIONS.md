# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_motion/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:15、MERGE_PLAN.md:27、notes/decode_motion.md:93・:183 | w = clamp(1 − 0.66·d, 0, 1) | d = f32(1/(thr+0.01f))·(float)(SAD−best)、d < 1.5151515f の近傍だけ w = (d·−0.66f)+1.0f (2 回丸め、clamp なし。基準位置だけ +0.0f)。REPORT.md:113・notes:203 の記述が正しい (VERDICT:9-10) |
| REPORT.md §3.2 (:52-61) | 表の全行が修正後の cmp_intervals.py の出力 | 区間 B の全行と区間 A の変種行は、repro/ を読んでいた 10-17 の旧スクリプトでの測定。修正後に測り直したのは区間 A の v5・nvm の 2 行 (Y 47.117 / 46.609) だけ。base/nv_band.py は repro 側より古い版で、影響は評価していない (VERDICT:33-34) |
| MERGE_PLAN.md:38 | 未検出の 2 種は打切りの < → <= で等価 | cut_le は等価 (d = cut で w = 0)、cut_double は未完走。打切りの向きは逆アセンブル (b.pl) だけで確認 (VERDICT:30・:42) |
| MERGE_PLAN.md:9 | 15,058 件でビット一致 | ブロックが面の内側の速い経路のみ。端の遅い経路・NaN の閾値・elem 4 は未照合 |
| REPORT.md:5・:90-91・:96-97、notes:172・:186 | 本家の呼び出しは selftest のみ、31/31 変更なし、承認待ち、288 件全件一致 | nvh sad・b32 は承認済みで 15,058 件照合。読み取り専用ファイルは §5.5 のとおり 9 件が異なっていた (repro/ は書き換わっていないことを統括が確認)。10-17 の 288 件の時点では 1 ulp のずれがあった (notes §8) |
| REPORT.md:34 | selftest_motion.py の SHA 21117834… | 現物は 39c92476…125d |
