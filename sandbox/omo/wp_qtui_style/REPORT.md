# qtui の macOS 表示崩れの修正 (作業者 qtui-style、2026-10-31)

転記と要約: 統括。経過の全文は `PROGRESS.md`、独立検証の判定は `sandbox/omo/coord_style/VERDICT_style.md` にある。

## きっかけ
ユーザーの指摘は「タブの位置や ▽ マークの位置が表示崩れしている」。統括が本家 (Resolve の中の Neat Video 6.1.3 Pro) と qtui の実際のウィンドウを `screencapture -l` で撮り比べ、違いを 6 つ見つけた。撮影は `ref/` にある。

## 原因 (観測)
- qtui はスタイルを指定していなかった。そのため、実際の画面 (cocoa) では macOS のスタイル、offscreen では Fusion になっていた。
- 自己試験と比較ツールは offscreen で動くので、この崩れを見落としていた。

## 対応
- 新規の `qtui/src/app/look.{h,cpp}` に `apply_look()` を置いた。中身は、Fusion を土台にした proxy style と、本家の画素から取った灰色のパレット (メニューバー 58 / タブの行 72 / ページ 89)。
- `main.cpp` と自己試験から呼ぶ。

違い 1〜6 の修正:
1. **ページのタブ:** 全幅の暗い行の中央に置き、選択中のタブが持ち上がって見えるようにした。
2. **メニュー付きボタンの ▾:** ボタンの内側の右下に描く。当たり判定は Fusion のまま変えていない。
3. **選択欄の ▾:** 2 重を 1 つにした。ボタンの text() は変えていない。
4. **プロファイル名の欄:** 右端に ▼ を描く。
5. **メニューバー:** `setNativeMenuBar(false)` でウィンドウの中に置いた。「Advanced Mode」は印なしの太字。
6. **全体:** 灰色のボタン・スライダーにし、ビューアの背景を 58 にした。

ユーザーの決定による追加:
- フレーム情報は、深度 32 のときだけ "Frame: WxH, 32-bit float" にした (単体のアプリと OFX のホストモードの両方)。8/10/12/16 は "N-bit" のまま。
- 新しい背景で読めなかった文字色を 5 か所直した。フレーム番号 #606060→#b4b4b4、"(?)"、下の階層の "(n/a)"、"profile not ready"、メモリ上限の表示。

## 確認 (観測)
- **作業者:**
  - 自己試験は 116/116 (`frameInfoFloat32` を追加し、" float" を外す変異で FAIL することを確かめた)。
  - `g1_compare --cases default` は ALL SAME、`compare_export` は ALL SAME。
  - 警告 0。
  - `qtui_style_grab` (cocoa で show せずに grab) で前後を撮った。
- **統括:**
  - 修正版を実際のウィンドウで開き、`screencapture -l` で撮った。
  - 6 か所が本家の形になり、「Advanced Mode」が右上にあることを確かめた。
- **独立検証:** **APPROVE** (M 0・L 3・I 6)。主な観測:
  - 自己試験 116/116、書き出しの照合 2 本はどちらも ALL SAME。
  - 当たり判定は素の Fusion と同じ。
  - メニュー 82 項目とショートカットは不変。
  - 文字のコントラスト比は 1.1〜1.5 から 2.4〜3.4 に上がった。
  - OFX のホストモードで "32-bit float" が出る。

## 残り (記録)
- L-1: メニューバーに薄い線 (階調 51・58) が出る。本家には無い。
- L-2: "profile not ready" の赤のコントラストが 2.37 で、直した色の中で最も低い。
- L-3: look.h の注釈に誤りがある。
- I-2 (推測): ネイティブのメニューバーをやめたので、macOS のアプリメニューから Preferences・About が消え、⌘, が効かなくなるとみられる。Tools (F10) と Help には残る。
- I-6 (変更前から): OFX のホストモードの深度の欄が "processing 32-bit" で、単体のアプリの表記と違う。
- 手を付けていない本家との差: アイコン、右パネルの題字入りの枠、Quality の棒。

