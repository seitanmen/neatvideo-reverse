# wp_motion6 サマリ (2026-11-04 時点) — 次セッションの入口

## 目的

本家の動き補償 + 時間フィルタを再現し、E9 の残差 (sintel Y −1.07/RGB −2.55) を潰す。

## 現在の E9 ベスト (spatial プロファイル + nl=0.8 + cn=0.3 + v6 空間 v6set0)

| 素材 | 類似度 | Y | 本家 | 差 | RGB | 本家 | 差 |
|---|---|---|---|---|---|---|---|
| park_joy σ3 | pyramid 1.5 | 39.75 | 40.30 | −0.55 | **37.16** | 37.09 | +0.07 |
| sintel σ3 | sim3c | 44.41 | 45.48 | −1.07 | **42.16** | 43.34 | −1.18 |
| old_town_cross σ3 | pyramid 1.5 | **37.55** | 37.41 | +0.14 | **33.95** | 33.59 | +0.36 |

ハイブリッド (pyramid Y + チャンネル別 Cb/Cr) は各素材の悪い面を組み合わせる結果 (2026-11-04)。

## 確定したこと (証拠つき)

1. **動き推定はビット一致済み**: b9a4 (300 ケース) / adbc (15 ケース) / 平滑化 (8 ケース)。
   `nvh b9a4|adbc|smooth|smoothfull|a79c` で照合可。訂正事項: 32×32 は毎ブロック、
   候補はインターリーブ、32×32 SAD はアンカー基準。
2. **E9 gap の主因はプロファイルモード** (auto の σ 過大評価)。spatial でほぼ解決。
   動き推定・しきい値・luma amount は無関係 (実験で確認)。
3. **時間類似度の構造は解読済み** (NOTES.md §14-§18):
   - しきい値 t = 1.3·モデル語 3 (全レベル一定、count 型鎖は正規状態語で恒等=0)
   - 正規化 X = 差/(ノイズピラミッド[レベル]+1e-4)、R = √(3×3 箱平均)
   - 4 レベル粗→細、w = clamp(2−R/t)·前段、方向経路は空間と同じ葉
   - mode 2 の差の合成 = (A+B+C)/3 (FUN_0097bbb0)
4. **本家しきい値は我々の補償では機能しない** (pyrgen2 実験: 棄却が増えて悪化) →
   **補償品質 (c90c) が最後のボトルネック**。
5. (a,b) の単体近似は逆効果 (gainab 実験: Y静 が悪化) — 本家の計測経路込みで実装が必要。
6. **c90c の近似 (均等重み + 本家どおりの回帰) も逆効果** (2026-11-04、
   `REPORT_v6temporal.md`、独立検証 APPROVE): park_joy Y 39.75 → 39.35・
   sintel RGB 42.16 → 41.03・old_town_cross は全ペア不適用でバイト一致。
   回帰式は本家どおり実装した (b は clamp 前の a_f から、有効率は float32 —
   0x98d218〜0x98d2b4 を統括が再確認)。(a,b) はノイズではなく補償に失敗した
   ブロックの偏りで決まり (クリーン素材でも同じ値)、均等重みの計測では
   本家の重み表 0x1561d40 の近似にならない。**素材別ベスト構成は不変。**

## 次のタスク (順番)

### 1. `nvh c90c` ハーネス照合
- 引数 (§11): x0=instance (+0x538 旗, +0x540, +0x2f8=0, +0x5a8, +0x8→settings)、
  x1=[frameset+0xb0]、x2=[x9+0xb0]、x3=W、x4=H、w5=mode (0/1/2)、x6=records、
  stack: frame_count、planeB、row_table_out、ref_array、meas_array。
- 確定済み: 回帰式 (a = (nΣxy−ΣyΣx)/(nΣyy−Σy²) clamp[0.66,1.5]、b = (Σx−aΣy)/n clamp[−64,64]、
  有効率は出力+0x10、既定 {1,0,0,0})。計測は max(|測定|,|参照|) ≥ 2048 を除外。
- 未確定: 計測の 3 モードの意味、参照配列の作り方、ワープ行テーブルの形式。

### 2. v6 時間段の組み立て — 01: ユーザー操作が必要
**agent-desktop で Resolve の本家プロセスからメモリダンプ**:
- \*(base + 0x1561d40) = 計測の重み表 (c90c mode 1/2 のハーネス照合に必要)
- image 表 +0x1D390 = 受理係数 (nv_motion のフロー品質向上に必要)
これらが手に入れば → c90c bit-exact → v6 時間段 → 全素材で本家構造の既定化。

### 2b. ユーザー操作なしでできること — c90c 近似は閉じた
- **v5 フロー + c90c の (a,b) 回帰の組み合わせ → 実施して逆効果**
  (`REPORT_v6temporal.md` §0/§4、variant `c90c`、独立検証 APPROVE)。
  (a,b) を生かすには本家の重み表の観測 (上の 01) が前提。
- nv_motion のフロー品質: v5 の全探索 (19.93 dB) が nv_motion のパターン探索 (19.27) を
  上回る (既知)。本家の settings 値 (image 表 +0x1D390) が要。
- 色差の σ 推定精度の改善 (sintel Cb −2.9/Cr −2.3 の残差) — 未実施。
- σ6・インターレース素材での c90c 検証 — 未実施 (σ3 で逆効果のため優先度 low)。

### 3. 補助課題
- sintel の色差: sim3c (チャンネル別重み) で RGB +1.37 — 本家 mode 2 は RGB 内部処理なので
  色差 σ の推定精度が前提。
- モデル語 3 の領域サイズ依存 (96² 領域では無効マーカー = 負) の確認。

## 道具

- `tools/e9_nvm.py` — E9 実行 (--variant nvm|v5|pyrgen|pyrgen2|nocrt|sim3|sim3b|sim3c|gainab|gainab2|hybrid|c90c、
  --spatial-variant、--profile-file、--noise-level、--chroma-noise、--set KEY=値)。
  **NV_NATIVE=0 が必要** (ネイティブコアは C++ 側でフローを計算するため)。
  c90c 変種の環境変数: NV6_C90C_BASE (builtin|sim3c) / NV6_C90C_GATE / NV6_C90C_MEAS (warped|shift) /
  NV6_C90C_EXCL_SCALE / NV6_C90C_LOG (ディレクトリは事前に作る)。
- `tools/verify_b9a4.py` / `verify_adbc.py` / `verify_smooth.py` — ビット照合。
- 詳細は NOTES.md (§1-§21。§1-§20 は e5951ae で誤って削除されたため 002b6a7 から復元済み) と
  E9_CALIBRATION_FINDINGS.md、c90c 近似の結果は REPORT_v6temporal.md と eval_v6temporal/。
