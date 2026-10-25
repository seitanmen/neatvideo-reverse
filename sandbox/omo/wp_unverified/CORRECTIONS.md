# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_unverified/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:34-46・:112-114、MERGE_PLAN.md:6-11 | 状態 5 でも 7 バッファは 0xaed9b4 で解放され、漏れない | 0xaed9b4 は曲線用オブジェクトの 2 本の配列を delete[] するだけ。7 バッファは 0x9b2620〜0x9b2658 の FUN_009d7910 だけで解放され、状態 5 の入口 0x9b1d88・0x9b1f5c はそこを飛ばす。AGENTS.md §6.5 の記述が正しい (VERDICT:27-40) |
| REPORT.md:57-64・:119-120、MERGE_PLAN.md:28-30 | SIGBUS は FUN_009f15b4 が 2 行を無条件に読むため | FUN_009f15b4 に高さの引数はなく、呼び出し元 0x99576c/0x995770 が行数 < 2 で呼ばない。原因は不明 (VERDICT:48-54) |
| REPORT.md:12-13 | 呼び出し元は FUN_009b9720 の 3 件 | 9b9e80 の呼び出し元 2 件のうち 9b9720 の内側は 1 件 (VERDICT:17) |
| REPORT.md:25 | FindFreqLevs / BuildFreqImage は各 1 回 | FindFreqLevs は 0x14fa9c と 0x14facc の 2 回 (VERDICT:21) |
| REPORT.md:24・:32、MERGE_PLAN.md:16-18・:21 | 表示用 (確定)、色モードは書き換えられない | 確かめたのは呼び出し元の構造と、コンストラクタ 0x9ba3d0 が書く初期値だけ。表示用という意味づけと、.nfp・UI による上書きは未確認 (VERDICT:18・:25) |
| REPORT.md 項目 1 | モデル面の間隔 0x1f58 | 0x1f54 (`repro/nv_profile.py` の MODEL) |
