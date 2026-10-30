# 独立検証 VERDICT — static/QUALITY.md・quality_draft.py (2026-10-26)

方法: RS `/tmp/nv6rs_arm64.ofx` の全逆アセンブルの抜き取り (sed は禁止範囲を除外する `rng.py` 相当で)、Demo は `objdump -d` で必要範囲だけ、
両バイナリの __text の生の命令語の走査 (B/BL の飛び先、fmov の即値)、静的データのファイル読み。本家・RS・nvh は実行・dlopen していない。
禁止範囲と g (RS 0xa4b03c / Demo 0x9bb748) の中は読んでいない。「観測」= 自分で読んだ命令・バイト、「推定」= そこからの推論。

## 判定: APPROVE (下の注記つき)

## 1. 参照の連鎖と表示
| 主張 | 判定 | 根拠 |
|---|---|---|
| "profileQualityLabel" を Ui+0x170 に保存 | 確認済み | RS 0x114dfc〜0x114e18 (str x21,[x20,#0x170]、名前 0x14a8775 長さ 0x13)。Ui = dlg+0x30 は 0x112d20 `add x21,x19,#0x30` → 0x112d38 `bl 0x1131e0`。→ dlg+0x1a0 |
| "simpleQualityLabel" Ui+0x20、"advancedQualityLabel" Ui+0x28、同じクラス 0x58 バイト | 確認済み | RS 0xdf10c〜0xdf128 (名前 0x14a29ef 長さ 0x12)、0xe78bc〜0xe78d8 (0x14a32ad 長さ 0x14)。注: 0x15c04 は `b 0x15b50` (本体は 0x15b50) |
| "Overall Profile Quality" を Ui+0x160 に | 確認済み (setTitle であることは推定のまま) | RS 0x11a530〜0x11a55c |
| 更新 0x117b98 のプロファイル選択 (w1≠0 現在 / w1=0 複写 +0x114e8) | 確認済み | RS 0x117bd0〜0x117c2c。禁止範囲外の 0x117c74〜0x118740 に x19・x28 への書き込みなし (禁止範囲の中は未確認) |
| 0x1186b4 で dlg+0x1a0、0x1186bc で呼び出し、double で qRound(×100)、"%" を連結 | 確認済み | RS 0x1186c0〜0x118704 (100.0 = 0x4059000000000000、d≥0 なら int(d+0.5))、0x118714 の "%" = 0x14966f1 (ファイルで確認) |
| パネル 0xdb1b0: p=[[x1+0x350]+0x28]、u8 p[0xbf34]≠0 のとき呼ぶ、float で ×100.0f の qRound、0x15ce0 へ | 確認済み | RS 0xdb1e4〜0xdb2a8。補足: 呼ぶのは x2 (w23)≠0 の経路だけ |
| 0x15ce0 が int を +0x50 に保存し number(n,10)+"%" → setText | 確認済み (setText は推定) | RS 0x15cf4・0x15d04・0x15d08 ("%" 0x14966f1)・0x15d24 |
| 0xa4acf8 を呼ぶのは 2 か所だけ | 確認済み | __text 全体の B/BL の命令語走査で RS 0xdb25c・0x1186bc の 2 件のみ (B の末尾呼び出しなし)。adrp+add で 0xa4acf8 を作る箇所なし、下位 36 ビットが 0xa4acf8 のデータ語なし |
| [obj+0xd6a0] の先が simple/advanced のラベル | 未確認 (QUALITY.md も推定) | 定数 0xd6a0 は全逆アセンブルで 0xdb1dc の 1 か所だけ |
| クランプなし | 確認済み | 両経路とも qRound の後そのまま |

## 2. Quality 関数の式 (RS 0xa4acf8)
| 主張 | 判定 | 根拠 |
|---|---|---|
| p[0xbf34]==0 → 0 | 確認済み | 0xa4ad14〜0xa4ad20 → 0xa4af18 (movi v0,0) |
| s11 = 0.5·g + 0.5 (fmul→fadd) | 確認済み | 0xa4ad88〜0xa4ad9c |
| 面 0x5e48/0x7d9c/0x9cf0、luma = u8 [0x1622348 + 3·mode + i]、(f, 2.0, −0.25)、luma=0 なら 0.5u+0.5 | 確認済み | 0xa4ada0〜0xa4ae48 (adrp 3032 → 0x1622000 +0x348) |
| P = ((u0+.5)(u1+.5))(u2+.5)+0.25、/3.625、×s11 | 確認済み | 0xa4ae94〜0xa4aeb8。3.625 は 0xa4aeb0 |
| S: W=[x20+0x38c]、H=[x20+0x390]、どちらか 0 なら scvtf(i32 p[0xbf38])、m=min、表 0x162c580 で区分線形、m<16 は 0、m≥256 は 5、q *= S/5 | 確認済み | 0xa4aebc〜0xa4af98 (csel gt で min、fdiv s1=S/5 → fmul) |
| クリップ 1 つごとに ×0.5 | 確認済み | 0xa4aff0〜0xa4b01c (fcsel eq で 0 なら元の値) |
| 静的データ: RS 0x1622348 / Demo 0x147dfc8 = u8 {1,1,1,1,0,0,1,0,0,0,0,0} | 確認済み | ファイルから読んだ値 (直後は "RGB")。どちらも __DATA,__data で vmaddr = ファイルオフセット |
| RS 0x162c580 / Demo 0x1488128 = int64 {16,32,64,128,256} | 確認済み (静的な値) | 同上。注記 A を参照 |
| u = clamp(2.5 − f, 0, 1) (Demo 0x9d49b4) | 確認済み (Demo) | Demo 0x9d49b4〜0x9d4a28: s≠0 で ((0.5/s)/c)·x + ((c−1)·0.5)/c、b.mi → 0、b.le → v、それ以外 1。NaN は v (NaN) を返す (b.le が無順序で成立)。注記 B |
| RS 0xa63938 も同じ式 | 未確認 (推定のまま) | 先頭 8 命令 (0xa63938〜0xa63954、s=0 の分岐) は Demo と同じ。0xa63958 から先は読んでいない |
| 禁止ブロックが s0・s10〜s13・x8・x9・x19・x20・x22 を壊さない | 未確認 | 禁止範囲のため読んでいない。生きているレジスタ: 0xa4ae4c〜0xa4ae90 で s0 (u2)・s10〜s13、0xa4aed0〜0xa4aed4 で x8・x9、0xa4af9c〜0xa4afec で s0・s10・x20 |

## 3. RS 0xa4acf8 ↔ Demo 0x9bb3e0
確認済み。`fmov s<任意>, #3.625` (命令語 & 0xffffffe0 == 0x1e21b000) は __text 全体で RS 0xa4aeb0・Demo 0x9bb5a0 の 1 件ずつ (double の 3.625 は 0 件)。
禁止範囲外の命令列は両者でオペランドまで同じ (Demo の表は 0x147dfc8・0x1488128 から 0x128..0x148)。Demo 0x9bb3e0 を呼ぶ BL も 2 件だけ
(Demo 0xdc354 = float の ×100.0f qRound、0x1196bc = dlg+0x1a0 と double の ×100 qRound) で、RS の 2 か所と同じ形。

## 4. quality_draft.py と R = 0.8
確認済み。`verify/recheck.py` (逆アセンブルから独立に書いた式。nv_flat.flatness と nv_profile.build_profile だけを借りる) の結果 `recheck_results.json`:
- penalty: 15 値 (−1, 0, 1.5 前後, 2.5 前後, 1e30, −0, NaN, ±inf) で草案とビット一致。size_term: W,H 20×20 × 区分 3 の 1,200 組でビット一致
- test_v02 n=312、(832, 0, 128, 128) の完全なプロファイル: 有効 1、区分 4、クリップ 0/0/0、W=H=128、色モード 1、平坦度 1.0704 / 1.1512 / 1.0 → **R = 0.8 を再計算で確認**
- g = 0, 0.25, 0.5, 1, −0.425, −1 で q = 0.4 / 0.5 / 0.6 / 0.8 / 0.23 / 0 (独立版と草案がビット一致。g = −0.425 で 23% になるのは §4 の計算と整合)
- 同じ角の他の大きさ: 48 → 0.5、96 → 0.7、192 → 0.1862、256 → 0.2069 (192・256 は雲などを含み平坦度 4〜7 で Y の u = 0、P = 0.75/3.625。P の罰の経路も草案と一致)

## 注記
- A (観測): Demo の大きさの表 0x1488128 は __DATA,__data の 0x1488xxx (AGENTS.md の「ROOT スロット」と同じ領域) にある。Quality からの読みは普通の adrp+ldr。RS 側の表 0x162c580..0x162c5a8 への静的な書き込みは adrp ベースの走査では見つからなかった (読みだけ)。実行時に書き換わらないことは静的には言えない (推定)。
- B (難読化の兆候、記録のみ): Demo 0x9d49d4〜0x9d49e0 に 0x1489cb8 からのダミー読み (`ldr xzr`/`ldr wzr`)。浮動小数点の値には使われない。そこで止めて解釈していない。
- C: QUALITY.md の「R は領域の大きさだけで決まる」は平坦度 < 1.5 の場合だけ (§4 の表の範囲では正しい)。
- D: QUALITY.md §2 の「コンストラクタ RS 0x15c04」は分岐 `b 0x15b50`。結論に影響なし。
- g (RS 0xa4b03c) は 23% の再現に必要だが未解読のまま (禁止範囲)。式の残りの部分は承認、23% の説明は未確定のまま。
