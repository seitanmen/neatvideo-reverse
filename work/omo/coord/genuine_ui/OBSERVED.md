# 本家 Neat Video 6.1.3 Pro (DaVinci Resolve 21 上) の UI 観測 — 2026-10-26

統括が Resolve で開かれていた本家プラグインの窓を screencapture で撮影し、cliclick で操作して記録した (Apply・Cancel は押していない)。
画像は `shots/` (3600×2250 の全画面。窓は 0,30 から 1800×1095 pt)。比較用のこちらの UI の画像は `ours_before/` (test_v02 のフレーム 0、offscreen の grab)。

注意 (観測):
- Resolve のタイムラインは 3840×2160 で、test_v02 (1920×1080) を拡大して渡している ("Frame: 3840x2160, 32-bit float")。そのため数値 (Noise Level 等) は 1080p のときと同じではない。
- 再生ヘッドは 01:00:00:00 (フレーム 0)。

## 窓全体
- タイトル: "Neat Video 6.1.3 Pro plug-in for Resolve (using 12 CPU cores and 1 GPU, auto)"。
- メニュー: Effect View Profile Filter Variants Tools Help。右端に "Advanced Mode" (切り替え)。
  - Effect: Copy Profile and Preset ⇧⌘C / Paste Profile and Preset ⇧⌘V / Skip Neat Video Processing / Apply ↵ / Cancel。
  - Profile: Undo Last Change ⌘Z / Redo Last Change ⇧⌘Z / Load Profile… / Save As… / Reset Profile / Copy Profile ⌘C / Paste Profile ⌘V /
    Save As Default Profile… / Assign As Default Profile / Reset Default Profile / Change Default Profile… /
    Auto Profile (Most Noisy Frame) F2 / Auto Profile (Current Frame) ⇧F2 / Generic Profile F6 / Profile Viewer… ⇧⌘I。
  - Help: Help F1 / Read Me / What's New / Quick Start Guide / How to Use Neat Video / Web Page / Community Forum / Report a Bug Online / Support via E-mail / About… ⌥F1。
  - View・Filter・Variants・Tools は撮影できなかった (メニューの切り替えが cliclick で働かなかった)。
- ページのタブ: "Prepare Noise Profile" | "Adjust and Preview" (プロファイルがないと Adjust は無効)。
- 最下段: 左に "P▾"・"γ▾"・"x1.0▾"、中央にサムネイル 11 枚 (青枠 = 現在 / プロファイルに使ったフレーム、Adjust ではオレンジ = 時間窓)、右に Cancel・Apply (プロファイルがないと Apply は無効)。
- ビューアの下のバー: ズーム "33.3%▾"、表示モード "YCrCb +▾"、手のひら、明るさのアイコンとスライダー、"W:512 H:216" (選択範囲の大きさ)、"R:237.31 G:225.06 B:176.12" (カーソル下の値、0〜255 尺度で小数 2 桁)、右に "Frame: 3840x2160, 32-bit float" と (i)。

## 表示モード (ビューアのモードメニュー)
- Primary Modes: Normal / YCrCb + Enhanced Contrast (Default)
- Channel Components: RGB (Red, Green, Blue) / YCrCb (Luminance, Chrominance)
- Frequency Components: Y Channel Frequencies / Cr Channel Frequencies / Cb Channel Frequencies
- Viewer Split: 2 x 2 / 1+3 Bottom / 1+3 Right (Normal では無効)
- 画面:
  - Normal = 1 画面。
  - それ以外 = 上が合成 (RGB)、下に 3 画面。ラベルは右下: "Y Enhanced"・"Cr Enhanced"・"Cb Enhanced" / "R" "G" "B" / "Y" "Cr" "Cb" / "Y High" "Y Mid" "Y Low"。
- 見た目:
  - YCrCb+: Y Enhanced は細部が浮き出るエンボス状の強い局所コントラスト。Cr はピンク、Cb は藤色の同様の画像。
  - YCrCb: Y は灰色。Cr はピンク系、Cb は藤色系の単色で、色差の値に応じた明るさ (中間の灰色付近の低コントラスト)。
  - RGB: R はピンク、G は緑、B は藤色の単色。
  - Frequencies: High/Mid/Low とも灰色の中間値まわりの帯域画像 (Low ほど強い)。
