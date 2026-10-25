# verify_consistency_20261020 (2026-10-20, 独立検証者。返答本文を統括が要旨として転記)
対象: 2026-10-20 の文書整合の修正 (AGENTS.md・exp_results.txt・wp_*/CORRECTIONS.md・コードの小修正)。読み取りのみ、nvh 不実行。
- 1 AGENTS.md: PARTIAL。ε の場所 (0x98f86c〜0x98f880 → bl 0x9ed5f8、FUN_0098edb0 内)、出力乗数 −cfg[+0x14+4k] (0x991748〜0x9917a0 の fneg と分岐)、§7 の速度比 21〜23 倍・17 倍、語 0x1464 の書き換え、ramp の一般形は CONFIRMED。言い過ぎ 3 点 (sandbox/omo の「マージしたのは…だけ」、§6.3 の空間の数値が旧 ε の測定、SIGBUS の確認範囲) → 統括が修正。
- 2 exp_results.txt: PARTIAL。unverified 節は CONFIRMED。down2 の 528,297 要素の件数の帰属が誤り → 統括が修正。
- 3 CORRECTIONS.md: PARTIAL。本文はバナー以外不変。行番号の誤り 2 件 (temporal の src:14・:228 → :16・:242、motion の notes:205 → :203) → 統括が修正。notes の本文が不変であることは検証できない (控えがバナー追加後)。
- 4 コード: CONFIRMED (wp_speed は src_fix と同一、wp_spatial は EPS とコメントだけ、wp_temporal はコメントだけ、nvh.c は usage だけ、aft_port の SHA256SUMS は一致)。nv_temporal.py:13 の「NaN で効く」→ 統括が修正。
- 修正後の再検証はしていない。
