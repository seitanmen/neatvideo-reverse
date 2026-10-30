# Neat Video v6 本家ウィンドウの配置 (公開資料による)

調査日 2026-10-25。凡例: **[資料]** = 出典に書かれている / 画像に写っている事実、**[推測]** = 当方の解釈。

## 出典 (すべてこのフォルダに保存済み)
- S1: OFX 版ユーザーガイド v6.1 (2026-09-30、Windows 版) https://www.neatvideo.com/user-guides/nv6/NVOFXUG.pdf → `NVOFXUG.pdf` / テキスト `NVOFXUG.txt` / 埋め込み画像 `img/`・`small/`
  - 画面全体のスクリーンショット: `img/p10_1_1272x732.png` (Prepare タブ・プロファイル作成前)、`img/p14_0_1272x732.png` (= p42、プロファイル作成後。いずれも「Neat Video 6.1.0 Pro plug-in for OpenFX hosts」、Windows、Beginner Mode)
  - パネルの画像: `img/p19..p36_*`、`small/*`、まとめ画像 `m1.png`〜`m4.png`
- S2: Premiere / Resolve / After Effects 版ガイド v6.1 (`NVPRUG.pdf`・`NVRSUG.pdf`・`NVAEUG.pdf`、同じ URL 形式)。画面全体の画像は S1 と同じもの (`img_pr/` など)
- S3: DustBuster (OFX) 用クイックスタートガイド https://www.neatvideo.com/support/quick-start-guides/nv6/db
  (curl・WebFetch では HTTP 418 になるため、Playwright で本文と画像の URL を取得)。画像 `qs/db-1-09.jpg` (Prepare、Build Profile 後)・`qs/db-1-10.jpg` (**Adjust and Preview**)。どちらも「Neat Video 6.0.0 Demo plug-in for OpenFX hosts」、macOS、暗い配色
- 制約: 画面全体の画像はすべて Beginner Mode。Advanced Mode の画面全体は見つからず、Advanced のパネルは S1 の部分画像だけ

## 1. メインウィンドウ
[資料 S1 p10/p14, S3 db-1-09/10]
- タイトルバー: `Neat Video 6.1.0 Pro plug-in for OpenFX hosts (using 6 CPU cores and 1 GPU, auto)`
- メニューバー: **Effect · View · Profile · Filter · Tools · Help**。右端に **Beginner Mode** (Beginner Mode / Advanced Mode を切り替えるメニュー。S1 §5.6.2 の「右上隅」)
- その下にページタブが 2 つ: **Prepare Noise Profile** | **Adjust and Preview**。アイコン付き。Win 6.1 では左寄せ、Mac 6.0.0 では中央寄せ
- 本文は左右 2 分割。左 (幅約 77%) が「ツールバー → ビューア → ビューア下のバー」、右 (約 23%) が設定パネル
  - Prepare タブでは右パネルが **Device Noise Profile** 枠
  - Adjust タブでは右パネルが **Filter Settings** 枠
- ウィンドウ最下段は全幅の 1 行
  - 左から: `P▾` (Video Scan Type。Progressive/Interlaced)、`ℓ▾` (Input Data Gamma。ホストが linear を扱える場合だけ出る。S1 p11。Mac DustBuster の画像には無い)、`x1.0` (Input Data Gain▾。Auto Adjust と Help を含む)
  - 続いてフレームのサムネイル列 (11 枚)
  - 右端に **Cancel**、**Apply** (Apply が既定ボタン)
- 右パネルの下部の案内文: 「How to use: **Quick Start Guide**.」「Currently working in **Beginner Mode**. For more control switch to **Advanced Mode**.」(プロファイル作成前は 1 行目だけ)
- [推測] OK ボタンは無く、Cancel と Apply の 2 つだけ

## 2. ビューア
- ビューア上のツールバー
  - Prepare タブ [資料 p10/p14]: 左から `Auto Profile▾` (手動で選択範囲を描くと名前が **Build Profile▾** に変わる)、`Generic Profile`、`Load Profile`。右端の `Profile Check` (歯車アイコン) は、プロファイルができた後に表示される
  - Adjust タブ [資料 S3 db-1-10]: 左から 選択範囲/全体の切り替えアイコン、**Preview** (目のアイコン。押された状態で青く表示)、**Play All▾** (緑の再生アイコン)
  - Advanced Mode では、ツールバーに **Variants** ボタン、**Make a new variant** ボタン、変種ごとのボタン (最大 4 個、数字キーでも選べる) が加わる [資料 S1 §6.1。画像なし]
