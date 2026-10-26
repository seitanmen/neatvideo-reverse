# Neat Video v6 (OFX 6.1.4 Demo, arm64) ウィンドウ構成の静的復元

対象: `neatvideo_ofx_614/NeatVideo6.arm64.ofx`。静的解析だけで調べた (読み込み・実行はしていない)。保護・難読化コードは扱っていない。
アドレスは arm64 スライスの VM アドレス (= ファイルオフセット)。関数名は `analysis/functions.csv` の FUN_xxxx。

## 方法 (再現手順)
- `strings -t x` → `strings.txt` / UI 部分は `uistr.txt`
- `/tmp/nv_expand/disasm_full.txt` (otool) の adrp+add を追って、__cstring への参照 21,698 件を「関数 / 命令アドレス / 文字列アドレス / 文字列」の表 `xrefs.txt` にした (スクリプト `../scripts/xref.py`)
- 既存のデコンパイル結果 `prof_0xb0934.c` / `prof_0xc74e0.c` / `prof_0xe0b18.c` / `prof_0xdec00.c` から、ビルダー呼び出しの骨格を `skel_b0934*.txt` に取り出した (`../scripts/skel.py`)
- 凡例: **[観測]** = 参照順・引数・定数を直接読んだもの。**[推定]** = 観測からの推論。Qt は名前空間 `NeatNV6Space` に静的リンクされていてシンボルがないため、addWidget などの呼び出しは名前で特定できない。順序の根拠は「uic の setupUi が objectName を設定する順」と「ビルダー関数の呼び出し順」である。

## 1. メインウィンドウ
uic の setupUi `FUN_00098f98` (フォーム名 `filtrationTaskEditorForm`) と、retranslateUi `FUN_000a5d60`。

**[観測] objectName の順 (setupUi)**
```
centralwidget / centralHorizontalLayout
 editorWidget / editorWidgetLayout
  tabsWidget / tabsWidgetLayout
   modeTabs (QTabWidget)
     inputTab   > inputPage    icon :/icons/small/InputPage.png    0x99478..
     profileTab > profilePage  icon :/icons/small/ProfilePage.png  0x99704..
     previewTab > previewPage  icon :/icons/small/PreviewPage.png  0x9994c..
     outputTab  > outputPage   icon :/icons/small/OutputPage.png   0x99b94..
 bottomWidget / horizontalLayout                                   0x99e1c
   inputDataFormatWidget: scanTypeButton(:/icons/small/ProgressiveDark.png), gammaButton(:/icons/small/Gamma.png),
                          gammaWarningIconWidget, gainButton, gainWarningIconWidget
   lineMiddle
   thumbsWidget: thumbsListView (showDropIndicator, isWrapping)
   buttonsWidget: actionSetupButton, cancelButton, applyButton    0x9ab5c / 0x9abd8 / 0x9ac58
   gripWidget
 mainMenu
```
**[観測] retranslateUi のタブ名 (`FUN_000a5d60`、この順)**: "Open Input Image" (0xa5df8) / **"Prepare Noise Profile"** (0xa5e70) / **"Adjust and Preview"** (0xa5ee8) / "Save Output Image" (0xa5f60)。
続いて "Video Scan Type"・"Progressive"・"Input Data"・"Input Data Gain"・"x3/4" と、ツールチップ "Do not apply any changes and return" + **"Cancel"**、"Apply filtration and return" + **"Apply"**。
- [推定] OFX 版で見えるタブは Prepare Noise Profile と Adjust and Preview の 2 枚。Input/Output のタブ、thumbs、actionSetupButton ("Action Setup"、ツールチップは Photoshop について) は Neat Image と共通のコードで、OFX では隠すとみられる。隠すコードは特定していない。
- [観測] 下段左の 3 ボタン:
  - スキャン型: Progressive / Interlaced
  - ガンマ: "Input Data Gamma" で Gamma Corrected / Linear
  - ゲイン: 選択肢は x0.25 / x0.33 / x0.5 / x0.66 / x0.75 / **x1.0** / x1.33 / x1.5 / x2.0 / x3.0 / x4.0、ほかに "Auto Adjust" と "Auto Adjust (%1)" (0x12fbf90〜、0x12fa8e3)
  - メッセージ文が "(x1.0) in the left bottom corner of Neat Video window" と書いており、下段左にあることと合う (0x12faf77)
