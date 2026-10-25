# WP motion: 動き補償 (MotionCompensator) のリバースと移植

前提: `../COMMON.md`、`analysis/rtti/{EstimateScaleMotion,CompensateImageMotion,CompensateImageSet}_*`
(`0x98a368`・`0x989cd8`・`0x987f88`)。v5 は 3 段ピラミッド BM + SSD 放物線 + メディアン + 双線形の密フローの代用。

## 手順
1. リバース: マクロブロックの大きさ・探索範囲・SAD の定義・サブ画素の求め方・EstimateScaleMotion (拡大縮小・大域動き?) の役割・
   補償画像の補間方法・失敗時の扱い・FilterTemporal への渡し方。
2. 実装: `src/nv_motion.py`。
3. 比較:
   - 部品単位: 判定に触れない関数を単体で呼ぶサブコマンドを COMMON §4 で要求し、ベクトル場と補償画像をビット比較 (合成の平行移動・実写の 2 フレーム)。
   - 結果単位: v5 の動き推定と差し替えた変種で区間 A/B の Y と動き領域 Ymov (calibrate_v5 の指標) を測る (COMMON §6)。
