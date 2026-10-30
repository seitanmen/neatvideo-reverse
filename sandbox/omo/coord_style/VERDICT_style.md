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

---

# Round 2 (2026-10-31): イコライザの行の Completeness と "n/a" の棚卸し

対象: a7d6f3f の上の未コミットの差分 (`qtui/src/widgets/profile_panel.cpp`・`qtui/tests/selftest.cpp`・`PROGRESS.md`)。

判定: **APPROVE** (M なし。L 1 件・I 3 件)

## R2-1 再実行 (観測)

| 項目 | 結果 |
|---|---|
| qtui の新しいビルド先 (`$V/build2`) でクリーンビルド | rc 0、警告 0 |
| `build_main` のインクリメンタルビルド | 警告 0 |
| 自己試験 | **117 passed, 0 failed** (`$V/selftest2.log`)。`equalizerCompleteness`・`qualityPanel`・`resetFineTuneCompleteness` は PASS |
| `g1_compare --cases default` | **RESULT: ALL SAME** (test 43/70、test_v02 50/26) |
| 変異 (`$V/mut` の写しで `complete_lbl->setText(...)` の 1 行を削除) | `qualityPanel`・`equalizerCompleteness`・`resetFineTuneCompleteness` が FAIL。試験は差を検出する |

- 変異のときは、ほかに `ofx*` の 3 件も FAIL した。これは写しの環境によるもので、変異とは関係がない。
  - 写しでは `qtui_selftest` だけをビルドし、MeatVideoQt を起動できなかった (r.code −1)。
  - 変異なしの写しでも同じ 3 件が FAIL するので、変異とは関係がない。worktree の build_main では 117 件すべて合格している。

## R2-2 差分の確認 (観測)

- **表示だけの変更か**: そのとおり。
  - `ProfilePanel::set_completeness` が、Completeness 欄の文字列を `complete_lbl` にも写すだけ。
  - ほかに変わったのは、ツールチップの文言とコメント。
- **常に同じ値になるか**: 構造上そうなる。
  - Completeness を書くのは `set_completeness` だけ (grep。呼ぶのは `MainWindow::update_quality` の 2 か所)。`completeness->setText` を直接呼ぶ箇所は無い。
- **場面ごとの実測**: 自作のプローブ `$V/probe/probe2.cpp` で確かめた。製品には入れていない。offscreen で、設定は一時のファイルを使った。
  - 結果: 12 場面すべてで、パネルとイコライザの行の値が一致した (DIFF 0)。

| 場面 | パネル / 行 | 場面 | パネル / 行 |
|---|---|---|---|
| 動画なし (プロファイルなし) | n/a / n/a | Reset Profile | n/a / n/a |
| test.mp4 を開いた直後 (Auto Profile と AFT の後) | 70% / 70% | Reset Profile を Undo | 70% / 70% |
| Reset Fine-Tuning | 0% / 0% | 旧形式 `g1_legacy_r4` を読み込む | 72% / 72% |
| Undo | 70% / 70% | 旧形式 `g1_legacy_b6tune` を読み込む | 80% / 80% |
| Redo | 0% / 0% | 読み込みを Undo | 72% / 72% |
| もう一度 Undo | 70% / 70% | 動画を開き直す | 70% / 70% |

## R2-3 作業者の調査結論の確かめ (観測と推測を分ける)

- **3 つの経路の一致 (観測)**: test_v02 で、作業者の表に無いフレーム 400 と、表にある 240 を比べた。
  - 流れはどちらも同じで、Auto Profile (F−2〜F+2) の後に AFT。
  - Python 版は、作業者のスクリプトを使わず、自分で書き直したもの (`$V/q2py/vq.py`) で実行した。

  | フレーム | 今の C++ | G1 前の C++ | Python (native) | Python (NV_NATIVE=0) | 領域 | AFT のマーカー |
  |---|---|---|---|---|---|---|
  | 400 | 49, 22 | 49, 22 | 49, 22 | 49, 22 | 0, 840, 128 | 6 |
  | 240 | 47, 19 | 47, 19 | 47, 19 | — | 0, 856, 128 | 5 |

  - 「G1 の前後でも、Python 版でも同じ値」は確かめられた。
  - g の項を分けると、AFT の後は「マーカー +1/27」の項だけが残る (400: 6 項、240: 5 項。ほかの 21〜22 ビンは 0)。Completeness はマーカーの数 ÷ 27 になる (6/27 = 22%、5/27 = 19%)。本家の 85% は 23/27 に当たる。
