# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_integrate/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:61・:69-71・:80-81・:85・:89、MERGE_PLAN.md:3-4・:29-31・:41-44・:48 | (b) 本家の領域モデルの帯域比は baseline より Y −0.28/−0.08 dB | src/nvfilter.py:800-801 で a-trous 尺度の tot を q99 尺度の sig[0] で割っていた (tot_ratio が 1.0 に張り付き σ が −33%)。尺度を揃えると区間 A で 47.118/50.996/51.203 で baseline と同等。(c)(d) と合成ベンチの (b)(c)(d) は同じ欠陥を含むので比較に使わない。(a) の結論 (色差は不可) は有効 (VERDICT:10-11・:23) |
| REPORT.md:38-39・:111-112・:130-131、MERGE_PLAN.md:59-60 | σ 構造体の所在は未決着 | 決着した。インスタンス +0x17330 (プロファイル複写の変換面 3 のモデル面ブロック)、σ_k は語 5+k。スタック配列説は否定 (VERDICT:32-38) |
| REPORT.md:80・:82・:97、MERGE_PLAN.md:31・:49・:52、verify/nvh_integration_f312.py | フレーム 312 | 読んでいるのは test_v02_296.yuv の先頭、つまり n=296 (VERDICT:29)。照合の結論には影響しない |