- 既定のビューアモードは **YCrCb+** [資料 p14、S1 p40 のメニュー画像]
  - 上段: 合成画像 (ビューアの高さの約 2/3)
  - 下段: 3 等分の **Y Enhanced / Cr Enhanced / Cb Enhanced**。各区画の右下にラベル
- ビューアモードのメニュー [資料 `img/p40_0`]
  - 見出し **Primary Modes**: `Normal`、`YCrCb + Enhanced Contrast (Default)`
  - 見出し **Channel Components**: `RGB (Red, Green, Blue)` (画像の切れ目。[推測] 他の成分や周波数成分の項目が続く)
  - 見出し **Viewer Split**: `2 x 2`、`1+3 Bottom`、`1+3 Right`
  - ここでの「Split」は成分の並べ方。入力と出力を左右に並べる比較表示は資料に無い
- 入力と出力の比較 [資料 S1 §5.6.1]: Adjust タブでプレビューが完成すると、各区画の右上隅に **"Filtered"** と出る。プレビュー区域の中を**クリックしている間だけ元フレームに戻る**
  - 手動で範囲を選ぶと、**その範囲だけを処理**する (速い)。全体を処理するのは範囲がないとき
  - [推測] 範囲を選んだときは、選択した矩形の中だけがフィルタ済みになる。既定の大きさは資料に記載がない
- ビューア下のバー [資料 p14]: 左から `50%▾` (ズーム)、`YCrCb +▾` (ビューアモード)、手のひら (パン)、`☼` とスライダー (表示の明るさ。リセット付き)、`W:128 H:128` (選択範囲の大きさ)、`R: G: B:` (カーソル位置の値)。右端に `Frame: 1920x1080, 32-bit float` と ⓘ
- ズームの操作 [資料 S1 p12]: ホイール、下のバーのズーム、Ctrl +/−/0/Alt+0。パンはスペースキー＋左ドラッグか中ボタンのドラッグ
- 選択範囲 [資料 p14]: 緑の正方形 (枠が太い)。上に `Noise Level: 12.2 (+0%)`、右下に ▾ の三角。Y/Cr/Cb の下段にも、同じ位置に灰色の矩形が出る
  - 大きさが閾値を超えるたびに枠の太さが変わる (S1 p45)
  - 大きさは 32〜512px、微調整用は 16px から
  - 測れる帯域の表示「Good (Low Freq)」などがある (S1 p44)
- サムネイル [資料 S1 §5.6.1・S3 db-1-10]
  - 現在のフレームは**青枠**。時間フィルタが使う近傍フレームは**オレンジ枠** (Radius 2 で前後 2 枚ずつ)
  - クリックするとそのフレームをビューアに表示する
  - 重複フレームには白の "Copy"、黄の "Rep" のラベルが付く
- Assist [資料]: 独立したモードではなく、Spatial タブの **Filter Tuning Assist** [▶ Begin]/Finish ボタン (調整の影響を強調して見せる)。同じ型の道具として Profile Check、Noise Level Check、Local Flicker Check、Repeated Frames Check がある。Check 系は青いマスクを表示する

## 3. Prepare Noise Profile ページ
[資料 p10/p14/db-1-09、S1 p40 の部品画像]
- ツールバー (ビューアの上): Auto Profile▾ / Build Profile▾ → Generic Profile → Load Profile …… (右寄せ) Profile Check
  - Auto Profile▾ のメニューに `Auto Profile (Current Frame)` がある。Shift を押しながらクリックしても同じ
