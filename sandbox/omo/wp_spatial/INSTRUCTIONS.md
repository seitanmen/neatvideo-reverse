# WP spatial: 空間フィルタ (Filter 0x9ac2b0) のリバースと移植

前提: `../COMMON.md`、`Agents.md` §4・§6.2、`analysis/rtti/astfqg6Filter_*`、`FilterVideoField 0x9a17b8`。v5 は a-trous 5 帯域 + 局所 Wiener の代用で、本家の空間段は未精読。

## 手順
1. リバース: Filter の流れ (帯域分解の種類と段数、帯域ごとの縮小則、ノイズモデル (プロファイル 6 面 × 5 段) と輝度別係数の使い方、
   Noise Reduction Amount・バンドゲイン・Sharpening などの設定値の効き方、時間段の残留ノイズの受け取り方、色空間)。方向フィルタの使用の有無。
2. 実装: `src/nv_spatial.py` (参照版を先に)。
3. 比較:
   - 部品単位: 判定に触れない葉の関数を単体で呼ぶサブコマンドを COMMON §4 で要求し、ビット比較。
   - 結果単位: v5 の空間段と差し替えた変種で区間 A/B (COMMON §6)。設定値は本家出力の設定が不明なので、既定値と v5 の較正値の両方で測る。
