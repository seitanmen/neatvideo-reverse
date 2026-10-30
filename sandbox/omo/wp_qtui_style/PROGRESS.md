# wp_qtui_style — qtui の macOS 実画面の見た目を本家に合わせる (2026-10-31)

worktree `.claude/worktrees/n3fix-style` (branch n3fix-style、基点 53a52e1)。コミット・ブランチ操作はしていない。

## 原因 (観測)
- qtui はスタイルを指定しておらず、cocoa では `macos` スタイル (暗い背景・青いスライダー・⌄/⇕ の矢印・ネイティブメニューバー)、
  offscreen では Fusion になっていた。`qtui_style_grab` の出力 `platform cocoa, style macos` / `platform offscreen, style fusion` で確認。
- 文字の「▾」付きのボタン (倍率・表示モード・P/γ/x1.0・Advanced Mode) に、macOS スタイルがメニューの印 ⌄ を別に描くので 2 重になっていた。

## 対応 (見た目だけ。機能・処理・保存形式・文言は不変)
- `qtui/src/app/look.{h,cpp}` (新規) `nvqt::apply_look(app)`: Fusion を土台にした QProxyStyle と本家の灰色のパレット
  (本家の画面の画素値: メニューバー 58、タブの行 72、ページ 89、ボタンは上 119 → 下 100 の勾配)。
  - ツールボタンのメニューの印は、ボタンの内側の右下に小さな ▾ を描く。InstantPopup は Fusion のまま。MenuButtonPopup (Build Profile、Undo/Redo、Play All) は、
    ベベルをボタン全体に 1 つだけ描く。メニューの当たり判定は従来どおり右端の PM_MenuButtonIndicator 幅で、クリックの振る舞いは不変。
  - 文字が「▾」で終わるボタンは、▾ を文字として描かずに右下の印にする。ボタンの text() は不変。
  - `nv_arrow = false` のボタンは印を描かない (Advanced Mode。本家に印なし)。
  - 編集不可のコンボは、平らなくぼんだ欄に白い ▼ を 1 つ描く。横スライダーは、明るい平らな溝と灰色のつまみにする (本家の画素値。青い塗りなし)。
  - ニーモニックの下線は描かない。macOS テーマが一部のクラスに小さいフォント (ツールボタン 11pt) を割り当てるので、全クラスを同じフォントにそろえる。
- `main.cpp`: `apply_look` を呼ぶ (OFX モードも同じ)。
- `mainwindow.cpp`:
  - ページのタブを、全幅の暗い行 (72) の中央に置く。選択中のタブはページと同じ色 (89) で、一段高く見える。非選択は暗い色で、文字は灰色。
    タブの QSS を本家の形にした。
  - 中央のウィジェットは外側の余白を 0 にし、その中のページに従来の余白を付けた。
  - `menuBar()->setNativeMenuBar(false)` でメニューバーをウィンドウの中に置き、本家の色 58 にした。
  - Advanced Mode のボタンに `nv_arrow=false` を付けた。
- `rows.cpp` IconBar: アイコンボタンの autoRaise をやめて、常にベベルを描く (本家)。
- `viewer.cpp`: ビューアの背景を 40 から 58 (本家) にした。
- `filter_panel.cpp`: Performance の白い枠に `color:black` を付けた。暗いパレットで白地に白文字になるのを防ぐため。
- `tests/selftest.cpp`: `main` で `apply_look` を呼び、製品と同じ見た目で自己試験する。
- `tests/style_grab.cpp` (新規) と CMake ターゲット `qtui_style_grab`: show せずに `QWidget::grab()` で Prepare と Adjust を撮る。
  - 設定は出力先の `settings.ini` (mem_gb 0.05) を使う。
  - `NV_GRAB_SHOW=1` は offscreen のときだけ使え、show して撮る (ダイアログも撮る)。
  - `NV_GRAB_NO_LOOK=1` は apply_look なしで撮る。

## 撮影 (`shots/`、作り方は `shots/mkcmp.py`)
- before = 基点 53a52e1 を `git archive` で scratchpad に展開し、grab ツールだけ足した版。after = この worktree。どちらも 1800×1010 の 2 倍で、本家と同じ尺度。
- `cmp_tabs_toolbar.png`・`cmp_profile_header.png`・`cmp_viewer_controls.png`・`cmp_menubar_corner.png`・`cmp_panel_lower.png`: 上から本家 / before / after。
- `before_native_*.png`・`after_native_*.png`: 全体 (cocoa、show せずに grab)。`*_offscreen_*.png`: offscreen (show)。`preferences.png`・`profile_viewer.png`: offscreen のダイアログ。

