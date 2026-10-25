# タスク: Neat Video v6 UI を Qt (PySide6) で完全再現する

## 概要

Neat Video v6 (OpenFX プラグイン) の UI を PySide6 で完全に再現する。
バイナリから抽出した 1,367 件の UI 文字列を構造化した仕様書に基づき、
本家と同一のレイアウト・パラメータ・操作性を実現する。

- 仕様書: `repro/ui_spec/UI_SPEC.md`
- 既存 UI (tkinter・参考用): `repro/nv_ui.py`
- バックエンド: `repro/nvfilter.py` (v5 パイプライン) + `repro/nv_aft_v5.py` (AFT)

## 必須要件

1. **PySide6** を使用 (tkinter は使わない)
2. 本家と同一の **2 ページ構成** (Prepare Profile + Filter Settings)
3. 本家と同一のパラメータ名・範囲・既定値・レイアウト
4. バックエンドは既存の v5 パイプライン (`nvfilter.py`) をそのまま使う
5. AFT (`nv_aft_v5.py`) を Prepare Profile ページから呼び出せること

## 画面構成

### メインウィンドウ

```
┌──────────────────────────────────────────────────────────────┐
│  Neat Video v6 (Reduce Noise v6)                    [_][□][×]│
├──────────────────────────────────────────────────────────────┤
│  [Prepare Profile] [Filter Settings]        ← ページ切替タブ  │
├──────────────────────────────┬───────────────────────────────┤
│                              │  サイドパネル                  │
│   プレビューエリア            │  (ページごとに内容が変わる)    │
│   (Original|Filtered|Split)  │                               │
│                              │                               │
│   ┌──────────────────────┐   │                               │
│   │                      │   │                               │
│   │   動画プレビュー      │   │                               │
│   │                      │   │                               │
│   └──────────────────────┘   │                               │
│   フレームスライダー ◄──●──►  │                               │
├──────────────────────────────┴───────────────────────────────┤
│  ステータスバー (処理状態・プロファイル品質等)                  │
└──────────────────────────────────────────────────────────────┘
```

### Prepare Profile ページ

サイドパネルの内容:
```
┌─ Device Noise Profile ──────────────────────────┐
│ Device Name and Mode: [label / dbl-click edit]  │
│ Input Data Gain: [x1.0] ▼                       │
├─────────────────────────────────────────────────┤
│ [Auto Profile (Current Frame)]                  │
│ [Auto Profile (Most Noisy Frame)]               │
│ [Build Profile (Current Frame)]                 │
│ [Build Profile (Most Noisy Frame)]              │
├─────────────────────────────────────────────────┤
│ [Auto Fine-Tune]  [Auto Complete]               │
│                                                 │
│ Manual Fine-Tune:                               │
│ ┌───────────────────────────────────────┐       │
│ │  曲線グラフ (輝度 vs ノイズレベル)      │       │
│ │  9 ビン × スライダー                   │       │
│ └───────────────────────────────────────┘       │
├─────────────────────────────────────────────────┤
│ Overall Profile Quality: ████████░░ 82%         │
│ Profile-image match: Good                       │
│ [Profile Check]  [Tuning Guide]                 │
└─────────────────────────────────────────────────┘
```

### Filter Settings ページ

