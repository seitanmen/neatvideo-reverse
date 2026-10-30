# nv_ui_qt を本家の配置に寄せる設計 (2026-10-25)

根拠: `uiresearch/web/LAYOUT_WEB.md` (公式ユーザーガイド v6.1 と DustBuster クイックスタートの画像)。
主な画像: `web/img/p14_0_1272x732.png` (Prepare 全体)、`web/qs/db-1-10.jpg` (Adjust and Preview 全体)、
`web/img/p31_0_288x610.png` (Spatial Advanced)、`web/img/p23_0_288x268.png` (Temporal Advanced)、
`web/img/p36_0_288x456.png` (General)、`web/img/p46_1_290x223.png`/`p17_0` (Profile Check and Tuning)、`web/img/p45_2_*` (イコライザ)、`web/m1..m4.png`。
実装者は着手前にこれらの画像を Read で見ること。

## 変えないもの (厳守)
- バックエンド (`nvfilter.py`・`nv_aft_v5.py`・`denoise_video_v5.py`) と `nv_ui_qt/backend.py`・`export.py` の処理内容。既定設定で `process_frame_v5` とビット一致、書き出しが `backend.process_frame` とビット一致のまま。
- 既存の PARAM_SPECS のキーと意味 (プリセット互換)。新しいキーを足すのは可 (下記)。
- Undo/Redo・QThread・子プロセス AFT・取り消し・後始末 (D1〜D10 の修正) の挙動。

## 1. ウィンドウ全体
- メニュー: **Effect · View · Profile · Filter · Tools · Help**。右端 (setCornerWidget) に `Advanced Mode ▾` (Beginner Mode / Advanced Mode)。既定は Advanced。Beginner では本家の Beginner と同じ行だけ残す (§4)。
  - Effect: Open Video... (repro)、Apply (= Process Full Video...)、Save Preview PNG (repro)、Cancel (= 閉じる)。
  - View: ビューアモード (Normal / YCrCb + Enhanced Contrast (Default) / Channel Components: RGB, Y, Cr, Cb)、Viewer Split (2 x 2 / 1+3 Bottom / 1+3 Right)、ズーム (Fit, 25, 50, 100, 200%)。
  - Profile / Filter / Tools / Help: 現行の項目を維持。Profile に Auto Fine-Tune、Auto Profile (Current Frame)、Build Profile (Current Frame) を入れる。
- メニュー下にページタブ (アイコン付き、左寄せ): **Prepare Noise Profile** | **Adjust and Preview**。
- 本文: 左 = [ビューア上のツールバー → ビューア → ビューア下のバー]、右 = 設定パネル (幅 約 300px、QSplitter で左が伸びる)。
- 最下段 (全幅 1 行): `P▾` (Scan type、Progressive 固定で無効 n/a)、`x1.0▾` (Input Data Gain、無効 n/a)、**サムネイル列** (11 枚、現在フレームを中心。現在 = 青枠、時間フィルタの窓 (±Radius) = オレンジ枠、クリックでそのフレームへ)、右端に **Cancel**・**Apply**。
  - Apply = 全編の書き出しダイアログ (現行の Process Full Video)。Cancel = ウィンドウを閉じる (本家はホストに戻る。ツールチップに明記)。
  - 本家はホストのタイムラインで移動するのでスライダーは無いが、単体アプリなのでサムネイル列の上か下に細いタイムラインのスライダーとフレーム番号を置く (repro と表示)。
- ステータスバーは現行のまま (進捗・書き出しの取り消し・メモリ上限の表示)。

## 2. ビューア
- 上のツールバー
  - Prepare: `Auto Profile ▾` (メニュー: Auto Profile (Current Frame) / Auto Profile (Most Noisy Frame))。選択範囲を描いた後はボタン名が `Build Profile ▾` に変わり (メニュー: Build Profile (Current Frame) / (Most Noisy Frame))、選択を解除すると戻る。続いて `Generic Profile` (= 既定プロファイルに戻す。本家の汎用プロファイルは無いので現行の Reset/Default Profile に割り当て、ツールチップに明記)、`Load Profile`。右寄せで `Profile Check` (プロファイルがあるとき表示)。
  - Adjust: 選択範囲/全体の切り替えアイコン、`Preview` (トグル、押された状態が既定)、`Play All ▾`。
