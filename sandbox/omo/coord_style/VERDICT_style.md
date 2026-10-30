# VERDICT — qtui の見た目の修正 (worktree n3fix-style、基点 53a52e1、未コミットの差分)

判定: **APPROVE** (M なし。L 3 件・I 6 件は直さなくてもマージしてよい)

検証者: verify-style (独立検証)。2026-10-31。
作業場所: `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/vstyle/`
(以下 `$V`)。ソースは編集していない。変異試験は `$V/mut/` に qtui を写して、その写しだけを書き換えた。

## 1. 再実行 (観測)

| 項目 | 結果 |
|---|---|
| `make -C native lib` | Nothing to be done (最新) |
| qtui の新しいビルド先 (`$V/build`) でクリーンビルド | rc 0、`warning` 0 件 |
| `qtui/build_main` のインクリメンタルビルド | 警告・エラーなし |
| `QT_QPA_PLATFORM=offscreen NV_TEST_VIDEO=…/test.mp4 qtui/build_main/qtui_selftest` | **116 passed, 0 failed** (`$V/selftest.log`) |
| `g1_compare.py $V/g1 --before …/g1cpp_before/qtui/build --cases default` | **RESULT: ALL SAME** (test・test_v02 とも frame・curve/sigma・quality・export・params SAME) |
| `NV_NATIVE=1 compare_export.py $V/exp --exe <wt>/qtui/build_main/qtui_export` | **RESULT: ALL SAME** (e3/e4/g1〜g5 を含む全件) |

## 2. 差分の読み (観測と推測を分ける)

- 見た目以外の変更は、frame_info の " float" (例外 1) と色 (例外 2) だけ。ほかの変更は配置・色・スタイル・試験。
  - `set_frame_info` は、既定値 false の引数を 1 つ足しただけ。
  - ボタンの `text()`、メニューの項目、ショートカット、設定の保存は、差分の上では変わっていない。
  - 中央ウィジェットの入れ子を変えた (`central` → `tabrow` + `page`)。`centralWidget()->layout()` に依存するコードは無い (grep)。
- **MenuButtonPopup の当たり判定 (観測)**。自作のプローブ `$V/probe/probe.cpp` で確かめた。製品には入れていない。libnvui.a にリンクし、`QWidget::grab` は show せずに使う。クリックは offscreen だけで送る。
  - 汎用の QToolButton で、MenuButtonPopup・InstantPopup・DelayedPopup の各位置をクリックした。
    - 結果: 開くか・クリックになるかが、apply_look の場合と素の Fusion で全く同じ (`$V/probe_off.txt` と `$V/probe_off_fusion.txt`)。
    - MenuButtonPopup の場合、中央はクリック 1・ポップアップ 0。右端の PM_MenuButtonIndicator 幅 (12px) の中は、クリック 0・ポップアップ 1。
  - 実際のボタン (Auto Profile・Play All・Undo/Redo) の `SC_ToolButtonMenu` と `SC_ToolButton` の矩形も、look と Fusion で一致した。
    - 例: Auto Profile は 121×32 で、メニュー部は x109 から幅 12。Undo は 45×28 で、メニュー部は x29 から幅 16。
  - 推測: cocoa で以前使われていた `macos` スタイルとは、メニュー部の幅が違い得る (未測定)。ただし offscreen の自己試験は以前から Fusion なので、試験の前提は変わらない。
- **メニュー部を押したときの描画 (観測)**。押すとボタン全体が沈む (画素値 103。Fusion ではメニュー部だけが沈む)。ベベルを 1 つにした設計どおり。
- **`setNativeMenuBar(false)` (観測と推測)**。cocoa で、show せずに 82 項目を列挙した。
  - 観測: 全項目の menuRole は TextHeuristicRole (1) で、ショートカットは設定どおりだった (F10・Ctrl+O・Ctrl+Shift+C など)。
  - 推測: ネイティブのメニューバーのとき、`Preferences…`・`About…`・`About Qt` は Qt の文字列判定で macOS のアプリメニューへ移され、Tools・Help からは消えていたはず。今回の変更後は、本家や仕様どおり Tools・Help に残る。その代わり、アプリメニューの「環境設定…」「About」と ⌘, は無くなる (I-2)。
  - 推測: ⌘Q (Quit) は、Qt の cocoa プラグインが作るアプリメニューに残る。show しないと確かめられないので、未確認。