- **「見本の領域の違い (空の 128² と、2 倍に拡大した全面) が原因」は、repro の中では確かめられない (推測のまま)**。
  - 確かめようとした結果 (観測):
    - 見本を 512² (中央・左上・左下。景色の構造を含む) にしても、test_v02 240 の Completeness は 19% のままで、マーカーは 5。Quality は 15% / 12% に下がる。
    - 2 倍 (bicubic) に拡大した 3840×2160 の test_v02 (236〜244) では、Auto Profile の領域は (1808, 2016, 128) になり、AFT の後の Completeness は 15% (マーカー 4) だった。
    - 全面 (1920×1080) の見本は、repro では作れない。`nv_model` が「1 辺 512px まで」の ValueError を出す。
  - したがって、確かめられる範囲では、領域の大きさを変えても、入力を 2 倍に拡大しても、repro の AFT は 22〜23 ビンを測り、85% には近づかない。
  - 本家との差の原因として残る候補 (推測):
    - (i) 全面の見本そのもの (repro では作れない)
    - (ii) AFT の移植 (類似度の判定、ビンの集計) と本家の違い。AFT の移植は、本家と照合したのがタイル順だけ (§6.5)。
  - 作業者の文書のうち、確定してよいのは「G1 前後と Python で同じ」まで。原因の部分は「推測 (repro の中では確かめられない)」と書くのがよい (L-R2-1)。

## R2-4 "n/a" の一覧と分類 (観測)

- qtui/src の "n/a" 24 か所と、`state.cpp` の implemented = false の項目をすべて見た。PROGRESS.md §3.3 の分類と照らし合わせた。
- **(a)「値があって表示するだけで済む」の取りこぼしは無い**。
  - Quality・Completeness・Profile Viewer の "n/a" は、プロファイルが無いときだけ出る (上の表で確かめた)。
  - (n/a) の行は、どれも backend に対応する処理か値が無い。
- 補足 (I-R2-1): Flicker は、CLI の `denoise_video_v5.py --flicker R` に全画面のゲイン補正 (移動平均) がある。
  - ただし本家の「Full Frame Flicker Threshold」とは意味が違う。UI につなぐには処理の経路を変える必要があるので、(b) の分類は妥当。
  - PROGRESS.md の理由「局所的なフリッカーの処理もない」は正しいが、全画面の補正が CLI にはあることを書き添えるとよい。
- 補足 (I-R2-2): 作業者が見つけた "Profiled with: Full frame (WxH)" は、文言の変更になるので棚卸しの外にある。
  - これは固定の文字列で、実際の見本が 128² でも "Full frame" と出る (`profile_panel.cpp:394`)。Python 版 (`profile_panel.py:362`) も同じ。
  - 実際の領域の値 (`prof["area"]`) はあるので、表示するだけで済む種類に近い。統括が Completeness を「全面・AFT 済み」と読み違えたのは、この表示のためとみられる。
  - 文言を変えるかどうかは統括が決めること。変えるなら、例えば「Area 128x128 at (x, y)」のような repro の書き方が考えられる。

## R2-5 指摘

- **L-R2-1** PROGRESS.md §3.2 の「本家の 65%/85% と違うのは、プロファイルの作り方が違うため」は、repro の中では確かめられていない。
  - 512² の見本でも、2 倍に拡大した入力でも、repro の Completeness は 19% / 15% のままだった (R2-3)。
  - 確定した事実 (G1 前後と Python で同じ、Completeness = マーカーの数 ÷ 27) と、推測 (見本の違い、または AFT の移植の違い) を分けて書くこと。コードの変更は不要。
- **I-R2-1** Flicker の分類理由への補足 (R2-4)。
- **I-R2-2** "Profiled with: Full frame" が実際の見本と違う (R2-4。統括の判断待ち)。
- **I-R2-3** ツールチップの文言を変えた。誤った「n/a: not computed」を消すためで、妥当。