サイドパネルの内容 (アコーディオン式の折りたたみセクション):
```
┌─ Temporal Filter ───────────────────────────────┐
│ [✓] Enable Temporal Filter                      │
│ Temporal Radius:        [▼ 2]                   │
│ Temporal Noise Reduction Amount: ████░░ 100%    │
│ Temporal Quality Mode:  [Normal ▼]              │
│ ── Components ──                                │
│ Luminance Noise Level: ████░░ 100%              │
│ Cb Noise Level:        ████░░ 100%              │
│ Cr Noise Level:        ████░░ 100%              │
├─ Spatial Filter ────────────────────────────────┤
│ [✓] Enable Spatial Filter                       │
│ Spatial Quality Mode:   [Normal ▼]              │
│ ── Noise Reduction Amount ──                    │
│ ── Components ──                                │
│ Luminance:                                      │
│   Ultra Low:  █░░░░░░░   0%                     │
│   Very Low:   ██████░░░  60%                    │
│   Low:        ████████░  80%                    │
│   Mid:        █████████  90%                    │
│   High:       ██████████ 100%                   │
│ Cb: (同様の 5 スライダー)                        │
│ Cr: (同様の 5 スライダー)                        │
│ ── Noise Level ──                               │
│ Luminance:                                      │
│   Ultra Low:  ██████████ 100%                   │
│   Very Low:   ██████████ 100%                   │
│   ... (同様)                                     │
├─ Sharpening ────────────────────────────────────┤
│ [ ] Enable Sharpening                           │
│ (展開時に 5 バンドのスライダー表示)               │
├─ Jitter ────────────────────────────────────────┤
│ [ ] Enable Jitter Filter                        │
│ Jitter Threshold: ░░░░░░ 0%                     │
├─ Dust and Scratches ────────────────────────────┤
│ [ ] Enable                                      │
│ Mode: [Normal ▼]  Threshold: ░░░░░░ 0%          │
├─ Flicker ───────────────────────────────────────┤
│ [ ] Enable Flicker Filter                       │
│ Local: ░░░░░░ 0%  Full Frame: ░░░░░░ 0%         │
├─ Edge Smoothing ────────────────────────────────┤
│ Low: ░░░░ 0%  Mid: ░░░░ 0%  High: ░░░░ 0%       │
├─ Output ────────────────────────────────────────┤
│ Mix with original: ░░░░░░░░ 0%                  │
│ [Apply]                                         │
└─────────────────────────────────────────────────┘
```

### メニューバー

```
Profile                    Filter                     Tools
├─ Load Profile...         ├─ Load Filter Preset...   ├─ Preferences
├─ Save Profile As...      ├─ Save Preset As...       ├─ Batch Profiler...
├─ Reset Profile           ├─ Reset Filter Settings   ├─ Filter Settings Check
├─ Change Default...       ├─ Change Default...       ├─ Noise Level Check
├─ Copy Profile            ├─ Copy Filter Settings    └─ Profile Check
├─ Paste Profile           ├─ Paste Filter Settings
└─ Assign As Default       └─ Assign As Default
```

## 技術要件

### ファイル構成

```
repro/
├── nv_ui_qt/
│   ├── __init__.py
│   ├── main.py              # エントリポイント
│   ├── main_window.py       # メインウィンドウ (2 ページ切替)
│   ├── pages/
│   │   ├── __init__.py
│   │   ├── profile_page.py  # Prepare Profile ページ
│   │   └── filter_page.py   # Filter Settings ページ
│   ├── widgets/
│   │   ├── __init__.py
│   │   ├── preview.py       # プレビュー (Original/Filtered/Split/Assist)
│   │   ├── profile_panel.py # プロファイルサイドパネル
│   │   ├── filter_panel.py  # フィルタサイドパネル (アコーディオン)
│   │   ├── curve_editor.py  # AFT 手動調整用曲線エディタ (9 ビン)
│   │   ├── noise_graph.py   # ノイズグラフ (輝度 vs ノイズレベル)
│   │   └── sliders.py       # 共通スライダーコンポーネント
│   ├── menus.py             # メニューバー
│   ├── worker.py            # バックグラウンド処理スレッド
│   └── state.py             # アプリ状態管理 (undo/redo 履歴含む)
├── nvfilter.py              # (既存、変更なし)
├── nv_aft_v5.py             # (既存、変更なし)
└── nv_ui.py                 # (既存の tkinter 版、参考用として残す)
```

### 実装の注意点