- 下の 3 画面は、合成画面より少し大きい倍率でフレームのほぼ全体を写し、選択範囲を灰色の枠で示す。
- Prepare と Adjust でモードは別々に保持される (Adjust に移ると Normal)。

## Prepare Noise Profile ページ
- ツールバー:
  - 左: [Build Profile▾ / Auto Profile▾] [Generic Profile] [Load Profile]。
  - 右: [Profile Check] (プロファイル作成後に出る) [Variants]。
  - ボタン名は、選択範囲がないと "Auto Profile"、あると "Build Profile" に変わる。
- 選択範囲:
  - 合成画面上のドラッグで作る長方形 (正方形に限らない)。各辺は最大 512 px で、ドラッグを始めた点が角になる。小さいもの (W:30 H:18) も受け付ける。
  - 範囲の内側をドラッグすると、大きさを保って移動する。合成画面を (ドラッグせず) クリックすると選択が消える。
  - 枠の色: 通常は緑の太枠、小さいと黄色、プロファイル作成後は青。
  - 枠の上に "Noise Level: 4.8" (ドラッグ中もその場で更新)。
  - 枠の下に、場合によって黄色の "Not Uniform: Y, Cr, Cb" (均一でない成分の列挙) と (i)。
  - 枠の右下に ▽ があり、メニューは "Build Profile (Most Noisy Frame)" / "Build Profile (Current Frame)"。
- 右パネル "Device Noise Profile":
  - アイコン: 開く・保存・Undo▾・Redo▾・×・ピン▾・(i)。
  - プロファイル名のコンボ: "profile not ready" (赤) → "Noise profile (3840x2160p) *"。
  - "Device Name and Mode:" と ▸≡、名前の欄 ("Unspecified device")。
  - 情報: Frame size / Scan type / Gamma / Gain / Profiled with ("Full frame (3840x2160)")。
  - Quality (43%)、Noise Levels: Main Sample 7.2 / Selection 7.2。
  - 帯域のバー: High / Medium / Low / Very Low / Ultra Low (緑の棒、見本の模様付き)。
  - イコライザ:
    - +200%〜−100%。線は R・G・B の 3 本で、点は 9 個 (AFT の後は曲がる)。下に輝度のグラデーション。
    - 下のボタン: 3 つ (Auto Fine-Tune 系とみられる)、[RGB][R][G][B] の選択、"90%" の欄、×。
  - "Profile Check and Tuning": ⚙ Profile Check (?) [▶ Begin]、▸ Noise Level スライダー "+0%"。
- Build Profile を押すと、その場でプロファイルを作り、AFT の後とみられる曲線を表示する。Apply が有効になり、Adjust タブが有効になる。

## Adjust and Preview ページ
- ツールバー:
  - 左: [範囲選択のアイコン] [◉ Preview (押された状態)] [▶ Play All▾]。
  - 右: [⚙ Filter Settings Check▾] [Variants]。
- 合成画面の右上に "Filtered"。
- 右パネル "Filter Settings":
  - アイコン (開く・保存・Undo・Redo・×・ピン・(i))、"Filter preset (program's default settings)"、タブ Temporal / Spatial / General と ▸≡。
  - Temporal タブ:
    - Temporal Filter Enable ✓、Quality Mode Normal、Radius 2。
    - ▸ Temporal Noise、▸ Repeated Frames、▸ Jitter of Details、▸ Dust and Scratches (Enable)、Adapt to Changing Noise (Enable)。
  - Spatial タブ:
    - Spatial Filter Enable ✓、⚙ Filter Tuning Assist (?) [▶ Begin]、Quality Mode Normal。
    - ▾ Noise Reduction Amount: Luminance ○ 40%、▸ Chrominance、▸ Frequencies。
    - ▸ Noise Level +0%、▸ Artifact Removal Enable ✓、▸ Edge Smoothing Enable ☐、▸ Sharpening Enable ☐。
  - General タブ:
    - Mix with Original Enable ☐ / ▸ Mix 75%。
    - Performance: Frame Size 3840x2160 progressive、Frame Bitdepth 32-bit float、Floating Point: Fast processing、Temporal Filter、Spatial Filter (Frequencies: High, Mid, Low, Very Low; Artifact Removal: Enabled)、Using CPU/GPU。
    - [Check Speed] [Preferences...]。
