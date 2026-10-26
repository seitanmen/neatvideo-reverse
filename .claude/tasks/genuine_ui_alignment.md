# タスク: 本家 UI との完全整合 (優先順位付き実装計画)

ユーザーが Resolve 上の本家 6.1.3 Pro を観測した結果をもとに、再現 UI を本家に完全に合わせる。
計画は統括が作成し、ユーザーに提示済み。

## 実行順序

### Phase 0: ユーザー協力 (ブロッカー)

 Resolve のタイムラインを 1920×1080 に変更する
 (現在 3840×2160 のため本家の Noise Level 数値と再現の数値を直接比較できない)
 → ユーザーに依頼、完了後に Phase 1 を開始

### Phase 1: 本家の画面を観測して UI に反映 (項目 1-4)

- Resolve で本家を操作して撮影・観測:
  - Noise Level の数値照合 (1080p 変更後)
  - View / Filter / Tools メニューの中身 (クリック撮影 or バイナリ静的解析)
  - Profile Check 中のフレーム切替・マウス操作の表示
  - 窓を小さくしたときのパネルのスクロール挙動
  - 選択範囲の枠の色 (min(w,h) で赤 ≤15 / 黄 ≤127 / 緑)
  - ズームのメニュー段 (33.3% 等)
- パネル幅・(?) アイコン・ボタンの絵を本家に合わせる
  (本家のアイコン画像は NeatLab の著作物なので使用しない)

### Phase 2: 静的解析で式を読んで移植 (項目 5-10)

処理結果は変わらないため、安全に実行できる:

- 項目 5: Quality の計算式 (本家 23%) → プロファイル欄に値を書き込む関数を特定
- 項目 6: 完了度 (本家 0%) → イコライザが測った値の数から完了度を出す
- 項目 7: 帯域の棒の値、イコライザの緑枠・赤枠 → 描画関数を読む
- 項目 9: 表示のディザ (16384 個の乱数表、種 0x9) → 見た目 ±0.25 の差
- 項目 10: Assist Image の作り方 → Profile Check 中のフィルタ設定をたどる

### Phase 3: 処理結果が変わるもの (ユーザーの判断待ち)

- 項目 11: イコライザを R/G/B にする (プロファイル調整値を Y/Cr/Cb → R/G/B に変更)
- 項目 12: Frequencies の 5 帯域 Noise Level (プロファイルの帯域比に掛ける)
- 項目 13: Luminance 既定値 40% (本家 40% = 内部 70% 換算の要検討)
- 項目 16: フレームスライダーの要否 (本家はホストのタイムライン使用)

### Phase 4: 大規模作業 (別セッション)

- 項目 8: Manual Fine-Tune (選択範囲でイコライザ調整、aft_port を使う)
- 項目 14: Variants (設定候補の比較機能)
- 項目 15: Sharpening・Jitter・Dust & Scratches・Flicker・Edge Smoothing・品質モード

## 参照ファイル

- 本家観測: `sandbox/omo/coord/genuine_ui/OBSERVED.md`・`shots/`
- 静的解析: `sandbox/omo/coord/genuine_ui/STATIC.md`
- 仕様: `sandbox/omo/coord/genuine_ui/SPEC.md`
- 検証: `sandbox/omo/coord/genuine_ui/VERDICT.md` (APPROVE)
- レイアウト: `sandbox/omo/coord/verify_qtui_layout/LAYOUT.md`
- AFT 静的移植: `neatvideo_ofx_614/analysis/noise/aft_port/`
