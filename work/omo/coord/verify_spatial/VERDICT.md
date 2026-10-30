# VERDICT wp_spatial (検証者, 2026-10-17)

保護領域 (0x991000〜0x991080 等) は中身を調べていない (逆アセンブル抜粋の表示に含まれた行は読んだだけで解析していない)。本家は `nvh ramp|wmul|down2` のみ呼んだ (事前に `nvh selftest` 終了コード 3 を確認)。

## 1. 静的解読の主張
| 項目 | 判定 | 根拠 |
|---|---|---|
| 分解 box3×3 → 2×2 平均で縮小 → (3,1)/4 拡大差 | CONFIRMED | 98edb0.c:250 FUN_009ed014(box)→266 FUN_009f15b4(2x2)→273 FUN_00990da8 = param_2 − 993df0(coarse)。993df0.c の係数は 3.0・0.0625 (=(3,1)/4 の積)。端は複製。 |
| 縮小核 W = B/(σ+ε) | PARTIAL (定数が誤り) | 98edb0 の呼び出し (dis 0x98f86c-0x98f880: `mov w8,#0xb717; movk #0x38d1,lsl 16; fmov s0,w8; bl 9ed5f8`) までは正しい (1/(x+ε)、W=帯域×これ)。**ε = 0x38d1b717 は float で 9.99999747e-05 (=1e-4)。「9.4433e-4」は誤り** (python struct で確認)。 |
| 1.1 | CONFIRMED | 990f74 dis 0x99167c: 0x3f8ccccd = 1.1 (1.10000002)。 |
| R = √(box3(W²)) | CONFIRMED (静的) | 990f74.c:219 FUN_009950f4 (=§6.1 の局所 RMS、1/9 箱の平方根)。 |
| X = W·clamp((2t−R)/t,0,1) | CONFIRMED | 990f74 dis 0x991688-0x9916a4: s0×1.1、`fadd s1,s0,s0` (=2t)、1.0、0 を 9eebf8 に渡す。ランプの実体は下記 2 のハーネス照合で一致。 |
| t = 1.1·cfg[k]·\|model[...]\| | PARTIAL | 高速経路の t は (cfg[k]+1e-6)·\|model[+0x14+4k]\|·1.1 (dis 0x99155c-0x991574: s8=cfg[k], s10=model[0x14+k*4]; 990f74.c:230,234)。REPORT §1.3 の `model[+0x780+組·5+k]` は方向経路 (990f74.c:473) の添字で、通常経路の記述として誤り。notes §2.5 の元の記述 `|model[k]|` が正。 |
| 通常経路の出力乗数 = −model[+0x14+4k] (§5 の「訂正」) | **REFUTED** | dis 0x991748-0x991758: `x8=[sp,#0x170]` (これは関数先頭 0x990ff0 で `str x7`=param_8=**cfg**)、`ldr s0,[x8,#0xb8]` (cfg+0xb8 の適応係数)、`ldr s1,[x8+level*4+0x14]` (cfg+0x14+4k)。次に 0x9917a0 `fneg s0,s2`・(1−uVar27)·s0 → 9ee304。990f74.c:847-850 も `fVar44 = *(param_8 + param_7*4 + 0x14)`。つまり通常経路の scale は **−cfg[+0x14+4k] (amount)**、model ではない。当初の notes §2.5「scale = −amount (通常)」が正しく、作業者の「訂正」が誤り (fVar47 の再代入を見落とし)。cfg+0xb8>0 かつ cfg 値≠1 のときだけ ee7bc 経路。マージ計画 §3 の Agents.md 追記 (「model[+0x14+4k]」)・REPORT §6-3 は**反映してはならない**。 |

## 2. 葉の部品のビット照合 (`leaf.py`, `probe3-6.py`, `leaf_log.txt`)
入力: 乱数・境界値 (t, 2t, ±nextafter, 0, -0, 1e±30, 1e-45, ±inf, NaN)、t を 10 通り、n=1..9,13,64,1000,4099。
- ramp (nvh ramp = 009ee93c): 本家と一致する式を特定した。slope=f32((0−1)/(2t−t))、ic=f32(1−t·slope) (非融合)、n&~3 までの NEON 部は **fma(x,slope,ic)**、端数は非融合 `ic+x·slope`、最後に fmin(1)/fmax(0) (NaN は NaN)。この式は 468 ケース・約 7 千要素で NaN 以外 0 不一致 (本家 NaN は NaN 伝播、np.fmax で作った私の模擬が NaN を落としただけ)。
  **nv_spatial の `np.clip((2t−R)/t,0,1)` はビット不一致** (現実的な入力 10 万要素で 12.9% が異なる、絶対差は最大 6e-8、最大 1937ulp は 0 付近)。意味は同じ、丸めだけ違う。