- **プロキシスタイル**
  - CC_ToolButton・横向きの CC_Slider・編集不可の CC_ComboBox だけを置き換え、ほかは Fusion に任せる。
  - 観測: 編集可能なコンボ・スピンボックス・チェックボックス・QLineEdit・QPushButton・QMessageBox・メニューが Fusion のまま正しく描かれる (`$V/probe_cocoa/widgets.png`・`msgbox.png`・`menu_*.png`)。
  - QSS 付きのボタン (padding 指定) とコンボ (`color:#ff6060`) も崩れない。

## 3. 見た目 (cocoa で show せずに grab: `$V/grab_native/`。offscreen で show: `$V/grab_off/`)

本家 (`ref/genuine.png`) と比べた (`$V/c_*.png`)。

| # | 箇所 | 判定 (観測) |
|---|---|---|
| 1 | ページのタブ | 解消。全幅の暗い行 (72) の中央にあり、選択中のタブはページ色 (89)。行・ページの画素値は本家と同じ 58/72/89。本家にあるページ外周の 1px の枠線は無い (I-5) |
| 2 | メニュー付きボタンの ▽ | 解消。Build Profile・Undo/Redo・Filter Settings Check・Play All の ▾ が、ボタンの内側の右下にある |
| 3 | 選択欄の 2 重の ▽ | 解消。49.4% ▾・YCrCb + ▾・Normal ▾ の印は 1 つずつ |
| 4 | プロファイル名の欄の矢印 | 解消。平らな欄に白い ▼ が 1 つ (本家と同じ形) |
| 5 | メニューバーと Advanced Mode | ウィンドウの中に入った (`menu bar native 0`)。Advanced Mode は、cocoa で grab したときには写らない (既知: show 前は corner widget が配置されないため)。offscreen で show すると 1664,0・136×24 に置かれ、印なしの太字で写る。統括の実画面の撮影 `livecap2/cpp_after.png` でも右上に写っている |
| 6 | 全体の見た目 | Fusion と灰色のパレットになった。ボタンの勾配・溝・つまみ・欄は本家に近い |

- **文字の読みやすさ (観測)**。背景 89 に対する WCAG のコントラスト比で比べた。
  - 変更前: "Frame n/N" 1.11・"(?)" 1.45・"profile not ready" 1.26 で、ほとんど読めなかった。
  - 変更後: 3.38・2.90・2.37。下層の "(n/a)" は 2.95、橙は 3.26 (本文 232 は 5.72)。
  - 画像で読めることを目で確かめた。"profile not ready" の確認には、プローブで同じ QSS を付けたコンボを使った。
- **新しく崩れた箇所**
  - 確かめた範囲: Adjust ページ (Filter Settings のタブ・見出し・無効の Enable・スライダー・Quick Start のリンク)、Preferences、Profile Viewer、メニュー 7 本 (無効の項目・チェック・節の見出し)、QMessageBox、無効のボタン (P/γ/x1.0・Variants・Push disabled)、無効のコンボ・チェックボックス・スライダー。
  - どれも崩れは無かった。下の L・I は細部の差。

## 4. 32-bit float (観測)

- **単体のアプリ**: 自己試験 `bitDepthMemoryCapAndFloat` で確かめた。深度 32 (gbrpf32le) で "Frame: 640x360, 32-bit float"、10 ビットで "Frame: 640x360, 10-bit" (完全一致)。
- **OFX のホストモードを端から端まで**: `qtui_ofx_drv --ofx 160 96 30 3 --depth 8|16|32` に、test.mp4 の 3 フレームを標準入力から渡した。8 は uint8、16 は ×256 の uint16 LE、32 は /255 の float32 LE。設定は `$V/ofx/*.ini`。
  - `dump` の state_ofx.json は、それぞれ "Frame: 160x96, 8-bit"・"16-bit"・"**32-bit float**"。
