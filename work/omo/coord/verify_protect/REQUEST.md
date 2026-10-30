# 検証依頼: 統括の静的読み (lead_static_20261021.md) の抜き取り確認 (2026-10-22)

検証者へ。統括 (引き継ぎエージェント) が `sandbox/omo/wp_spatial/notes/lead_static_20261021.md` に書いた
静的な読みのうち、**AGENTS.md への反映対象の主張**を抜き取りで確認してほしい。
装備: 逆アセンブル `/tmp/nv_expand/disasm_full.txt` (3,287,476 行、arm64 スライス) と
`NeatVideo6.arm64.ofx` のファイルバイト (__TEXT は fileoff = addr、__data も fileoff = addr、
__bss / __common はファイルにない = ゼロ初期化)。実行はしない (静的のみ)。

規則: `sandbox/omo/coord/VERIFIER_RULES.md`。ただし今回の対象は**統括が保護区画を含めて読んだ記録の確認**で、
ユーザーが 2026-10-21 に統括へ保護区画の解析を一任している。あなたも確認のために同じ範囲を読んでよいが、
「読む範囲を最小にする」こと (主張の箇所 ± 数命令で足りる)。書き換え・パッチ・ハーネス変更はしない。

## 確認する主張 (各 1 件ずつ、逆アセンブルの引用付きで CONFIRMED / REFUTED を返すこと)

| # | 主張 (出典は lead_static_20261021.md の節) | 確認ポイント |
|---|---|---|
| A (§9.1) | FUN_00990448 の count 欄 [0x810] への `lsl x15, x15, x22` は浅い角度 (flag=1) の経路 0x9907a0 だけにあり、急な角度 (flag=0) の 0x99061c〜0x990624 にはない。x22 は 0x9904c8 の `ldr x22, [0x1561000+0xd58]` (u64) | 2 箇所の str [.., #0x810] と ldr のレジスタ幅 |
| B (§9.2) | 0x1561d58 と 0x1561dec は __DATA __common (0x155b1e0 + 0x2f734) に属し、ファイルには値がない (otool -l の sect addr/size で確認) | セクション範囲の計算 |
| C (§9.2) | 保証語 0x1561dec が 0 のとき popcount(0 & 0x3fffff) − 16 = −16 ≠ 0 で、FUN_009ae918 末尾 0x9af230〜0x9af244 は [0x1561d58] = 1 を書く。cbz で 0 のときだけ書かない | 0x9aed78〜0x9aee74 の ubfx 鎖と 0x9aee70 の sub #0x10、0x9aee74 の stp xzr (PopcountGuarantee ≥ 16 のとき配列 NULL)、0x9af230 の cbz |
| D (§9.3) | 0x1561d58 への書き込みは全 3 箇所 (0x45114 / 0x9af244 / 0x9ebbbc)。0x9ebbbc は FUN_009eacb0 内にあり、0x9eb460 の popcount 鎖 ([x21+0x8c] & 0x1ffff の 32 ビット popcount − 16) を [sp+0x80] に置き、0x9ebba8 の cbz で非 0 のときだけ 1 を書く。FUN_009e9940 は 0x9ea2bc で FUN_009eacb0 を呼ぶ | `grep -n "#0xd58\]" disasm` で書き手を全列挙 (adrp 0x1561000 のものだけ)。0x44dd4 関数が RTTI `14AbstractDrawer` の slot3 であること (vtable 0x13b5808 の中身) |
| E (§9.4) | count<<1 で浅い角度のカーネルは idx テーブル local_1058[512] の [n..511] (未初期化 = 前のカーネルの残骸) を読み得る。start+2·count−1 ≤ 511 は常に成り立つので SIGSEGV ではなく残骸依存 | idx テーブルの確保命令 (0x990448 の prologue、512 語) と start/count の定義域 |
| F (§9.5) | FUN_0099afb4 内 0x99b270〜0x99b384 は w19 = *(u32*)(*(0x1487c30) + 0x763648) の 32 ビット popcount − 16 で、0x1487c30 は rebase エントリ (ファイル値 0x869678) なので **w19 = *(u32*)(image+0xFCCCC0) = 0xe22edf32、popcount = 18、値は固定 2** | ファイルバイト 0xFCCCC0 の 4 バイト (e2 2e df 32) と popcount 計算。0x869678 が rebase (上位ビットなし) であること |
| G (§9.5) | FUN_0099afb4 の係数は s15 = ((DAT_012445a8[x27] × 0.25) × s8 × 1.27734) × 0.9 × (x22 − 2)。x22 は FUN_0099afb4 の整数引数で、2 = popcount(w19) − 16 | 0x99b364〜0x99b3b8 の fmul/fmov の定数 (0x3f8cccccd → 0.25f は 0x3e000000 ではなく 0x3f8c00c…? **要確認: 0x3f8ccccd = 1.1 ではない。0x3f8ccccd = 1.1f**。→ 定数の読み違えがないか特に確認) |
| H (§9.5) | 呼び出し側 FUN_0099a76c は s0 = ((popcount 鎖 (w22) − (popcount 鎖 (w19) + 0xe)) × 1.3 × [x24+0xc]) を FUN_0099afb4 の v0 (s8) で渡す。0x1487fd8 は bind エントリ (実行時値) | 0x99aabc〜0x99ac78 の鎖と 0x3fa66666 (= 1.3f) |
| I (§9.6) | FUN_009ac2088 は作業面プールのキャッシュ: 0x9f7c54 検索 / 0x9f7c9c 比較 / FUN_009d3278 alloc、エントリに 100.0/n を書く (0x42c80000)、memset_pattern16 0x88 バイト。チャンク配置には関与しない | 呼び出し 0x9ac87c の引数 (w5 = (核+2·余白)²·4 ではなく面積?) と 0xac24f8 の str 0x64 |
| J (§9.4) | 下限试算の結果 (暗部ビンで +3.9% / −0.6%、明部 ≪1%) は再計算で再現する | `python3 - <<EOF` で lead_static の patch (count<<1, 残骸 0, 面外無視) を nv_lines.gather_tables に当てて ncv.genuine_noise_curve(crop, batch=False) を 2 回 (seed 7 の乱数タイルでよい。実フレームでなくてよい)。比率の傾向 (暗部で動く、明部でほぼ 1) が再現すれば CONFIRMED |

## 返してほしいもの
`VERDICT.md` に (1) 各主張の判定、(2) G の定数 (0x3f8ccccd) の正しい float 値と、係数式の正しい形、
(3) 「マージしてよい事実 / 書いてはいけない事実」の 2 リスト。統括が AGENTS.md・exp_results.txt への
反映を行うので、あなたはファイルを書き換えないこと。

なお E の「未初期化スタックの残骸」は本質的に再現不能 (未定義動作) なので、「残骸依存で決定的再現不可」
という結論自体の確認でよい。
