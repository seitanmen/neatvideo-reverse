# wp_motion6 サマリ (2026-11-04 時点) — 次セッションの入口

## 目的

本家の動き補償 + 時間フィルタを再現し、E9 の残差 (sintel Y −1.07/RGB −2.55) を潰す。

## 現在の E9 ベスト (spatial プロファイル + nl=0.8 + cn=0.3 + v6 空間 v6set0)

| 素材 | 類似度 | Y | 本家 | 差 | RGB | 本家 | 差 |
|---|---|---|---|---|---|---|---|
| park_joy σ3 | pyramid 1.5 | 39.75 | 40.30 | −0.55 | **37.16** | 37.09 | +0.07 |
| sintel σ3 | v5 (または sim3c: RGB 42.16) | 44.41 | 45.48 | −1.07 | 40.79 | 43.34 | −2.55 |
| old_town_cross σ3 | pyramid 1.5 | **37.55** | 37.41 | +0.14 | **33.95** | 33.59 | +0.36 |

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

## 次のタスク (順番)

### 1. `nvh c90c` ハーネス照合
- 引数 (§11): x0=instance (+0x538 旗, +0x540, +0x2f8=0, +0x5a8, +0x8→settings)、
  x1=[frameset+0xb0]、x2=[x9+0xb0]、x3=W、x4=H、w5=mode (0/1/2)、x6=records、
  stack: frame_count、planeB、row_table_out、ref_array、meas_array。
- 確定済み: 回帰式 (a = (nΣxy−ΣyΣx)/(nΣyy−Σy²) clamp[0.66,1.5]、b = (Σx−aΣy)/n clamp[−64,64]、
  有効率は出力+0x10、既定 {1,0,0,0})。計測は max(|測定|,|参照|) ≥ 2048 を除外。
- 未確定: 計測の 3 モードの意味、参照配列の作り方、ワープ行テーブルの形式。

### 2. v6 時間段の組み立て
c90c 補償 (ワープ + ブロック (a,b) + 有効率ゲート) + 解読済み類似度
(t = 1.3·モデル語 3 の pyramid) → E9 再評価 → 全素材で本家構造を既定化。

### 3. 補助課題
- sintel の色差: sim3c (チャンネル別重み) で RGB +1.37 — 本家 mode 2 は RGB 内部処理なので
  色差 σ の推定精度が前提。
- モデル語 3 の領域サイズ依存 (96² 領域では無効マーカー = 負) の確認。

## 道具

- `tools/e9_nvm.py` — E9 実行 (--variant nvm|v5|pyrgen|pyrgen2|nocrt|sim3|sim3b|sim3c|gainab、
  --spatial-variant、--profile-file、--noise-level、--chroma-noise、--set KEY=値)。
  **NV_NATIVE=0 が必要** (ネイティブコアは C++ 側でフローを計算するため)。
- `tools/verify_b9a4.py` / `verify_adbc.py` / `verify_smooth.py` — ビット照合。
- 詳細は NOTES.md (§1-§18) と E9_CALIBRATION_FINDINGS.md。