## R2-6 証拠
- `$V/selftest2.log`
- `$V/build2.log` (警告 0)
- `$V/g1r2/`
- `$V/q2/{now,before}{400,240}/default.quality.json`
- `$V/q2py/vq.py`・`vq_area.py`・`up2.mkv` (2 倍に拡大した 9 フレーム)
- `$V/probe/probe2.cpp` と実行結果 (R2-2 の表)

## Round 2 追記: 統括の観測 ("Profiled with:" の選択肢) を受けて

- **文字列の確認 (自分で実行した結果)**
  - `strings -a neatvideo_ofx_614/NeatVideo6.arm64.ofx` の 121427〜121437 行目に、次の順で並んでいる。
    `Frame size:` → `Scan type:` → `Progressive` / `Interlaced` → `Gamma:` → `Gamma corrected` / `Linear` → `Gain:` → `Profiled with:` → `Full frame (` / `Two fields (` / `One field (`
  - 本家のバイナリは読んだだけで、実行も dlopen もしていない。統括の観測と一致した。
- **解釈 (推測)**
  - "Profiled with:" の 3 つの選択肢は、走査方式 (プログレッシブ / インターレース) の項目のすぐ後に並び、Two fields・One field と対になっている。
  - したがって、"Full frame" は「プログレッシブのフレームとして作った」という意味で、見本の領域の大きさは表していないとみるのが自然。
  - 括弧の中の WxH (本家で観測した "Full frame (3840x2160)") は、フレームの大きさとみられる。
  - ただし、表示を作る関数は読んでいないので、確定ではない。

### 1. 「今の C++・G1 前の C++・PySide6 が同じ値」: **成り立つ (観測)**
- R2-3 のとおり、自分で実行して確かめた。本家に依存しない比較。
  - test_v02 400: 49 / 22 が 4 つの経路で一致した。
  - test_v02 240: 47 / 19 が 3 つの経路で一致した。
- 経路は、qtui_g1eq (今と G1 前) と、自分で書いた Python スクリプト (NV_NATIVE=1 と 0)。
- 作業者の表の 6 入力とも矛盾しない。
- repro の Completeness が「マーカーの数 ÷ 27」になることも、g の項を分けて確かめた。これも本家と関係なく成り立つ。

### 2. 本家の 65% / 85% との差の原因: **「未確定」とするのが妥当**
- 作業者の説明「本家は全面を見本にした」の根拠は、"Profiled with: Full frame" という表示だった。
  - 上の解釈が正しければ、この表示は領域の根拠にならない。
  - 同じ表示を前提にした QUALITY.md の「全面プロファイル」の記述も、同じく根拠が弱くなる。
    - QUALITY.md の S/5 = 1.0 は、全面でなくても成り立つ。SIZE_T は 256 までなので、1 辺 256 以上の領域なら成り立つとみられる (推測)。
    - したがって、23% や 65% と整合することは、全面であることの証明にならない。
- repro の中での反証の試み (R2-3 の観測)
  - 見本を 512² にしても、入力を 2 倍に拡大しても、repro の Completeness は 19% / 15% のままで、85% に近づかない。
  - 全面の見本は repro では作れない。
- 原因の候補として残るもの (どれも推測で、絞り込めていない)
  - (i) 本家の見本の領域。Resolve での実際の領域は記録がなく、不明。
  - (ii) AFT の移植 (類似度の判定、ビンの集計) と本家の違い。本家と照合したのはタイル順だけ。
  - (iii) 入力の違い。Resolve が拡大したフレームと、repro の bicubic による拡大は違う。
- **L-R2-1 を強める**: PROGRESS.md §3.2 の「プロファイルの作り方 (全面 / 128²) が違うため」は、「原因は未確定」と書き直すこと。

### I-R2-2 の撤回と訂正
- R2-4 では「"Profiled with: Full frame (WxH)" が実際の見本と違う。領域を出すことも考えられる」と書いた。これを撤回する。
- 上の解釈が正しければ、repro の固定の表示は、本家と同じ意味 (プログレッシブ = Full frame、括弧はフレームの大きさ) で正しい。領域に書き換えてはいけない。
- PROGRESS.md §3.2 の「統括が Completeness を『全面・AFT 済み』と読み違えたのはこの表示のため」も、同じ理由で根拠が無くなる。
- 判定 (APPROVE) は変わらない。コードの変更は不要で、文書の訂正 (L-R2-1) だけ。