- [観測] Apply の表記は "Apply%1"、ツールチップは "Apply changed filter settings%1and return" (`FUN_0009d374`)。
- [観測] ウィンドウ題名の部品: " - "・"%n CPU core(s)"・" and "・"%n GPU(s)"・", auto"・"using "・" (processing skipped)" (`FUN_000a2b7c`)。[推定] 題名は「… using N CPU cores and M GPU(s)」の形。
- [観測] 専用のステータスバーは見当たらない (QStatusBar の objectName・文字列はない)。代わりに HTML の表 "Using: / CPU: %1 %2 / GPU: …" がある (`FUN_000c2fc4`)。[推定] 性能タブの表示。

### 1.1 各ページの共通の骨組み (`FUN_00089184`、フォーム `commonEditorPage`)
**[観測]** 次の順:
```
topLineWidget/topLine
bodyWidget/hboxLayout
  leftLayout
    toolbarContainerWidget      (ページのツールバー)
    variantToolbarContainer/variantToolbar
    viewerFrame                 (ビューア)
  sidePanelContainerWidget      (サイドパネル)
```
[推定] ビューアが左で、サイドパネルが右。左の列は上からツールバー → バリアントのツールバー → ビューアと並ぶ。サイドパネルはスタイルシートで `CommonSidePanel` と呼ばれる。

## 2. メニューバー (`FUN_00074430`、作成順) [観測]
- **E&ffect**
  - Open Input Image... / &Close Input Image / Select All / Save Output Image As...
  - Copy Profile and Preset (Shift+Ctrl+C) / Paste Profile and Preset (Shift+Ctrl+V)
  - Apply / Cancel / &Skip Neat Video Processing