## 確認 (自分で実行した結果)
- 自己試験 `QT_QPA_PLATFORM=offscreen NV_TEST_VIDEO=…/test.mp4 qtui/build_main/qtui_selftest`: 115 passed, 0 failed。最後の変更の後にもう一度実行した。
- `g1_compare.py … --cases default`: RESULT: ALL SAME (test・test_v02 とも、frame・curve/sigma・quality・export・params が SAME)。最終ビルドで実行した。
- `compare_export.py --exe qtui/build_main/qtui_export` (NV_NATIVE=1): RESULT: ALL SAME。最終ビルドで実行した。
- 新しいビルド先での clean ビルド (`cmake --build`): 警告 0 (grep "warning" の件数 0)。
  その後の差分 (filter_panel の 1 行・style_grab) を足したインクリメンタルビルドでも、警告とエラーは 0。

## 残り・制約
- cocoa で show せずに grab すると、メニューバーの corner widget (Advanced Mode) が配置されず、写らない。
  - 観測: grab の時点で位置は 0,0、大きさは 640×480 のまま。offscreen で show すると 1664,0・136×24 に置かれて写る (`cmp_menubar_corner.png` の最下段)。
  - 推測: Qt の QMenuBar は show のときに corner の位置を決めるため。
  - 実際のウィンドウでの確認は、統括に頼む必要がある。
- 本家との差 (今回は扱っていない):
  - タブ・ツールバー・パネルのアイコン (本家は独自の画像。qtui は Fusion の標準アイコン)
  - 右パネルの「Device Noise Profile」の枠 (本家は題字が枠線に入った角丸の枠)
  - Quality 欄の棒の表示
  - P/γ/x1.0 が無効表示であること (機能が無いため)
  - ステータスバー (本家には無い)
  - フレームのスライダー (以前から残すと決めた差)

## 追加作業: フレーム情報の表記 (統括の依頼。文言の変更はここだけ例外)
- `ViewerBar::set_frame_info(W, H, bits, is_float = false)` (`viewer.h`・`viewer.cpp`): is_float のとき "Frame: WxH, 32-bit float"
  (本家の書き方。`coord/genuine_ui/OBSERVED.md` の 7・21 行目)。8・10・12・16 は従来どおり "N-bit"。
- 呼び出し元 (`mainwindow.cpp`):
  - 単体のアプリ: 処理の形式 `pf.is_float` を渡す (深度 32 = gbrpf32le のときだけ真)。
  - OFX のホストモード `set_ofx_frames`: `depth == 32` を渡す (--depth 32 はホストから float32 を受け取る。ofx.h)。
- 試験 (`tests/selftest.cpp`):
  - 新規の項目 `frameInfoFloat32`:
    - ViewerBar 単体で、32-bit float の表記と、8/10/12/16 の "N-bit" を確かめる。
    - OFX のホストモードのウィンドウを --depth 8/16/32 で作って確かめる (32 は float32 のフレームを渡す)。
  - 既存の 10 ビットの項目: 部分一致から完全一致 "Frame: 640x360, 10-bit" に変えた。
  - 既存の 32 ビットの項目: "Frame: 640x360, 32-bit float" の確認を足した。
  - 比較ツールの py_dump.py も frame_info を出すが、8 ビットの場合は表記が変わらない (Python 版は参照用で、更新しない)。
- 結果 (観測):
  - 自己試験 116 passed, 0 failed (115 + 新しい項目 1)。単体実行 `qtui_selftest frameInfoFloat32` も合格。
  - 感度: " float" を付けないように変えると FAIL (Actual "Frame: 3840x2160, 32-bit")。確認後に元へ戻した。
  - `g1_compare --cases default`: RESULT: ALL SAME。
  - 新しいビルド先での clean ビルド: 警告 0。