- 表示 (既定 **YCrCb +** の 1+3 Bottom): 上段に合成 RGB (高さ約 2/3)、下段に 3 等分の **Y Enhanced / Cr Enhanced / Cb Enhanced** (右下にラベル)。
  - Enhanced Contrast は repro の定義でよい (例: 各成分を表示中フレーム全体の 1〜99% 分位でストレッチし、ノイズが見えるようにする)。定義はコードのコメントとツールチップに書く。
  - Viewer Split 2x2 (合成 + 3 成分)、1+3 Right (右に縦 3 つ)。Normal = 合成だけ。Channel Components = その成分を 1 面で。
  - 4 区画はズーム・パンを共有する。
- 入力と出力の比較 (本家どおり): Adjust タブで Preview が押されていてフィルタ結果ができたら、各区画の右上に **"Filtered"** を表示。ビューアの中でマウスの左ボタンを押している間だけ元フレームに戻す (押している間は "Filtered" を消す)。
  - 旧 Split (左右比較)・Assist は表示モードから外す。Assist は Spatial タブの **Filter Tuning Assist [Begin]/[Finish]** のトグルで、押している間は現行の差分強調表示を出す。
- プレビュー範囲: Adjust タブで選択範囲 (正方形/矩形) があるときは、その範囲だけを処理し (現行の Preview Area の crop 経路)、範囲の外は元画像のまま表示。範囲がないときは全体を処理する。旧「Preview Area」チェック (中央 50%) は、ツールバーの切り替えアイコンの「範囲がなければ中央 50% を自動で選ぶ」動作に置き換えてよい (その場合 Preferences の値を使う)。
- Prepare タブ: 常に元フレーム。選択範囲は**緑の太枠の正方形**、上に `Noise Level: 12.2 (+0%)` のラベル (値は repro の推定: 選択範囲の Y の σ を現行の推定関数で。何を表示しているかをツールチップに書く)。下段の 3 区画にも同じ位置に灰色の枠。プロファイルの解析領域 (現行の region 表示) も残す。
- ビューア下のバー: `ズーム▾`・`表示モード▾`・明るさ ☼ スライダー (表示だけ)・`W: H:` (選択範囲の大きさ)・`R: G: B:` (カーソル下の値)・右端に `Frame: 1920x1080, 8-bit`。

## 3. Prepare Noise Profile の右パネル「Device Noise Profile」(上から)
1. アイコン行: 開く・保存 | Undo・Redo・✕ (Reset Profile) | Default Profile ▾ (Assign/Change Default) · ⓘ (Profile Viewer = 現行の Profile Check ダイアログでよい)。Undo/Redo は QUndoStack の action を使う。
2. プロファイル名のドロップダウン: 無いときは赤字 `profile not ready`、あるときは `Noise profile (1920x1080p)` (未保存の変更があれば ` *`)。
3. `Device Name and Mode:` の QLineEdit (現行の device 名。編集は Undo 可能に)。
4. 情報欄 (枠付き): Frame size / Scan type (Progressive) / Gamma (Gamma corrected) / Gain (x1.0) / Profiled with (`Area 128x128 at (x, y), frame N` か `Full frame`)。
5. `Quality [ n/a ]` の横棒 (無効、現行どおり n/a)。
6. 帯の見出し **Profile Check and Tuning (?)**: `Profile Check (?) [▶ Begin]` (現行の Profile Check ダイアログ) と **Noise Level +0%** スライダー。
   - Noise Level はプロファイル全体の倍率 (新しい ProfileState の値 `tune`、既定 1.0、範囲 −100〜+200% を倍率 0.0〜3.0 ではなく 0.2〜3.0 に制限してよい)。effective_profile で曲線全体に掛ける。Undo 可能・JSON に保存 (古い JSON は 1.0 として読む)。