- **&Task**
- **&Edit**: Copy Output Image / Paste Input Image
- **&View**
  - &Zoom 子メニュー: 倍率の一覧 (Ctrl+Alt+0 は 100%)・Zoom to Fit (Ctrl+0)・Zoom In (Ctrl++ と Ctrl+=)・Zoom Out (Ctrl+-)
  - &Viewer Mode 子メニュー:
    - Primary Modes: Ctrl+1・Ctrl+2
    - Channel Components: Ctrl+3・Ctrl+4
    - Frequency Components: Ctrl+5・Ctrl+6・Ctrl+7
  - Viewer Split: Next Split Mode (Ctrl+]) / Previous Split Mode (Ctrl+[)
  - &Navigator: &Top Left Corner / &Bottom Left Corner / Show Navigator (F8)
  - Show Preview (Alt+H、ツールチップ Toggle Auto Preview)
  - Toggle Panning Mode (Ctrl+D)
  - Use &Alpha Channel
  - Start Playback (Ctrl+P) / Stop Playback (Current Frame) / Stop P&layback (Initial Frame)
- **&Profile**
  - Undo (Ctrl+Z) / Redo (Shift+Ctrl+Z と Ctrl+Y)
  - &Load Profile... / &Save As... / &Reset Profile / Copy Profile / Paste Profile
  - Save As Defa&ult Profile... / Assign As Defa&ult Profile / Reset Defaul&t Profile / C&hange Default Profile...
  - Auto Match (Ctrl+M) / Auto Profile (Most Noisy Frame) (F2) / Auto Profile (Current Frame) (Shift+F2) / Auto Profile with Calibration Target (F3) / Smart Profile (F5) / Generic Profile (F6)
  - Auto Fine-Tune (Ctrl+F) / Fine-Tune Using Selected Area (Ctrl+T) / Auto Com&plete / R&eset Fine-Tuning Results
  - Profile Viewer... (Ctrl+Shift+I) / Profile Check (Ctrl+K)
- **F&ilter**
  - Undo / Redo
  - &Load Filter Preset... / &Save As Filter Preset... / &Reset Filter Settings / Copy Filter Settings / Paste Filter Settings
  - Save As Defa&ult Preset... / Assign As Defa&ult Preset / Reset Defaul&t Preset / C&hange Default Preset... / Preset &Description...
  - F&ilter Tuning Assist / Filter Settings &Check
- **Va&riants**: Show Variants (F7) / &New / &Remove / Copy Noise &Profile from / Copy Fi&ter Settings from
- **T&ools**
  - Calibration &Target... / &Batch Profiler... / &Profile Converter... / Tas&k Thumbs
  - Preferences... (F10) / Performance tab (Shift+Ctrl+P) / Optimizer (Shift+Ctrl+O)
- **&Help**
  - Help (F1) / &Contents / Read &Me / What's &New / &Quick Start Guide / H&ow to Use Neat Image
  - &Web Page / Community &Forum / Report a &Bug Online / &Support via E-mail
  - About... (Alt+F1) / &Register... / &Purchase / &Upgrade / &Updates
- 最後に **&Advanced Mode / &Beginner Mode** (0x7c640〜)。"Mode" という文字列もある (`FUN_0007e7a0`)。[推定] メニューバーの隅に置くモード切替のボタンかメニュー。

## 3. ビューア
### 3.1 ビューアのツールバー (`FUN_000f19c4` setupUi、`FUN_000f6e90` retranslate、クラス `ViewerToolbar`) [観測] 左から
1. **zoomButton**: 表記 "100%"、ツールチップ "Adjust zoom level of image viewer"
2. **viewerModeButton**: 表記 "Normal"、ツールチップ "Adjust image viewer mode"
3. dragButton (Pan.png): "Toggle Viewer Panning Mode …"
4. alphaButton (Alpha.png): "Toggle Viewer Alpha Channel"
5. buttonsLine (区切り線)
6. levelAdjustmentWidget: levelAdjustmentSlider ("Adjust viewer brightness") と resetLevelAdjustmentButton (Brightness.png、"Reset viewer brightness")
7. levelsPresetLine
8. whWidget: "W:" と "H:" (選択範囲の幅・高さ)
9. whLine
10. rgbWidget: "R:" "G:" "B:" (ポインタ位置の画素値)
11. sizeLeftLine
12. sizeWidget: 画像サイズとビット深度。例 "Two fields: 99999x99999, 8 bits/channel"
13. sizeLine
14. isoWidget: "ISO: 200"
15. isoLine
16. infoButtonWidget: imageInfoButton (InfoWhite.png、"Additional information about image")

- [観測] 倍率表: 13 段 (`FUN_00054ce8` が 13 を返す。`FUN_0005490c` が 0xc94070 の double 表を引く)。
  5 / 10 / 12.5 / 16.7 / 20 / 25 / 33.3 / 50 / **100** / 200 / 300 / 400 / 800 %。表示書式は "%g%%" で、×1000 を丸めて /10。ほかに Zoom to Fit。
- [観測] ビューアのモード (`FUN_0009258c`。短い名前と長い名前の 2 列がある):

  | 短い名前 | 長い名前 (メニュー) | 内部名 | 区分・ショートカット |
  |---|---|---|---|
  | Normal | Normal | ModeNormal | Primary Modes, Ctrl+1 |
  | YCrCb + | YCrCb + Enhanced Contrast | ChannelYCrCbLowSum | Primary Modes, Ctrl+2 |
  | RGB | RGB  (Red, Green, Blue) | ChannelRGB | Channel Components, Ctrl+3 |
  | YCrCb | YCrCb  (Luminance, Chrominance) | ChannelYCrCb | Channel Components, Ctrl+4 |
  | Y Freq | Y Channel Frequencies | ChannelYFreq | Frequency Components, Ctrl+5 |
  | Cr Freq | Cr Channel Frequencies | ChannelCrFreq | Frequency Components, Ctrl+6 |
  | Cb Freq | Cb Channel Frequencies | ChannelCbFreq | Frequency Components, Ctrl+7 |

  ショートカットの割り当て (Ctrl+1〜7 がこの表の順) は推定。
  - 分割レイアウト: "2 x 2" (2x2) / "1+3 Bottom" (1+3b) / "1+3 Right" (1+3r)
  - 各ペインのラベル (`FUN_00042c44`): Normal, R, G, B, Y, Cr, Cb, Y High/Mid/Low, Cr High/Mid/Low, Cb High/Mid/Low, Y/Cr/Cb Enhanced
  - [推定] "Viewer Split" は元画像とフィルタ結果の分割ではない。成分モードで「主画面 + 3 成分」を 2×2・下に 3 つ・右に 3 つのどれで並べるかの切替である。**v6 には元画像とフィルタ結果を左右に並べる分割表示の文字列が見当たらない。**
- [観測] ビューア上の文字: "Original" (`FUN_00055f28`)・"Updating - " (`FUN_00051b10`)・"Final Result" (`FUN_0008f428`)。
  プロファイル領域のラベル (`FUN_0004b0c0`) は "Not Uniform: " と "Clipping: "。
- [観測] ナビゲータ (縮小図) は navigatorSidePanel / navigatorFrame / zoomViewNavigator。置き場所は左上か左下 (ShowNavigatorL.png・MoveNavigatorTL/BL.png)。
- [未確定] ViewerToolbar がビューアの上下どちらにあるか。作成箇所を特定していない。

### 3.2 Preview の矩形 [観測と推定]
- 文字列 (`FUN_000ce2a0`): "Select a preview area in the center of the viewer.\nYou can also double-click any part of the image to select a preview area there."。
  ビューアの右クリックメニューに "&Select Preview Area Here\tDouble Click" がある (0x12fc18f、`FUN_000aa190` 付近)。
- 切替ボタン toggleAutoPreviewButton: 表記 "Preview"、ツールチップ "Show preview/original image"。メニューでは "Show Preview" (Alt+H)。
- `FUN_0004bd08` が selectionChanged → 遅延タイマー → onDelayedSetPreviewRect をつなぐ。
- [推定] 全画面の上に置いた選択矩形 (preview area) の中だけを処理して表示し、Preview の切替で元画像に戻す。矩形の既定の大きさは静的には特定していない (それらしい文字列も、Preferences の項目も見当たらない)。

## 4. Prepare Noise Profile ページ
### 4.1 ツールバー (`FUN_000eb7fc` setupUi、`FUN_000ed358` retranslate) [観測] 左から
- buttonsWidget:
  1. autoProfileButton "Auto Profile" ("Automatically build a device noise profile using the input image")
  2. loadProfileButton "Load Profile"
  3. autoMatchButton "Auto Match"
  4. genericProfileButton "Generic Profile"
- rightButtonsWidget (右寄せ):
  1. profileCheckButton "Profile Check" (TuningAssist.png)
  2. compareButton "Variants" ("Compare several variants of noise profiles")
- 接続 (`FUN_000ed0f4`): Auto Profile のメニューから autoBuildProfileWithMultiFrames と autoBuildProfileWithCurrentFrame を呼ぶ。
- ビューアの右クリックメニュー (`FUN_000d442c`): "&Select Area Here\tDouble Click" / "Build Profile (Most N&oisy Frame)" / "Build Profile (&Current Frame)" / "&Manual Fine-Tune"。
  Profile Check 中は "Show Original" と "Profile Check" (`FUN_000d4b34`)。
- 選択領域の大きさの表示 (`FUN_000d1120`): "Size: Too small to profile" / "Size: Fine-tuning only" / "Size: Small (High Freq)" / "Size: Small (Mid Freq)" / "Size: Good (Low Freq)" / "Size: Good (Very Low Freq)" / "Size: Good (Ultra Low Freq)" / "Size: Best (Ultra Low Freq)"

### 4.2 サイドパネル (`FUN_000d99d4` setupUi、`FUN_000ddf88` retranslate) [観測] 上から
1. **deviceNoiseProfileGroupBox "Device Noise Profile"**
   - profileNameWidget の topButtonsLayout: profileOpenButton → profileSaveButton → undoButton → redoButton → resetButton (Trash) → defaultButton (PinProfile) → profileInfoButton (Info)
   - profileNameEdit
2. **noiseLevelScrollPanel**: スクロールする本体。中身はビルダーが作る (下記)
3. qsgTextLabel (Quick Start Guide への誘導文) と modeCommentLabel ("Currently working in Beginner Mode…")

- 別のフォーム deviceNameModePanel (`FUN_000d86d0`/`FUN_000ddd34`): "Device Name and Mode:"・navigateButton・deviceNameEdit ("Optional device name (e.g., 'Nikon D90')")・deviceModeEdit。[推定] グループ内のプロファイル名の近くに置く。
- 既定の表示 (`FUN_000d8ec8`): "profile not ready" / "Build a noise profile by analysing a uniform area or load a ready-made profile."

**スクロール部の中身**

Advanced (`FUN_000e0b18`) [観測] (作成順):
1. ページ "Profile Tuning" (`FUN_00750130`)
2. **profileStatsPanelAdvanced** (`FUN_000e820c`→`FUN_000e84d8`/`FUN_000ea530`)
   - Quality "100%" / Match "100%"
   - "Main Sample" と "Selection" の値 (例 "55.5")
   - "Noise Levels": 帯域ヒストグラム。objectName の順は hf → ulf → mf → vlf → lf だが、配置は gridLayout
3. **equalizerPanel** (`FUN_000e5ce4`→`FUN_000e5d90`/`FUN_000e79d8`)
   - equalizerGroupBox: equalizer (ノイズと輝度のグラフ)
   - tuneButtonsWidget: autoFineTuneButton "Auto Fine-Tune" → manualFineTuneButton "Manual Fine-Tune" → autoCompleteButton "Auto Complete"
   - line1
   - channelButtonsWidget: rgbButton → redButton → greenButton → blueButton
   - fineTuningLabel ("100%"、ツールチップ Overall completeness…)
   - resetButton ("Reset all fine-tuning")
4. **"Profile Check and Tuning"** (グループ、`FUN_00039518`、flag 0x19)
   - "Profile Check" (Begin/Finish の調整ボックス、0x30)
   - "Generic Noise" (0x58。Generic Profile のときだけ)。スライダー "Generic Noise" は 1〜50、**既定 25** (0x12453b8 = 25.0f)
   - **"Noise Level"** (0x5b):
     - 見出しのスライダー "Overall Noise Level" (−100〜150、刻み 5/20)
     - Luminance → **Cr → Cb**
     - "Frequencies": High → Mid → Low → Very Low → Ultra Low
     - 既定値は全部 1.0 (0x12453bc〜0x12453dc)。[推定] 内部の倍率 1.0 で、表示は +0%
5. 案内文 "Generic noise profile is now in its default state. …" (`FUN_0003c454`)

Beginner (`FUN_000dec00`) [観測]:
1. "Profile Tuning"
2. profileStatsPanelSimple (`FUN_000dfe1c`→`FUN_000dfec8`/`FUN_000e07dc`: Quality / Match)
3. "Profile Check and Tuning"
4. Profile Check
5. Generic Noise
6. "Noise Level" (0x59): "Overall Noise Level" だけ

## 5. Adjust and Preview ページ
### 5.1 ツールバー (`FUN_000cbb5c` setupUi、`FUN_000ce2a0` retranslate) [観測] 左から
- buttonsWidget:
  1. putSelectionButton (PutSelection.png。ツールチップは §3.2 の preview area の文)
  2. playStopButton "Play All" (Preview.png、"Start/Stop Playback")
  3. toggleAutoPreviewButton "Preview" ("Show preview/original image")
  4. cacheLowWarningWidget "Cache Too Small" (警告アイコン付き)
- rightButtonsWidget:
  1. filterSettingsCheckButton "Filter Settings Check" (TuningAssist.png。ホイール操作に反応する)
  2. compareButton "Variants" ("Compare variants of preview …")
- 再生ボタンのメニュー (`FUN_000cb260`): &Play / Stop (&Current Frame) / Stop (Initia&l Frame)。範囲は "All" か "&%1 Frames" (onPlayActionWithRadiusTriggered)。
- Filter Settings Check のメニュー (`FUN_000ac4e4`・`FUN_000ae230`): 書式 "&%1. %2" で番号付き
  1. Noise Level Check
  2. Local Flicker Check
  3. Repeated Frames Check
  4. Spatial Filter Tuning

  最後に "&Finish"。プレビューの種類は "Preview" (" (Default)" 付き)・"Assist Image"・"Filtered" (`FUN_000aa990`・`FUN_000abdf4`・`FUN_000d489c`)。

### 5.2 サイドパネル (`FUN_000bff98` setupUi、`FUN_000c64f0` retranslate、フォーム previewPageSidePanel) [観測]
```
filterSettingsGroupBox "Filter Settings"
  presetNameWidget / topButtonsLayout:
     presetOpenButton → presetSaveButton → undoButton → redoButton → resetButton(Trash) → defaultButton(PinPreset) → presetInfoButton(InfoPreset)
  presetNameEdit
filterSettingsWidget / filterSettingsTabs (QTabWidget)
  temporalTab    : temporalScrollPanel,    temporalQSGTextLabel,    temporalModeCommentLabel     → タブ名 "Temporal"
  spatialTab     : spatialScrollPanel,     spatialQSGTextLabel,     spatialModeCommentLabel      → タブ名 "Spatial"
  performanceTab : performanceScrollPanel, performanceQSGTextLabel, performanceModeCommentLabel  → タブ名 "General"
```
- ボタンのツールチップ: "Load filter settings from a preset..." / "Save current filter settings as a preset..." / "Undo last change to filter settings" / "Redo …" / "Reset preset (to Default Preset selected in Preferences)" / "Default Preset is used …" / "Description of current filter preset"

### 5.3 タブの中身: Advanced (`FUN_000b0934`。作成順 = 表示順と推定)
ビルダーの部品 (引数の型から推定):
- `FUN_00750130(パネル, "…Settings")`: ページ
- `FUN_00039518`: チェックボックス付きの最上位セクション (flag 0x5d・0x59・0x19)
- `FUN_00038ca4`: 子グループ (0x5b = 展開できる / 0x5f = 有効化チェック付き / 0x30・0x31 = Begin/Finish の調整ボックス / 0x58 = 条件付き)
- `FUN_0003aa60`: スライダー行 (名前・アイコン・例の画像・ツールチップ・最小/最大・刻み/ページ・設定キー名)
- `FUN_0003abb8`: グループ見出しに付くスライダー
- `FUN_0003bb84`: コンボ・ラジオ
- `FUN_0003b2d4`: Radius のコンボ
- `FUN_0003ad40`: チェックボックス
- `FUN_0003c454`: 警告のラベル
- `FUN_0003c6d4`: 区切り (推定)

範囲は引数を 32 ビットずつに分けたもの。例 0x96ffffff9c → 最小 −100・最大 150、0x1400000005 → 刻み 5・ページ 20。

**Temporal タブ (ページ "Temporal Settings")**
1. [x] **Temporal Filter** ("Enable Temporal Filter")
   - Quality Mode: Normal (Faster) / High (Slower) — "Temporal Quality Mode"
   - Radius: "1 frame"〜"5 frames" (値 1〜5) — "Temporal Radius"
2. **Temporal Noise** (0x5b)
   - [Noise Level Check: Begin/Finish] (0x30)
   - Noise Level: −100〜150、5/20 — "Temporal Noise Level"
   - [Local Flicker Check] (0x30)
   - Local Flicker: 0〜50、1/5 — "Local Flicker Threshold"
   - Frame Flicker: 0〜50、1/5 — "Full Frame Flicker Threshold"
3. **Noise Reduction Amount** (0x5b)
   - Amount: 0〜100、5/20 — "Temporal Noise Reduction Amount"
4. **Repeated Frames** (0x5b)
   - [Repeated Frames Check]
   - Repeat Rate: 0〜100、1/20。説明文 "Zero speed…" / "Full speed…" / "Reduced speed, %1% of repeated frames"
   - 警告 "Radius is too small for current settings"
5. **Jitter of Details** (0x5b)
   - Threshold: −100〜300 — "Jitter Threshold"
   - [ ] Jitter Extended Filtration
6. [x] **Dust and Scratches** (0x5f)
   - Mode: Normal / Aggressive
   - 警告 (Radius 不足)
   - Threshold: 0〜500、5/20
   - Amount: 0〜100、5/20
7. [ ] **Adapt to Changing Noise** (ページ直下のチェック)

**Spatial タブ (ページ "Spatial Settings")**
1. [x] **Spatial Filter** ("Enable Spatial Filter")
   - [Filter Tuning Assist: Begin/Finish] (0x31)
   - Quality Mode — "Spatial Quality Mode"。文字列は Normal / High / **Highest** と Normal (Faster) / High (Slower)。呼び出しで渡す個数は 2
2. **Noise Reduction Amount** (0x5b)
   - Luminance: 0〜100、5/10 — "Luminance Reduction Amount"
   - **Chrominance** (子グループ): **Cr** → **Cb** (0〜100)
   - **Frequencies** (子グループ): High → Mid → Low → Very Low → Ultra Low (0〜100、5/10)
3. **Noise Level** (0x5b)
   - 見出しのスライダー "Overall Noise Level" (−100〜150)
   - Luminance → **Cr → Cb** (−100〜150、5/20)
   - **Frequencies**: High → Mid → Low → Very Low → Ultra Low
4. [x] **Artifact Removal** (0x5f)
   - Dots / Lines (−100〜150)
   - Frequencies: High / Mid / Low
5. **Edge Smoothing**: High / Mid / Low (0〜100)
6. [x] **Sharpening** (0x5f)
   - High / Mid / Low の子グループ。見出しのスライダーが Amount (0〜250)、中に Range (−1〜+1)
   - Channels: Luminance → Cr → Cb (0〜250)
   - [ ] Prevent Over-Sharpening

**General タブ (ページ "General Settings")**
1. [x] **Mix with Original**
   - Mix (見出しのスライダー 0〜100)
   - Luminance → Cr → Cb (0〜100)
2. **Performance** (0x59)
   - Check Speed / Preferences...
   - "Current filter settings that affect processing speed the most"
   - "Processing speed achieved with current filter settings"

注意 (観測):
- **Spatial では Noise Reduction Amount が Noise Level より先に作られる** (Advanced も Beginner も同じ)。
- 色差はどこでも **Cr が Cb より先** (Noise Level・Reduction Amount・Sharpening・Mix・ビューアのモード)。
- Luminance / Cr / Cb を列に並べる形ではない。どの項目も**縦に並ぶスライダー行**で、子グループで入れ子になる。

### 5.4 タブの中身: Beginner (`FUN_000c74e0`) [観測]
- **Temporal settings**
  - [x] Temporal Filter: Radius (1〜5 frames)
  - [x] Flicker (0x5d)
  - [x] Dust and Scratches
- **Spatial settings**
  - [x] Spatial Filter: Noise Reduction Amount (Luminance、0〜100)
  - Noise Level (見出しの Overall Noise Level、−100〜150)
  - [x] Sharpening: Fine / Medium / Large (0〜250)
- **General settings**: Performance (Check Speed / Preferences...)
- Beginner / Advanced の説明文 (HTML) も出す。

## 6. 単位・既定値
- [観測] 単位を表す文字列は "%" 系だけ:
  - クラス `PercentLabel` とその書式 "%1%"
  - "  off  " (スライダー部品 `FUN_00039d5c`)。[推定] 0 のときの表示
  - **"dB" はない**
- [観測] 静的な既定値は、Profile ページのスライダーのもの (表 0x12453b0〜) だけ取れた: Generic Noise 25、Noise Level の倍率は全部 1.0。
  - Filter ページの既定値は、`__common` (0x155b8xx) か実行時のオブジェクト (Beginner の lVar4+…) を指していて、静的には読めない
  - AGENTS.md §3 の .nfp の観測を併記する: Noise Reduction Amount 0.2 / 0.8、Temporal Radius 2 / 5
- [観測] 各スライダーの第 2 の浮動小数引数: Noise Level 系 2.5、Amount 系 1.0、Flicker 50、Jitter 4、D&S Threshold 5、Range 2.0。意味は未確定 (表示値と内部値の変換係数か)。

## 7. ホスト側 (OFX パラメータ、`FUN_0000a22c`) [観測]
- ページ "Controls"
  - 押しボタン "Prepare Noise Profile" (Hint "Prepare a noise profile to enable the filters in this instance of the effect")
  - 押しボタン "Adjust Filter Settings"
- 文字列 "Prepare Profile..." / "Adjust Filters..." もある。

## 8. 未確定のもの
- Preview 矩形の既定の大きさと初期位置
- ViewerToolbar の上下の位置
- OFX で Input / Output タブを隠す処理
- profileStatsPanel の基準要素 0xd6c0 の正体
- Spatial の Quality Mode に Highest が出る条件
- Filter の既定値の実数値

## 付属物
- `xrefs.txt` (文字列の参照表)
- `skel_b0934.txt` / `skel_b0934_full.txt` (ビルダーの骨格)
- `strings.txt` / `uistr.txt`
- `png/` (バイナリに埋め込まれた PNG 734 枚。rcc の名前との対応は付けていない)