- **試験の検出力**: `$V/mut/` の写しで 4 種の変異を入れて試した。4 種とも自己試験が FAIL になり、元に戻すと 5/5 PASS。
  - M1: 単体のアプリで false を渡す → `bitDepthMemoryCapAndFloat` が FAIL ("32-bit")。
  - M2: OFX で false を渡す → `frameInfoFloat32` が FAIL。
  - M3: OFX で常に true を渡す → `frameInfoFloat32` が FAIL ("8-bit float")。
  - M4: 単体のアプリで常に true を渡す → `bitDepth10StatusBar` が FAIL ("10-bit float")。
  - " float" を付けない変異は、作業者が確認済み (PROGRESS.md)。

## 5. 指摘

### M (必須)
なし。

### L (直すことを勧める)

- **L-1 メニューバーの各項目の下に薄い線が出る (本家には無い)**
  - 再現: `qtui/build_main/qtui_style_grab $V/grab_native` を実行し、prepare.png を見る。2 倍の画像の y=39〜40 では、各メニュー名の下が 51 で、項目の間は 58 になっている。さらに y=47〜48 に、全幅の 51 の線がある。
  - 強調した画像: `$V/c_menubar_zoom.png`。
  - 原因の推測: Fusion の CE_MenuBarItem / CE_MenuBarEmptyArea が、下端に window.darker の線を描くため。
  - 本家の同じ帯は全部 58。差は 7 階調で目立たない。
- **L-2 "profile not ready" の赤 #ff6060 は、背景 89 に対するコントラスト比が 2.37 で、直した色の中で最も低い**
  - 再現: プロファイルのない状態で右パネルを見る。プローブでは `$V/probe_cocoa/widgets.png` の 4 段目。
  - 読めるが、#ffa0a0 にすれば 3.6 になる。"(?)" の #6ea8ff (2.90) も、#9cc4ff にすれば 3.92 になる。
  - 本家の色の実測が無いので、任意。
- **L-3 look.h の注釈と実装が合っていない**
  - look.h の注釈は「the menu part is only a hit area, subControlRect below」と書いているが、look.cpp に subControlRect の上書きは無い。
  - 実際の当たり判定は、上書きしていない Fusion の subControlRect で決まる。当たり判定が不変であることは §2 で確かめた。
  - 動作は正しく、文書だけの誤り。

### I (情報)

- **I-1** cocoa で show せずに grab すると、Advanced Mode は写らない (作業者の記録どおり)。実画面での見え方は、統括の撮影でしか確かめられていない。
- **I-2** ネイティブのメニューバーをやめた (推測。show が要るので未確認)。
  - macOS のアプリメニューから「Preferences…」「About」が消え、⌘, が効かなくなる。代わりに Tools の F10 と Help に残る。
  - 本家に合わせた結果として妥当だが、Mac の慣習とは違う。
  - ⌘Q は残るとみられる。
- **I-3** 縦向きのスライダーは、Fusion の青い塗りのまま (縦のときは置き換えていない)。qtui に縦のスライダーは無いので、今は影響しない。
- **I-4** 編集不可のコンボの置き換えには、ホバー・押下・開いている状態の表示が無い。フォーカスの枠だけがある。
- **I-5** 本家との残りの差 (作業者の「残り」以外)。
  - 本家には、ページ外周の 1px の明るい枠線がある。
  - ビューアの画像の外は 55 で、本家は 58 (viewer.cpp:613 の既存の色)。
  - 無効のボタンの文字 (140 / 113、比 1.45) は暗い。ただし無効の表示としては意図どおり。
- **I-6** 範囲外・既存: OFX モードの深度のラベルは "processing 32-bit" で、単体のアプリは "processing 32-bit float RGB"。今回の差分の前からあり、文言を変えない約束なので触れていない。

## 6. 証拠
- ログ: `$V/selftest.log`・`$V/g1.log`・`$V/exp.log`・`$V/build.log` (警告 0)・`$V/probe_off.txt`・`$V/probe_off_fusion.txt`・`$V/probe_cocoa.txt`・`$V/ofx/err*.txt`・`$V/ofx/d*/state_ofx.json`
- 画像: `$V/grab_native/{prepare,adjust}.png`・`$V/grab_off/{prepare,adjust,preferences,profile_viewer}.png`・`$V/c_*.png`・`$V/probe_cocoa/{states,widgets,msgbox,menu_*}.png`
