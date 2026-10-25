# Neat Video v6 UI 完全再現仕様書

バイナリから抽出した UI 文字列 (1,367 件) をもとに、本家と同一のレイアウト・パラメータ・操作性を再現する。

---

## 1. 画面構成 (2 ページ + メニュー)

### 1.1 Prepare Profile ページ (プロファイル準備)
### 1.2 Filter Settings ページ (フィルタ調整・プレビュー)

左: プレビューエリア (Original / Filtered / Split / Assist Image 切替)
右: サイドパネル (ページごとに切替)

メニューバー: Profile / Filter / Tools / Help

---

## 2. Prepare Profile ページ

### 2.1 Device Noise Profile グループ

| UI 要素 | 種類 | 説明 |
|---|---|---|
| Device Name and Mode | ラベル+ボタン | デバイス名・モード表示 (ダブルクリックで編集) |
| Input Data Gain | ボタン x1.0 | 入力ゲイン調整 (HDR/露出不足の自動補正付き) |
| Auto Profile (Current Frame) | ボタン | 現在のフレームから自動プロファイル |
| Auto Profile (Most Noisy Frame) | ボタン | 最もノイズの多いフレームから自動プロファイル |
| Build Profile (Current Frame) | ボタン | 手動領域選択でプロファイル構築 |
| Build Profile (Most Noisy Frame) | ボタン | 最もノイズの多いフレームで手動領域プロファイル |
| Auto Fine-Tune | ボタン | **AFT 実行 (ライセンス版のみ有効)** |
| Auto Complete | ボタン | 手動 Fine-Tune 後の補間 |
| Manual Fine-Tune | 領域 | ビンごとの手動調整 (スライダー + グラフ) |

### 2.2 プロファイル品質表示

| UI 要素 | 種類 |
|---|---|
| Overall Profile Quality | プログレスバー + % |
| Profile-image match | 一致度表示 |
| Profile Check and Tuning Guide | ガイドボタン |

### 2.3 ノイズサンプル領域選択

| UI 要素 | 種類 |
|---|---|
| プレビュー内の矩形選択 | ドラッグで領域選択 (Auto Profile 用) |
| "Select Preview Area Here / Double Click" | ラベル |

---

## 3. Filter Settings ページ

### 3.1 Preview エリア

| UI 要素 | 種類 | 説明 |
|---|---|---|
| Original / Filtered / Split | ラジオボタン or タブ | プレビューモード切替 |
| Assist Image | モード | ノイズ除去を強調表示する特別プレビュー |
| Split 位置 | スライダー | Split ビューの分割位置 |
| ズーム | コントロール | 100% / 200% / フィット |
| フレームナビゲーション | スライダー + 再生ボタン | フレーム移動・再生 |
| Preview Area | 矩形 | 中央のプレビュー領域 (計算量削減) |

### 3.2 Temporal Filter セクション

| UI 要素 | 種類 | 範囲 | 既定値 | 説明 |
|---|---|---|---|---|
| Enable Temporal Filter | チェックボックス | on/off | on | 時間フィルタの有効化 |
| Temporal Radius | スライダー/コンボ | 0-5 | 2 | 使用する前後フレーム数 |
| Temporal Noise Reduction Amount | スライダー | 0-100% | 100% | 時間フィルタの強さ |
| Temporal Quality Mode | コンボ | Normal/High | Normal | 品質モード (High = 類似度マップの精度向上) |

#### 3.2.1 Components (チャンネル別)

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Luminance Noise Level | スライダー | 0-200% (relative to profile) | 100% |
| Cb Noise Level | スライダー | 0-200% | 100% |
| Cr Noise Level | スライダー | 0-200% | 100% |

### 3.3 Spatial Filter セクション

| UI 要素 | 種類 | 範囲 | 既定値 | 説明 |
|---|---|---|---|---|
| Enable Spatial Filter | チェックボックス | on/off | on | 空間フィルタの有効化 |
| Spatial Quality Mode | コンボ | Normal/High | Normal | 品質モード (High = 方向経路を有効化) |

#### 3.3.1 Noise Reduction Amount (周波数バンド × コンポーネント)

5 バンド × 3 コンポーネント = 15 スライダー

| バンド | Luminance | Cb | Cr |
|---|---|---|---|
| Ultra Low | 0% | 100% | 100% |
| Very Low | 60% | 100% | 100% |
| Low | 80% | 100% | 100% |
| Mid | 90% | 100% | 100% |
| High | 100% | 100% | 100% |

各スライダー: 0-100%、ツールチップにバンドの説明付き

#### 3.3.2 Noise Level (周波数バンド × コンポーネント)

5 バンド × 3 コンポーネント = 15 スライダー

| バンド | Luminance | Cb | Cr |
|---|---|---|---|
| Ultra Low | 100% | 100% | 100% |
| Very Low | 100% | 100% | 100% |
| Low | 100% | 100% | 100% |
| Mid | 100% | 100% | 100% |
| High | 100% | 100% | 100% |

各スライダー: 0-200% (relative to profile)

