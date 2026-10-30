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
