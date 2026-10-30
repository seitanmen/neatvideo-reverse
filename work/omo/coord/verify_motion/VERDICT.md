# VERDICT verify_motion (2026-10-18, 独立検証者)
対象: wp_motion/src/nv_motion.py の sad16 (FUN_009f862c/009f9290) と refine32 (FUN_0098bf18)。書き込みはこのフォルダのみ。
本家呼び出しは `nvh` だけ。毎回 `nvh selftest` の終了コード 3 を確認 (実行前に rc=3 を観測、各スクリプトも assert)。同時 nvh は 3 本 (誤って 6 本起動した直後に pkill で止め、結果は破棄して順次再実行)。

## 1. 逆アセンブル 0x98bf18〜0x98c774 の重心部: CONFIRMED
/tmp/nv_expand/disasm_full.txt の 0x98c3f4〜0x98c728 を読んだ。notes §8 / REPORT の主張と一致:
- inv: fadd s0=thr+0.01f (0x3c23d70a) → fdiv 1.0/s0 (単精度、0x98c3f4〜0x98c408)
- d = inv·ucvtf(SAD−best) (x 全幅 64 ビット符号なし→f32、fmul)。FMA・double・frecpe は無い
- 基準: w=(d·−0.66f(0xbf28f5c3))+1.0f を fmul→fadd、さらに fadd +0.0f (0x98c430)、fcmp d,1.5151515f(0x3fc1f07c) の fcsel mi (d<cut のとき w、さもなくば 0。NaN は 0)
- 近傍 0..7 は表の順 (sp+0x34..0x50、表の dx/dy は x22+4/+6、+0xc/+0xe …)。d<cut のみ (b.pl で ≥・NaN は飛ばす) で w=(d·−0.66f)+1.0f (2 回丸め、clamp なし)、num(2 レーン)+=scvtf(dx,dy)·w、den+=w。+0.0f は基準の分母だけ (近傍 0 の分子に movi 0 を足す命令はあるが積が −0 にならず値に影響しない)
- 出力: fdiv num/den → fadd scvtf(x) / scvtf(y) (0x98c708〜0x98c728)。範囲外・best==0x7fffffff は 0x98c3a0 (位置そのまま)
- FUN_009f93ac に早期打切りが無いことは今回は命令を読んでいない (notes の 0x9f949c 記述に依存、未確認)。

## 2. 独自入力での nvh sad / nvh b32 とのビット比較: CONFIRMED (範囲限定)
自作 vm.py (移植は nv_motion_copy.py にコピーし import、repro/ は sys.path に入れない、python -I、PYTHONDONTWRITEBYTECODE=1)。
入力: 38 平面 (64x64〜320x240): 乱数/独立乱数/定数/定数差/勾配/滑らか+ノイズ/飽和 0・255/半画素ずれ、実写 test.mp4 n=300 台の 256x200 切り出し 3 組、test_v02.mp4 n=310 台の 320x240 切り出し 3 組 (ffmpeg gray、自フォルダ)。値は 0..255 に clip。
- sad: 950 件 (16×16 と 32×32、座標 −8 端を含む) 不一致 0
- b32: 4496 件 (閾値: 1e-6〜1e30、−0.005、−0.01 (分母 0)、乱数、SAD 差が d=1.5151515 をまたぐ値の前後 1ulp ×3 レベル) ret/sad/out_x/out_y をビットで比較 不一致 0
ログ: run_clean.log。
(最初の試行で不一致 634 件が出たが、自分の入力が 0..255 を外れていた (nvh が byte に折り返す) ためで、clip 後に 0 件。移植の欠陥ではない)
限界: nvh b32 の制限により速い経路 (1≤cx,cy かつ +33≤W,H) のみ。遅い経路 (範囲外近傍が 0x7fffffff、基準範囲外の 0x98c3a0) は本家未照合。NaN/inf の閾値は未試験。

## 3. 誤り注入: PARTIAL (6 件中 3 件検出、未検出 3 件は理由を確認)
mut_*.py (各 38 平面・5446 呼び出し):
- fused (w を double で 1 回丸め): 検出 (不一致 35)
- nobase (基準位置の重みを分母から除く): 検出 (1469)
- invdbl (inv を double で計算): 検出 (176)
- cut1ulp (閾値 1.5151515f を 1ulp 上げる)・le (d<=cut)・accdbl (num を double 累積): 未検出。
  - accdbl は f32 の和を double→f32 にしても同値 (±1 の積は厳密、加算の二重丸めは起きない) ので等価変異。
  - cut1ulp/le は専用の vm2.py で d==cut・cut+1ulp・cut−1ulp になる閾値を探索し計 88 件を本家と照合したが、なお不一致 0。境界の重み w≈6e-8 は分母 (≥約 1) と位置 (ulp≈4e-6) に比べ小さく f32 出力には現れないため、nvh の出力からは境界の < / <= は区別できない (観測)。境界の向きは 1. の逆アセンブル (b.pl) で確認済み。

## 4. cmp_intervals.py の sys.path: CONFIRMED (小さな注記つき)
verify/cmp_intervals.py は base/ と src/ のみを sys.path に入れ、repro/ を入れない。base/calibrate_v5.py の 24 行目は diff で元の「../../../repro」→ base/ 自身に変わった 1 行だけ (他は元と同一、原本 SHA256 abfbc0ce… 一致)。
注記: base/nv_band.py は現在の repro/nv_band.py と異なる (repro 側が分位点の n<=4096 経路を追加済み)。base の SHA256SUMS とは一致するので古い版のコピーであり、動き補償の PSNR 比較への影響は未評価 (読むだけ)。
SHA256SUMS の calibrate_v5.py 行は表記が「ファイル名 (注記)」で `sha256sum -c` が読めない (表記上の不具合のみ)。

## マージしてよい事実
- refine32 の重心演算 (単精度、基準のみ +0.0f、2 回丸め、d<1.5151515f、表順累積、num/den+位置) は逆アセンブルと nvh b32 の両方で裏づけ (速い経路、5446 呼び出し、不一致 0、実質的な誤り注入 3 件検出)。
- sad16/sad32 (バイト窓 SAD) は nvh sad と 950 件一致。
## マージしてはいけない/注記すべき事実
- 「全件ビット一致」は速い経路の範囲だけ。遅い経路 (範囲外) と NaN/inf 閾値は本家未照合。
- 閾値境界 (< と <=、cut の 1ulp) の向きは nvh 出力では検証不能。逆アセンブルでのみ確認。
- FUN_009f93ac の早期打切りなしは今回は再読していない。
- 98adbc の 32×32 枝は規則により調査せず (移植の analyze_level 内の使い方は本検証の対象外)。
