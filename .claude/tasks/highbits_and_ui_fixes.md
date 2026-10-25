# タスク: 高ビット深度対応 (10/12/16/32bit float) + UI 修正 3 件

## 優先度 1: 高ビット深度対応 (32bit float 含む) — 大タスク

### 現状の制限

v5 パイプラインは**8-bit yuv420p に固定**されている。
処理コア (`nvfilter.py`) は内部で float32 を使用するが、入出力が 8-bit に丸められるため、
高ビット深度のソース (10-bit HDR, 12-bit シネマ, 16-bit RAW, 32-bit float EXR 等) の品質が失われる。

### 目標

10/12/16-bit 整数と 32-bit float の映像を**ビット深度を落とさずに**処理・出力できるようにする。

### 修正箇所

#### A. 読み込み (`denoise_video_v5.py`)

```python
# 現状: ffmpeg に -pix_fmt yuv420p を強制 (8-bit に丸められる)
cmd = [..., "-pix_fmt", "yuv420p", ...]

# 修正: ソースの pix_fmt を検出して、より高いビット深度で読む
# - yuv420p10le → 10-bit
# - yuv420p12le → 12-bit
# - yuv444p16le → 16-bit
# - gbrpf32le  → 32-bit float RGB
```

ffprobe で入力の pix_fmt を検出し、そのビット深度で rawvideo を読む。
float32 の (Y, Cb, Cr) planes に変換して返す。

#### B. 処理コア (`nvfilter.py`)

既に float32 で動作するため変更は最小限。ただし:
- 4:2:0 のクロマサブサンプリングを 4:4:4 (全解像度) に拡張 (高ビット深度時)
- `CURVE_BINS` の量子化 (8-bit 前提の bin 計算) をビット深度に対応
- uint8 への clip/round を除去 (float32 のまま出力)

#### C. 書き出し (`denoise_video_v5.py`)

```python
# 現状: uint8 に clip → yuv420p で libx264 に入力
# 修正: ビット深度を維持したまま出力
# - 10-bit → libx264 10bit (yuv420p10le) または ProRes 4444 (yuv444p10le)
# - 16-bit → ffmpeg が対応するコーデック (ffv1, png sequence 等)
# - 32-bit float → EXR シーケンス または v210/v308
```

#### D. OFX ブリッジ (`nv_ui_qt/ofx_bridge.py`)

```python
# 現状: uint8 yuv420p (frame_size = W*H + 2*(W/2)*(H/2))
# 修正: ビット深度別のフレームサイズと pack/unpack
# - 8-bit:  現状のまま
# - 16-bit: W*H*2 + 2*(W/2)*(H/2)*2 bytes (uint16)
# - 32-bit: W*H*4 + 2*(W/2)*(H/2)*4 bytes (float32)
```

#### E. UI プレビュー (`nv_ui_qt/`)

- 16-bit 以上の planes を 8-bit にダウン変換して表示 (QImage は 8-bit)
- ビット深度の情報をステータスバーに表示

#### F. テスト

- 8-bit 入力 → 8-bit 出力: 従来とビット一致 (回帰テスト)
- 10-bit 入力 → 10-bit 出力: ビット深度が維持されることを確認
- 32-bit float 入力 → 32-bit float 出力: 精度が維持されることを確認

### 実装の注意点

1. **既存 8-bit の動作を壊さない**: `depth=8` が既定。明示的に `depth=32` 等を指定した場合のみ
   高ビット深度パスを使う
2. **ffmpeg の pix_fmt 対応表**:

| ffmpeg pix_fmt | bits | numpy dtype | プレーンサイズ (W×H) |
|---|---|---|---|
| yuv420p | 8 | uint8 | Y: W*H, Cb/Cr: W/2*H/2 |
| yuv420p10le | 10 | uint16 | 同 (10-bit 値を uint16 に格納) |
| yuv420p12le | 12 | uint16 | 同 |
| yuv444p16le | 16 | uint16 | Y/Cb/Cr: 各 W*H*2 |
| yuv444pf | 32 | float32 | Y/Cb/Cr: 各 W*H*4 |
| gbrpf32le | 32 | float32 | G/B/R: 各 W*H*4 (RGB 順に注意) |

3. **4:4:4 対応**: 高ビット深度では 4:2:0 のクロマサブサンプリングを避ける
   (情報落ちを防ぐため)。`nvfilter.py` の `levels` 計算で chroma の `s=1` を使う

4. **色空間**: HDR (PQ/HLG) のトーンマッピングは範囲外。ビット深度の維持のみを扱う

---

## 優先度 2: ノイズの選択範囲で長方形を許可 — 中タスク

### 現状

`find_flat_area()` は `(y0, x0, size)` の正方形を返す。
`nv_profile.build_profile()` は `(x, y, w, h)` を受け取るが、UI とパイプラインで正方形に制限している。

### 修正

1. `find_flat_area()` の戻り値を `(y0, x0, h, w)` に変更 (長方形)
2. UI の領域選択をドラッグで長方形に対応
3. `build_profile_v5()` に長方形の area を渡す
4. `nv_profile.build_profile()` の crop を長方形で切る (既に対応済みの可能性)

---

## 優先度 3: Beginner モードの動画修正 — 小タスク

### 現状

Begin ボタンで表示されるチュートリアル/ガイドの動画が本家と異なる。

### 修正

本家の Quick Start Guide は Neat Video の公式サイト (neatvideo.com) で公開されている
チュートリアル動画。再現では独自のガイド文を表示しているため、本家と同一の動画・
画像を使うには本家のリソースファイルを参照するか、同等のコンテンツを用意する。

---

## 優先度 4: ドロップダウンメニューの表示崩れ修正 — 小タスク

### 現状

Qt のコンボボックス / メニューの表示が崩れる (スタイルシートの問題)。

### 修正

PySide6 のスタイルシートでドロップダウンの項目高さ・フォントサイズ・
パディングを明示的に指定する。macOS のネイティブスタイルとの干渉を避けるため
`QApplication.setStyle("Fusion")` を検討。
