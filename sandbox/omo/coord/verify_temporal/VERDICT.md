# VERDICT wp_temporal 葉 4 関数 (ramp / wmul / pairw / down2) 2026-10-18 独立検証

総合: 4 関数の移植は **CONFIRMED** (NEON 判定 非 0 の経路、本家と完全ビット一致)。REPORT の文面に軽微な不正確が 2 点 (下記、出力には影響なし)。

## 1. 逆アセンブル読解 — CONFIRMED (文面 2 点を除く)
`/tmp/nv_expand/disasm_full.txt` を自分で読んだ (ramp 0x9ee93c〜0x9eebf4、wmul 0x9eebf8〜0x9eef58、pairw 0x9efb1c〜0x9efc90、down2 0x9f15b4〜0x9f1738)。
- 4 関数とも bl/blr/br なし。参照する大域は adrp ... #0x7a8 (ページ 0x14a6000 → 0x14a67a8) だけ (ldrb で NEON 判定)。スタックガードも使わない。→ 「外を呼ばず大域は 0x14a67a8 のみ」CONFIRMED。
- ramp: 定数 0x3727c5ac/0x3f7fff58/0xb727c5ac/0x3f800054 の補正、slope=(s3−s2)/(b'−a')、ic=s2−a'·slope。NEON 部 (n&~3): fmla(ic + x·slope、融合)→fmin→fmax。s3>s2 なら fmin 側=s3・fmax 側=s2、それ以外は逆 (b.le 分岐)。端数部: fmul→fadd→fcmp/fcsel (gt)、NaN は fmin 相当で素通り→fmax 相当で下限へ (NaN→下限)。REPORT の式どおり。
- **不正確 1**: REPORT は「b−a>b·1e-5 が成り立たないとき (順序なしを含む) 補正」と書くが、命令は `fcsel ..., hi` で、`hi` は順序なしで**真**。つまり順序なしでは補正**しない**。移植 (`keep = s4 > s6`) は順序なしで補正する。ただし順序なしになるのは a/b が NaN か a=b=±inf のときだけで、どちらの経路でも slope は NaN になり出力は同一。照合 (t = nan/±inf を含む) で差は出ない。出力に影響なし (観測)。文面だけ直すべき。
- wmul: ramp 係数は同一。NEON 部 `fmul v16(ramp), v17(prev)`、端数部 `fmul s4(prev), s5→` (第 1 オペランド = prev)。
- **不正確 2 (観測)**: 「端数部で NaN の選び方が違う」は命令上は正しいが、端数部の ramp は NaN を下限 (0) に潰すので ramp 側は NaN になれず、積の順序は出力に現れない (順序を入れ替えた誤り注入で 0 件の差、下記)。NEON 部の順序は観測可能 (入れ替えで 200/320 試行が不一致)。
- pairw: NEON 部 `fmax.4s → fsub.4s b → fmul.4s c·(…)`、端数部 `fcmp a,b; fcsel gt` (順序なし→b) → fsub → fmul c·(…)。CONFIRMED。NEON 判定 0 の経路は 4 要素束でも fcmgt/bsl (a>b?a:b)。
- down2: NEON 部は W&~7 列を 8 列単位で `faddp(上 8 列)`, `faddp(下 8 列)`, `fadd`, ×0.25 = (a+b)+(c+d)。残りは (W&7)>>1 個を ((a+b)+c)+d の順 (ldp/fadd/fadd/fadd/fmul)。奇数の最終列は捨てる。1 回の呼び出しで 2 行→1 行。CONFIRMED。
- 未確認: NEON 判定バイトが 0 の経路 (本家のメモリを書けないので実行できない。読解のみ。ramp 端数部と同じ非融合の経路になる)。

## 2. 本家とのビット比較 — CONFIRMED
手順: `nvh selftest` の終了コード 3 を確認後、独立に書いた `cmp.py` (シード 777001、実装者のスクリプトは未使用) で `nvh ramp|wmul|pairw|down2` を 1 プロセスずつ逐次実行。移植は `port/` にコピーし PYTHONDONTWRITEBYTECODE=1 で import。比較は uint32 ビット列 (NaN のペイロード・符号・−0 を含む)。
- 入力: 一様・正規×10^[−6,6]・[0,3] の 3 分布。各要素 12% を特殊値 (qNaN/sNaN の各種ペイロード、±inf、±0、非正規数、最大値、1.0、2.0 ほか) に置換。長さ 1〜4099 の 21 種 (端数 0〜3 を網羅)。
- t: 0.001, 0.01, 0.2, 0.5, 1, 1.3, 3, 4, 10, 100, 1e-6, 1e-20, 1e-38, 1e30, 0, −1, −0.5, NaN, +inf, −inf, 1e-5, 0.99999, 2.5e-45。
- pairw は a=b の同値多数の入力も含む。down2 は (W,H) 24 種 (2×2、W=3,7,8,9,15,16,17,23,24,25,31,33,64,65,127,129,1921,4096 など) 。
- 結果 (`RESULT bad=0`): ramp 1449 件 / 393,576 値、wmul 1449 / 393,576、pairw 216 / 406,629、down2 72 / 39,396。**不一致 0**。
- 補足: selftest rc=3 を実行前に確認 (出力の「genuine alert suppressed」は想定内)。nvh は同時 1〜3 本。

## 3. 誤り注入 — CONFIRMED (4 種検出、1 種は等価と確認)
`mutate.py` (移植のコピーを差し替え、同じ比較を間引き 40% で実行):
- down2 の境界を W&~7 → W&~3: 検出 (14 バッチ不一致)。
- pairw の端数部を np.maximum (NaN 伝播) に変更: 検出 (4 バッチ、NaN・同値で不一致)。
- ramp/wmul の fmla を非融合 (mul→add) に変更: 検出 (211 バッチ不一致)。
- wmul の NEON 部の積の順を prev·ramp に入れ替え (`wmul_nan2.py`、ramp が NaN になる入力): 検出 (200/320 試行)。正しい移植は 0/320。
- 参考: wmul の**端数部**の順序入れ替え、および ramp の 1e-5 補正を外す変更は、`mutate.py wmul_tail_order` が 0 件 (端数部の ramp が NaN を 0 に潰すため等価)。補正なし (`ramp_noadjust`) は今回未実行 (未確認)。

## 4. down2 = repro/nv_pixnoise.down2 — CONFIRMED
`repro/nv_pixnoise.py` を `port/repro_nv_pixnoise.py` にコピーし `down2(x, simd=True)` と `down2_genuine` を比較 (`item4.py`): W=2〜69, 127〜129, 1919〜1921 × H=2,3,6,9 × 3 分布 (特殊値込み) = 888 件、ビット不一致 0。式 ((a+b)+(c+d) を W&~7 列、残り ((a+b)+c)+d) は同一。なお `base/nv_pixnoise.py` は repro/ と同一ファイル (diff 一致)。

## マージしてよい事実
- 4 関数 (ramp/wmul/pairw/down2) の移植は NEON 非 0 の経路で本家とビット一致 (上の入力範囲)。外部呼び出しなし・大域は 0x14a67a8 のみ。
- down2 は repro/nv_pixnoise.down2 と同一式。
## マージしてはいけない / 直す点
- REPORT の「順序なしを含めて補正する」は誤読 (命令は順序なしで補正しない)。出力は同一なので「順序なしでは補正しないが、NaN になるため出力は同じ」と書く。
- 「wmul 端数部で NaN の選び方が違う」は観測できない差 (出力に影響しない) と注記する。
- NEON 判定 0 の経路 (0x14a67a8 が 0) は検証していない。
