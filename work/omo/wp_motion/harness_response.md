# 統括の回答 (2026-10-17)

追加した (再ビルド・selftest 終了コード 3 確認済み):
- `nvh sad W H a.f32 b.f32 ax ay bx by [elem]` — 32≤W,H≤512。面は FUN_0097eebc(desc, W, H, 8, elem, 16) / FUN_0097eef8 の記述子と同じ配置で作り、端 8 画素は複製。
  値は floor(x+0.5) を byte (elem 1) か int16 (elem 2) に。座標は画像座標 (−8 から)。出力 `sad16=.. sad32=..`。
  統括の確認: byte 面で numpy の 16×16 / 32×32 SAD と一致 (8821 / 34237)。
- `nvh b32 W H ref.f32 cur.f32 cx cy thr` — FUN_0098bf18(thr, NULL, desc(ref), cur の (cx,cy) のブロック, 32, cx, cy, offs, &ox, &oy, &sad)。
  **オフセット表の形式は差分案と異なる**: 0x98bf18 は 1 要素 8 バイト {int バイトオフセット (dy·stride+dx), short dx, short dy} × 8 を読む
  (逆コンパイルの param_8[2k] と (short)param_8[2k+1] の使い方から)。順は DAT_014877f8 の 8 近傍。out_sad は 8 バイト (ulong*)。
  制限 1≤cx,cy かつ cx,cy+33≤W,H。出力 `b32 ret=.. out_x=.. out_y=.. sad=.. bits=..`。
  統括の確認: 同一面で sad=0・位置そのまま、ずらした面で有限値を返す (正しさの照合は君が行う)。
