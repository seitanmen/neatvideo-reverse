> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# 修正依頼 (統括, 2026-10-17) — 独立検証 (sandbox/omo/coord/verify_aft/VERDICT.md) の反証 2 点
1. FUN_009b4e08 の位相 2 はビン 0〜8 の 9 個を処理する (0x9b5238 `cmp x9,#0x100` は書き込みの後)。`range(8)` を 9 ビンに直し、verify_self の「post bin8 never written」試験を逆に直す。
2. 段係数 5 語は 1.0f (memset_pattern16 の模様 0xc962e0 = 0000803f×4)。curve_post 位相 2・3、curve_fill.write の 4 か所を 1.0f に。notes・MERGE_PLAN の「係数 = 0」も訂正。
3. 類似度の CV 鎖で許容 0 直後は asm が b.pl (NaN で続行)。移植の NaN break を合わせる。
4. 試験に定数 (S_LVL の各モード)・ゲート閾値 0.5 の期待値を足し、誤り注入で検出されることを確かめる。
5. 修正後に区間 A/B の v5 変種を再測定し、REPORT.md・MERGE_PLAN.md を更新。
注意: FUN_009b4e08 のループ内 (0x9b51ac〜0x9b5210 付近) にはハッシュ書き込み様のコードがある。読まない・移植しない・調査しない。0x9be22c〜0x9be29c と FUN_009d41b0/9d4708 の検査も同様。aftpoly は却下 (harness_response.md)。