7. ここから Advanced Mode だけ:
   - **Noise Levels**: Selection / Main Sample の値 (repro の推定)。
   - **イコライザ** (Manual Fine-Tune): 現行の CurveEditor を本家の見た目に寄せる: 縦軸 +200% / +100% / +0% / −100% (表示は (係数−1)·100%)、横軸の下に黒→白の輝度グラデーション、**3 本の折れ線を同時に表示** (本家は R/G/B だが当方の係数は Y/Cr/Cb なので Y=白・Cr=赤・Cb=紫。順は Luminance, Cr, Cb)、結び目は編集済み = 塗り、未編集 = 黄色。下にボタン: All / Y / Cr / Cb (編集対象)、`Auto Fine-Tune`、`Auto Complete`、✕ (係数を 1 に)。
   - ノイズ曲線グラフ (現行の NoiseGraph) と Profile mode (repro) の選択はイコライザの下に「Repro」小見出しで残す。
8. 下端の案内文: `How to use: Quick Start Guide.` / `Currently working in Advanced Mode. For more control switch to Beginner Mode.` (モードに合わせて文言)。Quick Start Guide は現行の Tuning Guide を開く。

## 4. Adjust and Preview の右パネル「Filter Settings」
- 上部: アイコン行 (読み込み・保存 | Undo・Redo・✕ (Reset Filter Settings) | Default Preset ▾ · ⓘ)、プリセット名 (`default`、読み込んだらファイル名、変更があれば ` *`)、タブ **Temporal | Spatial | General**。
- 行の形式: 見出し行は灰色の帯「▼名前 …… Enable ☑」(折りたたみ可)。値の行は「ラベル → (色の丸 or 帯域模様) → スライダー → 右端の値」。値は点線の下線付きで、クリックかダブルクリックで数値入力 (QDoubleSpinBox をポップアップ等)。表記: Amount 系 `40%`・0 は `off`、Noise Level 系は相対 `+0%` (= 値 − 100)、Radius は `2`。既定から変わった行は右端に小さなリセットボタン (⋈ 相当) を出す。
- 色の丸: Luminance 白、Cr 赤、Cb 紫。帯域の行は小さなノイズ模様のサムネイル (帯域ごとのスケールの乱数模様を生成、repro)。
- 成分の順は Luminance → Cr → Cb、帯域の順は High → Mid → Low → Very Low → Ultra Low (細かい方が上)。

### Temporal タブ (上から)
1. **Temporal Filter** [Enable ☑ = t_enable]: Quality Mode [Normal▾] (n/a)、Radius (t_radius、0〜5 のスライダー)
2. ▸ **Temporal Noise**: Noise Level Check [Begin] (n/a)、Noise Level `+0%` (n/a: v5 の時間側に独立したノイズレベルは無い。Spatial の Noise Level が両方に効く旨をツールチップに)、Local Flicker / Frame Flicker (n/a)、▼ Noise Reduction Amount: Amount (t_amount)
3. ▸ **Repeated Frames**: Repeated Frames Check [Begin]・Repeat Rate (n/a)
4. ▸ **Jitter of Details**: Threshold (jitter_thr)・Extended Filtration (jitter_enable) (n/a)
5. ▸ **Dust and Scratches** [Enable = dust_enable]: Mode・Threshold・Amount (n/a)
6. **Adapt to Changing Noise** [Enable] (n/a)
- 既存の flicker_* キーは 2 の Local/Frame Flicker に割り当てる。新しい n/a のキーは足してよい (implemented=False)。

### Spatial タブ (上から)
1. **Spatial Filter** [Enable ☑ = s_enable] ↺ (Spatial の全設定を既定に戻す)
2. **Filter Tuning Assist (?)** [▶ Begin]/[Finish] (現行 Assist 表示のトグル。repro の差分強調)
3. Quality Mode [Normal▾] (s_quality、n/a)
4. ▼ **Noise Reduction Amount**: Luminance (amt_Y)、▼ Chrominance: Cr (amt_Cr)、Cb (amt_Cb)、▼ Frequencies: High / Mid / Low / Very Low / Ultra Low
   - Frequencies は本家と同じく全成分共通: 1 本動かすと bamt_Y_b・bamt_Cb_b・bamt_Cr_b を同じ値にする (1 回の Undo)。成分で値が違うときはスライダーに Y の値を出し、値の欄に `mixed` などを表示。