- wmul (nvh wmul = 009eebf8): out = prev × ramp 。上の ramp 式で 0 不一致。nv_spatial の `W*clip(...)` は同様に丸め差のみ。
- pairw: nv_spatial に該当部品なし → 該当なし。
- down2 (nvh down2 = 009f15b4): 本家は **(a+b)+(c+d)** の順 (a,b = 上の行の隣、c,d = 下の行) で ×0.25。528,297 要素 (15 サイズ) で 0 不一致。nv_spatial.halve は ((上a+下c)+上b)+下d の順で、**要素の約 40% が不一致** (最大 65536ulp は相殺で和が 0 付近のときの相対値、絶対誤差は数 ulp)。式の形 (2×2 平均) は CONFIRMED、ビット一致は REFUTED。
- 判定: 部品の意味 = CONFIRMED、nv_spatial の実装のビット一致 = REFUTED (修正は容易: ランプを fma 式に、halve の加算順を (a+b)+(c+d) に)。box3・拡大・局所 RMS は対応するハーネス部品が無く未照合。

## 3. 区間 A の再測定 (元の calibrate_v5.py の複写 + 差し替えフックのみ追加, 同時 3 プロセス)
`calibrate_v5_copy.py yuv 296 --candidates cands.json --jobs 3`。区間 YUV は私が ffmpeg で再抽出し作業者の data/ と SHA256 一致を確認。
| 候補 | 私の測定 Y/U/V | REPORT §4 |
|---|---|---|
| 入力 | 37.863/49.364/48.765 | 37.863/49.364/48.765 |
| v5 既定 | 47.117/50.985/51.179 | 47.117/50.985/51.179 |
| 差し替え t=1.5 | 46.997/50.625/50.476 | 46.997/50.625/50.476 |
→ **CONFIRMED** (差 0.000 dB)。参考: ε を正しい 1e-4 にしても t=1.5 は 46.996/50.624/50.474 (影響 0.002 dB 以下、誤定数は結果の数値を変えない)。区間 B は未測定。
注: v5 は本家出力への較正込み、差し替えは t=1.5 の手調整 (本家の実 t は未取得) なので「構造差は小さい」という解釈は推定。

## 4. src/nv_spatial.py のレビュー
- 既定動作: **変えない** (CONFIRMED)。nv_spatial は nvfilter から import されず、nvfilter.py も無変更 (repro/nvfilter.py と base/nvfilter.py は diff なし)。オプション/モンキーパッチ扱い。
- バグ/注意:
  1. EPS_WHITEN = 9.4433e-4 は誤 (正しくは 1e-4、0x38d1b717)。コメントも同じ誤り。影響は 0.002 dB 未満だが直すこと。
  2. P["spatial"] (v5 の空間強度 κ、既定 2.0、UI のスライダ) を使わない → 差し替え時に UI/CLI の spatial が無効になる。
  3. 出力の乗数は amt (v5 の band_amounts×luma/chroma を明示乗算) で、本家の cfg+0x14 に相当する値を v5 の Amount で代用。構造としては REFUTED の「model 側」ではなく cfg 側が正しいので、代用の方向は結果的に妥当。
  4. t=1.1×genuine_t は cfg[k]·|model[k]|≈1 の仮定 (未検証)。
  5. ramp と halve が本家とビット不一致 (上記 2)。
  6. 方向フィルタ組 1..4・周期パターン保護は未実装 (REPORT §7 に明記済み)。
  7. 極小平面 (h2 or w2 == 0) で halve が縮小せずコピーを返す (5 段でも 1080p/4:2:0 では起きない)。
- py_compile 相当: 区間 A の実走で import・実行は通った。

## マージしてよい事実 / よくない事実
- よい: 分解の構造 (box3→2×2→(3,1)/4 差、5 段)、W=B/(σ+ε) の形 (ε=1e-4 に直して)、1.1、X=W·clamp((2t−R)/t,0,1)、ランプ/縮小/2×2 平均の本家式 (上記の正確な丸め順)、区間 A の数値 (v5 既定・t=1.5)、nv_spatial が既定を変えないこと。
- よくない: ε=9.4433e-4、「通常経路の出力乗数は model[+0x14+4k]」(REPORT §1-3・§6-3、notes §5 の訂正、MERGE_PLAN §3)、通常経路の t の添字 `model[+0x780+組·5+k]`。Agents.md には cfg+0x14+4k (amount) / model+0x14+4k (t の因子) と書く。
- `nvh shrink` 却下は妥当 (私も 0x991000 付近を調べていない)。残る未照合は box3・拡大 (993df0)・局所 RMS・990f74 全体で、統括の方針どおりハーネス不可。

## 総合
静的解読 1〜3: 構造は CONFIRMED、定数 ε と出力乗数の 2 点が REFUTED、t の添字は PARTIAL。葉の部品: 意味 CONFIRMED / nv_spatial のビット一致 REFUTED。区間 A: CONFIRMED。コード: 既定不変 CONFIRMED、ε 等の軽微なバグあり。

成果物: leaf.py leaf_log.txt probe*.py calibrate_v5_copy.py cands.json run_A.log run_A_eps.log (すべて coord/verify_spatial/ 内)。