- 右パネル **Device Noise Profile** の上から順
  1. アイコン行: 開く · 保存 | Undo▾ · Redo▾ · ✕ (リセット) | ピン (**Default Profile**▾。Save As Default Profile...) · ⓘ
  2. プロファイル名のドロップダウン。作成前は赤字の `profile not ready`、作成後は `Noise profile (1920x1080p) *`。開くと Profiles フォルダの一覧が出る
  3. `Device Name and Mode:` のテキスト欄 (`Unspecified device`)
  4. 情報欄: Frame size `1920x1080 p` / Scan type `Progressive` / Gamma `Linear` か `Gamma corrected` / Gain `x1.0` / Profiled with `Full frame (1920x1080)`
  5. `Quality [69%]` (横棒)
  6. 見出しの帯 **Profile Check and Tuning**
     - `Profile Check (?)` と [▶ Begin]
     - `Noise Level` スライダー `+0%` (Beginner Mode)
     - Generic Profile では次のとおり (`img/p46_1`):
       - `Generic Noise 25`
       - ▼Noise Level: Luminance ○ / Cr 赤● / Cb 紫● (`+0%`)
       - ▼Frequencies: High / Mid / Low / Very Low / Ultra Low (サムネイルの模様付き)
- Advanced Mode だけのもの [資料 S1 p44-45、`img/p45_2`。画面上の位置は画像なし]
  - 周波数成分ごとのノイズレベルのヒストグラム。緑 = 正確、黄・赤 = 不正確。Beginner では Profile > Profile Viewer で見る
  - **Noise Levels** 表示: Selection (選択範囲) と Main Sample (プロファイルの解析領域)
  - **ノイズプロファイルのイコライザ** (`img/p45_2`)
    - 縦軸 +200% / +100% / +0% / −100%。横軸の下に黒→白の輝度グラデーション
    - R・G・B の 3 本の折れ線。結び目が塗り潰しなら実測、黄色なら補間
    - 下のボタン: 表示の切り替え 3 つ | 全チャンネル・R・G・B | 品質 `78%` | ✕
  - **Manual Fine-Tune** ボタン (Profile > Fine-Tune Using Selected Area)、**Auto Complete** ボタン
  - [推測] これらは Device Noise Profile パネルの中に入る (S1 の文言「Noise Levels indicators in the Device Noise Profile panel」)
- Profile メニューにある項目: Auto Fine-Tune、Auto Profile (Current Frame)、Build Profile (Current Frame)、Change Default Profile...、Profile Viewer、Save As Default Profile... [資料]
- Input Data Gain はタブの中ではなく、ウィンドウ最下段の左にある `x1.0▾` [資料 p14・§8.2]

## 4. Adjust and Preview ページ (右パネル Filter Settings)
[資料 S3 db-1-10、S1 m1〜m4]
- 枠の上部
  - 見出し `Filter Settings`
  - アイコン行: 読み込み (フォルダ) · 保存 | Undo▾ · Redo▾ · ✕ | Default Preset · ⓘ (Preset Description)
  - プリセット名のドロップダウン (`default`)
  - タブ **Temporal | Spatial | General**。右端に `▸≡` のメニュー
- 値の表記: 右端の数値に点線の下線 (クリックで編集できる、[推測])。`+0%`、`40%`、`100%`、`off` (0%)、`2`、`+0`、`15`、`150%`、`75%`。dB は使っていない
  - 既定値から変えた行には右端に桃色の `⋈` が付く [推測: 既定値に戻すボタン]
- 各見出し行の右端に `Enable ☑`。Spatial の見出しにはリセット `↺` も付く

### Temporal タブ
- Beginner (`m1` 左): Temporal Filter Enable☑ / Radius `2` / ● Flicker Enable☐ / ● Dust and Scratches Enable☐
- Advanced (`img/p23_0`、閉じた状態)
  1. **Temporal Filter** Enable☑
     - Quality Mode `[Normal▾]` (Normal / High)
     - Radius `2`
  2. ▸ **Temporal Noise**。開いた状態は `m1`・`m4`。資料の説明順:
     - `Noise Level Check (?) [▶Begin]`
     - `Noise Level +0%`
     - Local Flicker (既定 0)・Frame Flicker (既定 0)・Local Flicker Check
     - ▼ `Noise Reduction Amount`: Amount (既定 100%)
  3. ▸ **Repeated Frames**
     - `Repeated Frames Check (?) [▶Begin]`
     - `Repeat Rate 0%`
     - 状態の文 `Full speed, no repeated frames`
  4. ▸ **Jitter of Details**: `Threshold (?) +0%`・`Jitter Extended Filtration Enable☐`
  5. ▸ **Dust and Scratches** Enable☐: Mode `[Normal▾]` (Normal / Aggressive) · Threshold `150%` · Amount `100%`
  6. **Adapt to Changing Noise** Enable☐ (行は 1 つだけ)