---
## 追加: "n/a" の対応 (ユーザーの依頼、2026-10-31)

### イコライザの行の欄 (`complete_lbl`)
- 変更前は "n/a" だった。本家は同じ場所に 85% を出している。
- `ProfilePanel::set_completeness` で、パネルの Completeness と同じ値 (g×100 の %、プロファイルがなければ "n/a") を出すようにした。
- 本家の欄は割合の分だけ明るく塗った棒の形で、この見た目は未対応。
- 試験: `equalizerCompleteness` を新しく足し、既存の 3 項目にも確認を足した。自己試験は 117/117。表示の 1 行を消すと 3 項目が FAIL する。

### "n/a" の棚卸し (全 24 か所。分類の全文は `PROGRESS.md` §3.3)
**(a) 値があって表示するだけで済むもの**
- イコライザの行の欄 1 件だけで、上のとおりこの PR で直した。

**(b) 本家の機能そのものが未実装のもの**
処理・測定・ホストとの連携を変えることになるので、この PR では実装しない。
- Filter Settings の次の行
  - Quality Mode
  - Sharpening
  - Jitter
  - Dust and Scratches
  - Flicker (局所・全面)。CLI の `--flicker` はあるが意味が違い、UI につなぐには処理を変える必要がある
  - Edge Smoothing
  - Repeat Rate
  - Adapt to Changing Noise
  - Artifact Removal
  - Mix の成分別
- Local Flicker Check と Repeated Frames Check
- Manual Fine-Tune
- プロファイルの Noise Level の 5 帯域
- P・γ・x1.0
- Variants
- Auto Match
- Profile Converter (.dnp)
- Skip Neat Video Processing

**(c) そのまま残すもの**
- プロファイルが無いときの代わりの表示 "n/a"。本家にも、条件付きで "n/a" を出す表示がある。
- 本家での中身を観測していない Device Name and Mode のメニューと Save As Default Profile…。
- Help の外部リンク。

### Completeness の値 (観測と推測を分ける)
**観測**
- test_v02 の Auto Profile → AFT の Quality / Completeness は、次の経路ですべて同じ値になった。
  - 今の C++
  - G1 より前の C++
  - PySide6 版 (native と純 Python)
- 比べたフレームと値
  - 作業者: 6 フレーム。n=240 は 47/19
  - 独立検証者: 作業者が使っていない n=400 で 49/22
- したがって G1 による変化はない。
- AFT の後の Completeness は「未測定のビンの数 ÷ 27」になる。

**本家の値と未確定の点**
- 本家の同じ場面は Quality 65% / Completeness 85% (= 23/27)。
- 差の原因は**未確定**。
  - repro で見本を 512² にしても 19%、2 倍に拡大した入力でも 15% で、確かめられなかった。
  - 候補 (推測) は 2 つ。本家が実際に使った見本の領域が違うことと、AFT の移植と本家の違い (移植が本家と照合済みなのはタイル順だけ) 。

### 訂正: "Profiled with: Full frame (WxH)"
- 作業者と統括は当初、この表示を「見本 = フレーム全面」と読み、85% の説明にも使った。
- 本家バイナリの文字列では、"Profiled with:" に続く選択肢は "Full frame (" / "Two fields (" / "One field (" の 3 つで、直前に "Scan type: Progressive / Interlaced" がある (観測)。
- したがって、この欄はプログレッシブのフレームかインターレースのフィールドかを表すものとみられ (推測)、見本の領域は表さない。
- repro はプログレッシブだけを扱うので、今の "Full frame (WxH)" という表示は本家と同じ意味になり、変更しない。

### 独立検証 Round 2
- 判定は **APPROVE** (M 0・L 1・I 3)。
- 主な観測
  - パネルとイコライザの行の値は、12 の場面ですべて一致した。
  - 自己試験は 117/117、`g1_compare` は ALL SAME。
  - "n/a" の (a) に取りこぼしはない。
- L-R2-1 (観測と推測を分けて書くこと) は、この節で対応した。