## 追加作業 2: 暗い背景に暗い文字の修正 (統括の実画面の確認を受けて)
- 新しい背景 (89 = #595959) とほぼ同じ色で、読みにくかった色指定を明るくした (QSS とリッチテキスト。どれも見た目だけ)。
  - `timeline.cpp:84`: フレームの "Frame n / N" の文字を #606060 から #b4b4b4 にした。
  - `rows.cpp:430・474`: 説明の "(?)" を #2a6fd6 から #6ea8ff (パレットのリンク色) にした。
  - `rows.cpp:475`: 下の階層の "(n/a)" を #808080 から #a8a8a8 にした。
  - `profile_panel.cpp:360`: "profile not ready" の赤を #d01010 から #ff6060 にした。
  - `mainwindow.cpp:919`: メモリ上限で切った時のステータス表示の橙を #d08000 から #f0a030 にした。
- 一通り見て、変えなかったもの:
  - `DARK_BOX` (#3a3a3a に #d5d5d5)、見出しの帯 (#6e6e6e に #e9e9e9)、ACTIVE_TITLE の黄、Profile Check がオンのときの青地に白、タブ (#484848・#595959 に #a8a8a8・#f0f0f0)、ACTIVE_TITLE 以外の暗い見出し #9a9a9a: どれも読める (#9a9a9a は意図した控えめの表示)。
  - 赤い ✕ (#d02020・#e02020・#e0409a) は記号で、本家も灰色の地に赤い ✕。
  - 自前で塗る部品 (ビューア・曲線・グラフ・帯のバー): 自分で背景を塗るので、パレットの影響を受けない。
- 撮影: `shots/cmp_text_contrast.png` (cocoa で show せずに grab、前後)。`shots/after2_native_{prepare,adjust}.png`。
  "Frame 5 / 17" と "(?)" が読めることを目で確かめた。"profile not ready" (プロファイルのない状態) は撮っていない。
- 自己試験 116 passed, 0 failed。変更したファイルをビルドし直して、警告 0。

## 追加作業 3: "n/a" の表示 (ユーザーの依頼、PR #6)

### 3.1 イコライザの行の欄 (`complete_lbl`)
- `ProfilePanel::set_completeness` が、パネルの Completeness 欄と同じ文字列 (g×100 の "%"、プロファイルがなければ "n/a") をイコライザの行の欄にも出す。
  本家の Advanced パネルがこの位置に g×100 を出していることに合わせた (QUALITY.md §8.4、genuine.png の "85%")。
  ツールチップの「(n/a: not computed in the reproduction)」は「同じ値。プロファイルがなければ n/a」に変えた。
- 試験:
  - 新規の項目 `equalizerCompleteness`: 窓の値と一致すること、85 → "85%"、なし → "n/a"、ツールチップに "not computed" がないことを確かめる。
  - 既存の項目にも確認を足した: `qualityPanel` (AFT 後)、`resetFineTuneCompleteness` (Reset → 0%、Undo)、Reset Profile → "n/a"。
  - 感度: 1 行を消すと `equalizerCompleteness` と `qualityPanel` が FAIL (Actual "n/a")。確認後に元へ戻した。
- 撮影 `shots/cmp_eq_completeness.png`: 本家 85% / 前 n/a / 後 22% (test_v02、cocoa で grab)。
  本家の欄は 85% の割合だけ明るく塗った棒の形で、そこは未対応 (見た目の差として残す)。

### 3.2 Completeness の値の差の調査 (観測)
- 同じ入力・同じ流れ (フレーム F で Auto Profile → Auto Fine-Tune) で、次の 3 つを比べた。
  - C++ の今のビルド: `qtui_g1eq … default` の quality.json
  - G1 より前の C++ ビルド: `scratchpad/g1cpp_before/qtui/build/qtui_g1eq`
  - PySide6 版の backend/quality: `tools/py_quality.py`。NV_NATIVE=1 と、純 Python の NV_NATIVE=0 の両方
- 結果 (Quality, Completeness) と、領域、AFT が未測定とした (マーカーの付いた) ビンの数:

  | 入力 | C++ 今 | C++ G1 前 | Python (native) | Python (NV_NATIVE=0) | 領域 (y, x, size) | 未測定ビン |
  |---|---|---|---|---|---|---|
  | test_v02 100 | 50, 26 | 50, 26 | 50, 26 | — | 0, 800, 128 | 7 |
  | test_v02 240 | 47, 19 | 47, 19 | 47, 19 | 47, 19 | 0, 856, 128 | 5 |
  | test_v02 312 | 47, 19 | 47, 19 | 47, 19 | — | 0, 832, 128 | 5 |
  | test_v02 576 | 36, 19 | 36, 19 | 36, 19 | — | 864, 520, 64 | 5 |
  | test.mp4 300 | 43, 70 | 43, 70 | 43, 70 | 43, 70 | 288, 416, 48 | 19 |
  | test.mp4 30 | 40, 59 | 40, 59 | 40, 59 | — | 288, 392, 48 | 16 |

- G1 による変化はない。C++ は G1 の前後どちらでも Python と全件一致する。統括の実画面の 47%/19% は test_v02 240 の行と同じ値。
- 本家の 65%/85% と違うのは、プロファイルの作り方が違うため。
  - 本家: Resolve で 3840×2160 に拡大した test_v02 の全面 ("Full frame") を見本にした。AFT が測ったのは 4 ビンで、残る 23 ビンが未測定 (23/27 = 85%)。
  - repro の Auto Profile: FindFlatArea の移植が選んだ空の 128² を見本にする。AFT の移植は 22 ビンを測るので、未測定は 5 (5/27 = 19%)。
  - g は「未測定ビン 1 個につき +1/27」なので、AFT が多くのビンを測れるほど Completeness が下がる。
  - repro の UI には全面のプロファイルを作る経路がない (選択は 1 辺 512 まで)。AFT の移植の係数と測定ビンは、本家とは照合していない (§6.5)。
- 気づいた点 (UI の表示。直していない):
  - パネルの情報欄の "Profiled with: Full frame (WxH)" は、本家の文字列を固定で出している (`profile_panel.cpp:394`)。実際の見本は 128² の領域でも "Full frame" と出る。統括が「Full frame、AFT 済み」と読んだのはこのためとみられる。
  - 実際の領域を出すかどうかは文言の変更になるので、統括の判断を待つ。

### 3.3 qtui の "n/a" の一覧と分類
(a) 値がすでにあり、表示するだけで済む → この PR で表示した
- `profile_panel.cpp:274` イコライザの行の欄 → Completeness (3.1)

(b) 本家の機能そのものが未実装 → この PR では実装しない
- 下の各機能は、実装すると処理・測定・ホストとの連携を変えることになる。
- Filter Settings の (n/a) の行 (`state.cpp` の implemented=false。ラベル "(n/a)"、値 "n/a"、無効表示):
  - Quality Mode (時間・空間): v5 に品質モードがない
  - Sharpening 一式 (Enable、Prevent Over-Sharpening、Range ×3、チャンネル ×3): 鮮鋭化の処理がない
  - Jitter of Details (Threshold、Extended Filtration): 処理がない
  - Dust and Scratches (Mode、Threshold、Amount): 処理がない
  - Flicker (Local、Full Frame): 本家に合わせて全体補正は既定でオフ。局所的なフリッカーの処理もない
  - Edge Smoothing (Low・Mid・High): 処理がない
  - Repeated Frames の Repeat Rate: 処理がない
  - Adapt to Changing Noise: 処理がない
  - Artifact Removal (Dots・Lines・High・Mid・Low): 処理がない
  - Mix の成分別 (mix_Y/Cr/Cb): backend の mix は 1 つだけで、成分別にするには処理を変える必要がある
- 確認ツール:
  - "Begin (n/a)" の Local Flicker Check と Repeated Frames Check (Filter Settings Check のメニューの 2・3 番も同じ)
- Device Noise Profile パネル:
  - Manual Fine-Tune ボタン: 選んだ領域でビンを測り直す機能が未実装 (グラフの点をドラッグする手動の調整はある)
  - Noise Level の Frequencies (High..Ultra Low): repro のプロファイルの曲線には帯域がない
- 最下段の P (走査方式)・γ (入力のガンマ)・x1.0 (入力のゲイン): repro はプログレッシブとガンマ補正済みだけを扱い、入力のゲインはない
- ツールバーの Variants ×2 と Variants メニューの項目: 別の設定の組が未実装
- Profile メニュー:
  - Auto Match: 未実装
  - Profile Converter…: 本家の .dnp 形式の読み書きが未実装 (ツールチップは "Not available: …")
- Effect メニューの Skip Neat Video Processing: ホストとの連携 (処理の省略) が必要

(c) その他
- 表示する値がないときの代わりの表示 (仕様どおり、変えない):
  - パネルの Quality と Completeness の "n/a" (プロファイルがないとき)
  - Profile Viewer の "Overall Profile Quality: n/a" と "Completeness: n/a"
  - 本家も、片方の表示は条件付きで "n/a" を出す (REPORT の発見の節)
- 本家での中身が未観測のもの:
  - Device Name and Mode の ▸≡ メニュー
  - Profile メニューの Save As Default Profile… (repro には Assign As Default Profile がある)
- 外部の文書・サイトへのリンク (repro の対象外):
  - Help メニューの Read Me、What's New、How to Use Neat Video、Web Page、Community Forum、Report a Bug Online、Support via E-mail
- コメントだけで、表示はない: `mainwindow.h:3`、`rows.h:120`、`filter_panel.cpp:455`

### 3.4 確認 (観測)
- 自己試験: 117 passed, 0 failed (116 + equalizerCompleteness)。
- `g1_compare --cases default`: RESULT: ALL SAME。
- `compare_export` (NV_NATIVE=1): RESULT: ALL SAME。
- 新しいビルド先での clean ビルド: 警告 0。
- cocoa の grab: `shots/after3_native_prepare.png` と `cmp_eq_completeness.png` で、欄が 22% と出ることを目で確かめた。