### Spatial タブ
- Beginner (`m1` 中央)
  - Spatial Filter Enable☑
  - ● Noise Reduction Amount: Luminance ○ `40%`
  - ● Noise Level `+0%`
  - ▸ Sharpening Enable☐ (Fine / Medium / Large)
- Advanced (`img/p31_0`、すべて開いた状態)
  1. **Spatial Filter** Enable☑ ↺
  2. `Filter Tuning Assist (?) [▶Begin]`
  3. Quality Mode `[Normal▾]`
  4. ▼ **Noise Reduction Amount**
     - Luminance ○ `40%`
     - ▼ Chrominance: Cr 赤● `80%`、Cb 紫● `80%` (画像の値。S1 本文は「輝度以外は既定 100%」と書いており一致しない。画像は変更後の値の可能性がある)
     - ▼ Frequencies: **High `100%` → Mid → Low → Very Low `100%` → Ultra Low `off`** (細かい帯域が上。各行にノイズの模様のサムネイル)
  5. ▼ **Noise Level** (見出し行にも全体のスライダー `+0%`)
     - Luminance / Cr / Cb `+0%`
     - ▼ Frequencies: High / Mid / Low / Very Low / Ultra Low `+0%`
  6. **Artifact Removal** Enable☑
     - Dots `+50%`、Lines `+50%`
     - ▼ Frequencies: High / Mid / Low `+0%`
  7. **Edge Smoothing** Enable: High / Mid / Low (画像なし。資料の説明順では Artifact Removal と Sharpening の間)
  8. **Sharpening** Enable☑
     - ▼ High `100%`・Range `+0`
     - ▼ Mid `off`・Range、▼ Low `off`・Range
     - ▼ Channels: Luminance `100%` / Cr `off` / Cb `off`
     - `Prevent Over-Sharpening Enable☑`
     - 強さの範囲は 0〜250%
- 成分の順は Luminance → Cr → Cb (Cb より Cr が先)。帯域の順は High → Ultra Low

### General タブ (`m3` 左、`m4`)
1. **Mix with Original** Enable☐: ▸ Mix `75%` (開くと Luminance / Cr / Cb `100%`。Advanced Mode のみ)
2. **Performance** 情報欄: Frame Size、Frame Bitdepth、Floating Point、Temporal Filter…、Using: CPU/GPU
3. 下部に [Check Speed] と [Preferences...]、速度の表示 `32.40 frames/sec`

## 5. スクリーンショットの詳細 (要約)
- p14 (1272×732、Win 6.1 Pro)
  - 選択範囲は (370,170)〜(452,250) 付近の緑の矩形。ラベル「Noise Level: 12.2 (+0%)」
  - 右パネルの上端 y≈95 が「Device Noise Profile」。Quality 69%
  - 最下段: y≈711 に P / ℓ / x1.0、サムネイル 11 枚 (6 枚目が青枠)、Cancel・Apply
- db-1-10 (1280×720、Mac 6.0.0 Demo、暗い配色)
  - タブ「Adjust and Preview」が黄色の枠で強調されている
  - ツールバー: [⛶] [Preview] [Play All▾]
  - 4 つの区画すべての右上に「Filtered」
  - サムネイルは 4・5・7・8 枚目がオレンジ枠、6 枚目が青枠
  - 右パネル「Filter Settings」の下に、プリセット「default」とタブ Temporal / Spatial / General

## 6. PySide6 再現への示唆 [推測]
- 右パネルの幅は約 300px で固定し、Qt の QToolBox や折りたたみ式のグループで「▼見出し＋右端の Enable チェック」を再現する
- スライダーの行: ラベル → (色の丸か帯域の模様) → スライダー → 点線下線の値
- ビューアの既定は YCrCb+ の 1+3 (下段に 3 区画)。比較は「押している間だけ元画像」
- 最下段: Scan/Gamma/Gain の小ボタン → サムネイル列 (青枠 = 現在、オレンジ枠 = 時間窓) → Cancel・Apply
