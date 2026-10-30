# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_aft/VERDICT.md、coord/verify_aft2/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:59-66・:117・:126-128、MERGE_PLAN.md:14・:54-55、notes/AutoFineTune.md:6 | 本家との照合 0 件、aftshuffle/aftpoly は審査待ち | aftshuffle は採用し、移植と 194 件一致。aftpoly は却下 (harness_response.md)。それ以外は本家と未照合 |
| REPORT.md:68 (§5)・:73 | 150 PASS / 1 FAIL、空振り確認 | 旧版の自己試験。現行は §11.3 の 178 PASS (aft_port では 177)。旧試験は誤った読みを固定していた (verify_aft VERDICT:56-68) |
| REPORT.md:87-88 (§6) | 曲線の末尾 0.764 / 0.765 | 修正前のビン 8 の値。修正後は 0.7436 / 0.7466 (§11.5) |
| REPORT.md:174・:187、MERGE_PLAN.md:10・:39 | 誤り注入 12 種すべて検出 | 作者の 12 種は独立に再実行していない。第 2 検証者の 6 種のうち 3 種 (上側外挿の閾値、位相 2 の旗の <=、CV 許容に s_lvl) は未検出 (verify_aft2 VERDICT:56) |
| REPORT.md:157 | 4 か所目は見つからなかった | 移植の書き込み箇所は 3 か所 (curve_post の位相 2・3、curve_fill.write。9bdf30 の 8 か所はすべて write() 経由) (verify_aft2 VERDICT:12・:28・:57) |
| MERGE_PLAN.md:33 | 変わったのは AFT 曲線のビン 8 だけ | レベル値で変わったのはビン 8 だけ。段係数は全ビンで 0 → 1.0f に変わる (verify_aft2 VERDICT:47) |