1. **プレビュー**: QImage + QLabel で表示。Split モードは 2 つの QImage を 1 つの QPixmap に合成
2. **バックグラウンド処理**: QThread + signal/slot で UI をブロックしない
3. **Undo/Redo**: QUndoStack + QUndoCommand を使用
4. **スライダー**: QSlider + QLabel のセット。double precision 用には QDoubleSpinBox を併用
5. **曲線エディタ**: QGraphicsScene + QGraphicsPathItem でベジエ曲線を描画
6. **ノイズグラフ**: QPainter で棒グラフ or 折れ線グラフを描画
7. **AFT**: `nv_aft_v5.apply_aft()` を QThread で呼ぶ (15 秒程度かかるため)
8. **プリセット保存**: QSettings または JSON ファイル

### パラメータと既定値

```python
# V5_DEFAULTS (nvfilter.py から)
{
    "radius": 2,           # Temporal Radius
    "temporal": 1.0,       # Temporal Amount (0-1)
    "threshold": 4.0,      # Temporal Threshold (0.25-8)
    "spatial": 2.0,        # Spatial Wiener strength (0.25-4)
    "luma": 0.7,           # Noise Reduction Amount, Luminance (0-1)
    "chroma": 1.0,         # Noise Reduction Amount, Chrominance (0-1)
    "band_amounts": {      # Noise Reduction Amount per band (0-1)
        "ultra-low": 0.0, "very-low": 1.0, "low": 1.0,
        "medium": 1.0, "high": 1.0
    },
    "band_noise": {b: 1.0 for b in V5_BANDS},  # per-band Noise Level (0.25-4)
    "noise_level": 1.0,    # overall Noise Level (0.25-4)
    "channel_noise": {"Y": 1.0, "Cb": 1.0, "Cr": 1.0},
    "mix": 0.0,            # Mix with original (0-1)
}

# Neat Video の UI 表記との対応
#   Ultra Low  → "ultra-low"
#   Very Low   → "very-low"
#   Low        → "low"
#   Mid        → "medium"
#   High       → "high"
```

### Neat Video 本家のスライダー範囲

| パラメータ | UI 表記 | 範囲 | 既定 | v5 キー |
|---|---|---|---|---|
| Temporal Radius | ドロップダウン | 0-5 | 2 | radius |
| Temporal Amount | スライダー | 0-100% | 100% | temporal |
| Luminance Noise Level | スライダー | 0-200% | 100% | channel_noise.Y |
| Cb Noise Level | スライダー | 0-200% | 100% | channel_noise.Cb |
| Cr Noise Level | スライダー | 0-200% | 100% | channel_noise.Cr |
| Spatial Strength | (内部) | 0.25-4 | 2.0 | spatial |
| Luminance Reduction | スライダー | 0-100% | 70% | luma |
| Cb Reduction | スライダー | 0-100% | 100% | chroma |
| Ultra Low Reduction | スライダー | 0-100% | 0% | band_amounts["ultra-low"] |
| Very Low Reduction | スライダー | 0-100% | 100% | band_amounts["very-low"] |
| Low Reduction | スライダー | 0-100% | 100% | band_amounts["low"] |
| Mid Reduction | スライダー | 0-100% | 100% | band_amounts["medium"] |
| High Reduction | スライダー | 0-100% | 100% | band_amounts["high"] |
| Mix with original | スライダー | 0-100% | 0% | mix |

## 検収基準

1. `python -m nv_ui_qt.main` でアプリが起動する
2. 動画ファイルを開いてプレビューが表示される
3. Prepare Profile ページで Auto Profile が実行できる
4. AFT ボタンが動作する (曲線が変わる)
5. Filter Settings ページの全スライダーが機能する
6. スライダー変更 → プレビューが自動更新される
7. Process full video で mp4 が書き出せる
8. Undo/Redo が動作する
9. プリセットの保存・読み込みができる

## 参考ファイル

- 仕様書: `repro/ui_spec/UI_SPEC.md` (258 行、全 UI 要素の定義)
- 既存 tkinter UI: `repro/nv_ui.py` (378 行、バックエンド呼び出しの参考)
- v5 パイプライン: `repro/nvfilter.py` (V5_DEFAULTS、process_frame_v5、build_profile_v5)
- AFT: `repro/nv_aft_v5.py` (apply_aft)
- 動画 IO: `repro/denoise_video_v5.py` (probe、read_frames、encode)
