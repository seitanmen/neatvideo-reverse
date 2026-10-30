# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_temporal/VERDICT.md、coord/verify_integrate/VERDICT.md:32-38`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:28-29、notes/reverse_temporal.md:35-46・:90、MERGE_PLAN.md:19 | σ 構造体は 9a84f8 のスタック local_190 | インスタンス +0x17330 (0x9a8a20〜0x9a8a2c、99a76c の第 9 引数) = プロファイル複写 +0x114e8 の変換面 3 (モード 1 では Y) のモデル面ブロック。σ_k は語 5+k (q99)。src:21 と REPORT_20261018.md は正しい |
| REPORT.md:52、MERGE_PLAN.md:22、notes:129 | FUN_009efb1c(c, a, b, cc, out, n) → c·cc·(max−b) | FUN_009efb1c(a, b, c, out, n): x0..x3 がポインタ、x4 が n、浮動小数点の引数なし。out = c·(max(a,b) − b) |
| REPORT_20261018.md:9、src:12・:193・:205 (2026-10-20 にコメントだけ直した) | 補正は順序なしを含む / fcsel hi は順序なしで偽 | hi は順序なしで成立するので順序なしでは補正しない。slope が NaN になるので出力は同じ (VERDICT:9)。コードの keep は出力等価なので変えていない |
| REPORT_20261018.md:10、src:16・:242 | wmul の端数部で NaN の選び方が違う | 命令のオペランド順は違うが、端数部の ramp は NaN を 0 にするので出力には現れない (VERDICT:11) |
| REPORT.md:6・:63・:66・:73-75、MERGE_PLAN.md:3、notes:148 | 本家との照合 0 件、SHA 0a22e1f6…、近似 (双線形など) | 葉 4 関数は本家とビット一致 (実装者 4,031 件・検証者 3,186 件)。前任の GLM 版は ramp 107・wmul 20・pairw 12・down2 189 件で不一致。SHA と近似の記述は REPORT_20261018.md が現行 |