5. ▼ **Noise Level** (見出し行にも全体スライダー = adv_noise_level を相対 +0% 表記で): Luminance / Cr / Cb (noise_Y / noise_Cr / noise_Cb)、▼ Frequencies: High〜Ultra Low (bnoise_*_b を全成分共通で、4 と同じ扱い)
6. **Artifact Removal** [Enable]: Dots・Lines・▼ Frequencies High/Mid/Low (n/a、新キー)
7. **Edge Smoothing** [Enable]: High / Mid / Low (edge_*、n/a)
8. **Sharpening** [Enable = sharp_enable]: High / Mid / Low (sharp_high, sharp_medium, sharp_low) と Range (n/a)、▼ Channels、Prevent Over-Sharpening (sharp_prevent)。全部 n/a。
9. (Advanced Mode のみ) ▸ **Per-component bands (repro)** (既定は閉): 現行の成分 × 帯域のスライダー (bamt・bnoise)。4:2:0 の Cb/Cr High は無効表示のまま。
10. (Advanced Mode のみ) ▸ **Advanced (repro)**: adv_threshold・adv_spatial・adv_similarity。

### General タブ
1. **Mix with Original** [Enable ☐]: ▸ Mix (本家の意味: 高いほどフィルタの効果が多い。表示値 = 100 − 内部の mix。Enable が外れていれば内部 mix は 0 = フィルタのみ)。新キー `mix_enable` (既定 False) を足し、backend_params は `mix if mix_enable else 0` を使う。既定のビット一致は保つこと。Enable を入れたときの既定の表示値は 75% (内部 mix 25)。古いプリセット (mix_enable なし) は mix > 0 なら Enable とみなす。
2. **Performance** の情報欄: Frame Size、Frame Bitdepth (8-bit)、Floating Point (float32)、Temporal Filter (Enabled/Disabled、Radius)、Spatial Filter、Frequencies (Amount > 0 の帯域)、Using: CPU n cores (jobs)。
3. [Check Speed] (現在のフレームを 1 回処理して frames/sec を下に表示。プレビューと同じスレッドの仕組みで) と [Preferences...]。

### Beginner Mode
- Prepare: Advanced 専用 (§3 の 7) を隠す。
- Temporal: Temporal Filter (Radius だけ)、Flicker [Enable]、Dust and Scratches [Enable]。
- Spatial: Spatial Filter、Noise Reduction Amount の Luminance、Noise Level (全体)、Sharpening [Enable] (Fine / Medium / Large、n/a)。
- General: Mix with Original (Mix だけ)、Performance。
- 隠すだけで値は保持する。

## 5. 試験
- `selftest.py` を新しい配置に合わせて更新する。既存 33 項目の意図はすべて残す (既定のビット一致・成分別・全スライダーが backend に効く・無効項目・Undo/Redo・プリセット・AFT の各項目・プロファイル保存・Build Profile・Most Noisy・書き出しのビット一致など)。
- 追加: (a) 右パネルの見出しと行の順序が本設計どおり (Temporal・Spatial・General、Luminance→Cr→Cb、High→Ultra Low)、(b) Frequencies が 3 成分を同時に書き 1 回で Undo できる、(c) mix_enable の有無と古いプリセットの読み込み、(d) プロファイルの Noise Level (tune) が曲線に掛かり Undo でき JSON に残る、(e) 押している間だけ元画像 (Filtered ラベルの出入り)、(f) YCrCb+ の 4 区画が描画され、Split の切り替えで配置が変わる、(g) サムネイル列の青枠/オレンジ枠が Radius に追従し、クリックでフレームが変わる、(h) Beginner/Advanced の切り替えで行が隠れ値は保持、(i) Build Profile ▾ への名前の変化。
- 画面は出さない: `QT_QPA_PLATFORM=offscreen` だけ。出力先は scratchpad。
- 本家バンドルを読み込むもの・nvh は使わない。`repro/out/` に書かない。make は実行しない。
