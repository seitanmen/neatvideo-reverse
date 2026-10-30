# verify_spatial2 VERDICT (2026-10-21, 独立検証者)
(a) CONFIRMED / (b) PARTIAL (n=W*H のみ推定) / (c) CONFIRMED (ログ同一; 前後ログの生成条件は未確認) / (d) CONFIRMED。マージ可。
詳細は返答本文。スクリプト: v.py (python3 -I v.py)、src/ にコピー、data/ に入力。

## 統括による転記 (2026-10-21。検証者の返答本文の要旨)
- (d) CONFIRMED: before_bitexact との diff は、ヘッダのコメント・nv_temporal からコピーした葉関数・halve・縮小式 X = wmul_genuine(t, local_rms(W), W) だけ。src と base は同一。
- (b) PARTIAL: wp_spatial/tmp/disasm_full.txt で確認 (禁止範囲はスクリプトで除外)。0x98f29c で H<2 は呼ばず、0x98f2bc〜0x98f2dc で bl 0x9f15b4 を H>>1 回。0x990d70〜0x990d90 も同じ形。0x9916a4 の bl 0x9eebf8 は s0=t、s1=t+t (0x991690)、s2=1、s3=0、x0=R [sp+0x73b8]、x1=W [sp+0x150]、x2=X [sp+0x7398]、x3=n [sp+0x1b0]。同じ n を 0x9ed710・0x9ee7bc・0x9ee304 にも渡す。n の書き込みは禁止範囲 (0x99100c 付近) のため n = W_k·H_k は推定。
- (a) CONFIRMED: シード 777123、nvh selftest 3 を確認、同時 3 本以内。nvh down2 1,560 件・246,400 値、nvh wmul 924 件・17,460,894 値で不一致 0。旧 halve は 1,273/1,560 件で不一致 (感度)。
- (c) CONFIRMED (読めた範囲): bitexact と prebitexact のログの数値が一致し、v5 既定は AGENTS.md の値と一致。prebitexact が変更前のコードで作られたかは再実行していない。
- マージしてよくない事実: n = W_k·H_k (推定)、h2/w2 = 0 の段の一致、逆アセンブルの出典を /tmp/nv_expand と書くこと (当時は消えていた)。