### 3.4 Sharpening セクション (展開/折りたたみ)

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Enable Sharpening | チェックボックス | on/off | off |
| Ultra Low Freq Sharpening Amount | スライダー | 0-100% | 0% |
| Very Low Freq Sharpening Amount | スライダー | 0-100% | 0% |
| Low Freq Sharpening Amount | スライダー | 0-100% | 0% |
| Mid Freq Sharpening Amount | スライダー | 0-100% | 0% |
| High Freq Sharpening Amount | スライダー | 0-100% | 0% |
| Prevent Over-Sharpening | チェックボックス | on/off | on |

### 3.5 Additional Filters セクション

#### Jitter Filter

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Enable Jitter Filter | チェックボックス | on/off | off |
| Jitter Threshold | スライダー | 0-100% | 0% |

#### Dust and Scratches

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Enable Dust and Scratches | チェックボックス | on/off | off |
| Mode | コンボ | Normal/Aggressive | Normal |
| Threshold | スライダー | 0-100% | 0% |

#### Flicker Filter

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Enable Flicker Filter | チェックボックス | on/off | off |
| Local Flicker Threshold | スライダー | 0-100% | 0% |
| Full Frame Flicker Threshold | スライダー | 0-100% | 0% |

#### Edge Smoothing

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Low Freq Edge Smoothing | スライダー | 0-100% | 0% |
| Mid Freq Edge Smoothing | スライダー | 0-100% | 0% |
| High Freq Edge Smoothing | スライダー | 0-100% | 0% |

### 3.6 Mix と Output

| UI 要素 | 種類 | 範囲 | 既定値 |
|---|---|---|---|
| Mix with original | スライダー | 0-100% | 0% |
| Apply | ボタン | — | フィルタ適用してホストに戻る |

---

## 4. メニュー

### 4.1 Profile メニュー

| 項目 | 機能 |
|---|---|
| Load Profile... | .dnp ファイルをロード |
| Save Device Noise Profile As | .dnp ファイルとして保存 |
| Reset Profile | プロファイルをリセット |
| Change Default Profile... | 既定プロファイルを変更 |
| Copy Profile | クリップボードにコピー |
| Paste Profile | クリップボードからペースト |
| Copy Noise Profile from | 他のインスタンスからコピー |
| Assign As Default Profile | 現在のプロファイルを既定に |
| Profile Viewer... | プロファイル詳細表示 |
| Profile Converter... | プロファイル変換ツール |

### 4.2 Filter メニュー

| 項目 | 機能 |
|---|---|
| Load Filter Preset... | .nfp プリセットをロード |
| Save Filter Preset As... | .nfp プリセットとして保存 |
| Reset Filter Settings | フィルタ設定をリセット |
| Change Default Preset... | 既定プリセットを変更 |
| Copy Filter Settings | クリップボードにコピー |
| Paste Filter Settings | クリップボードからペースト |
| Assign As Default Preset | 現在の設定を既定に |

### 4.3 Tools メニュー

| 項目 | 機能 |
|---|---|
| Preferences | 設定ダイアログ (Computing Devices, Folders, その他) |
| Batch Profiler... | バッチプロファイラー |
| Filter Settings Check | フィルタ設定チェックガイド |
| Noise Level Check | ノイズレベルチェックガイド |
| Profile Check | プロファイルチェックガイド |

---

## 5. ツールチップ (全パラメータの説明文)

バイナリから抽出した説明文をそのまま使用:

### Temporal Radius
"Radius determines the number of consecutive frames (before and after the current frame) used by the Temporal filter."

### Noise Reduction Amount
"Amount of temporal noise reduction - high values -> more reduction of detected noise"

### Noise Level per band
"Estimation of noise level in [ultra low/very low/low/medium/high] frequency component (relative to noise profile) - higher values -> more image elements are treated as noise"

### Sharpening Amount
"Amount of sharpening applied to [ultra low/very low/low/medium/high] frequency component - high values -> more sharpening"

### Mix with original
"Mixing with original frame - high values -> more of filter effect"

---

## 6. Neat Video 特有の UI 機能

### 6.1 Filter Tuning Assist
プレビューを特別モードにして、ノイズ除去の適正値を視覚的にガイドする。
- "Too low / Optimal / Too high" の 3 段階表示
- ノイズが消えて細部が保たれているかを強調表示

### 6.2 Profile Check
プロファイルの品質を視覚的にチェックする。
- ノイズレベルのグラフ表示 (輝度 vs ノイズレベル、R/G/B チャンネル別)
- "Profile quality" インジケーター

### 6.3 Auto Gain
HDR/露出不足の映像を自動検出してゲインを調整。
- "Automatically apply decreased gain to HDR video"
- "Automatically apply increased gain to underexposed video"

### 6.4 Preset / Profile 履歴
- Undo/Redo (フィルタ設定・プロファイル両方)
- 履歴: History<DeviceNoiseProfileWithAttrs>, History<UserFilterParametersWithAttrs>

### 6.5 Preview キャッシュ
- PreviewCacheSize (スライダー)
- PreviewCacheRadius
- PreviewCacheResolution
- スムーズ再生のためのキャッシュ量調整
