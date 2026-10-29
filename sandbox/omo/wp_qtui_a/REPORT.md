# N3a REPORT: Qt 6 C++ UI の骨格と Prepare ページ

作業場所は worktree n3a (ブランチ n3a-qtui、fea7964 から)。コミットはしていません。AGENTS.md・MIGRATION.md・スナップショット・repro/・本家バイナリ・repro/out/ には触れていません。

## 前提の不足 (着手時に統括へ連絡済み)
- N2a の C ABI が持っていたのは FindFlatArea だけで、build_profile_v5 の残りは Python のままでした。そこで分担作業者 (fork) が残りを移植し、`nvc_build_profile_v5` として C ABI に足しました。
  - 対象: area_band_sigma・noise_curve_spatial/temporal・_dead・_fill_curve・find_flat_area_gradient など。
  - noisiest_frame・hh1_sigma・effective_profile・_wn_band_std も C ABI にしました。
- 一番の難所は np.polyfit でした。
  - 観測: numpy と同じ呼び方で比べると、anaconda の OpenBLAS 0.3.29 の dgelsd は 9320/9320 一致、Accelerate の dgelsd は 9068/9320 が不一致でした。
  - 対応: libopenblas を逆アセンブルし、gfortran が使う FMA の位置と neoversen1 カーネルの演算順を写して、`native/nv_lstsq` で dgelsd を再実装しました。46,600 系すべてビット一致です。OpenBLAS の同梱は要りません。

## Qt の導入
- aqtinstall を `~/Qt/.aqt-venv` に入れました (36 MB)。
- Qt 6.12.0 clang_64 は qtbase だけを `~/Qt/6.12.0/macos` に入れました (188 MB、所要 17 秒)。brew と anaconda は変更していません。

## 作ったもの
- **qtui/** (CMake、C++20、`-ffp-contract=off`)
  - `src/core`
    - pyobj: 順序付き JSON。Python の dict の意味と json.dumps の文字列を再現します。
    - state: PARAM_SPECS、ProfileState、Undo。
    - nvcore: C ABI の薄い包みです。数値計算はしません。
    - video: ffmpeg の探索、probe、read_frames。
    - engine: PreviewEngine。
  - `src/app`: mainwindow・menus、viewer・ViewerBar、worker。
  - `src/widgets`: rows・curve_editor・timeline・noise_graph・profile_panel。2 人目の分担作業者が書きました。
  - `tests/selftest.cpp` (Qt Test)、`tests/dump.cpp`、`tools/py_dump.py`・`tools/compare_ui.py`・`tools/gen_band_pixmaps.py`。
  - 実行ファイル: `MeatVideoQt.app`・`qtui_selftest`・`qtui_dump`。
- **native/ の新規ファイル**
  - `nv_ui.{h,cpp}` と `nvcore_c_ui.{h,cpp}`: yuv_to_rgb、display.py 一式 (render_simple・freq_images・render_freq・minmax)、noise_level、not_uniform、quality.profile_numbers、mix、reference_floor。
  - `nv_prof5.*`・`nv_lstsq.*`・`nvcore_c_prof5.*`。
  - 照合: `verify_bridge_ui.py`・`verify_prof5.py`・`verify_bridge_prof5.py`。
- **変更したファイル**
  - `native/Makefile`: LIB_SRC と LIB_HDR に追加。
  - `native/nv_curve.{h,cpp}`: up2 と ratio_eps の公開ラッパを追加 (挙動は不変)。
  - `native/verify_bridge.py`: prof5 群と ui 群を足し、既定の `--only` にも入れました。
  - `.gitignore`: qtui/ はソースを許可し、build/ を除外。
  - ABI の版は 1 のままで、既存の関数は変えていません。

## 合格条件ごとの結果

**1. 動画を開く → 選択 → Build Profile → パネル表示**

`compare_ui.py` で、test.mp4 と test_v02 (1080p、メモリ上限で 86 フレーム) を比べました。

| 比べたもの | 結果 |
|---|---|
| 状態 4 つ (自動プロファイル+AFT、選択、Build+AFT、Adjust) の表示文字列 19 項目 (Quality・Completeness・Main/Selection の Noise Level・選択ラベル (+n%)・Not Uniform・バー・ステータスバーなど) | すべて一致 |
| プロファイル JSON | 全葉ビット一致、`json.dumps(indent=1)` の文字列も同一 |
| ツールバー 10 個 | 同一 |
| メニュー 114 項目 | 文言・順序・ショートカットは同一 |

メニューの差は、N3b/N3c の項目 6 つを無効にしていることと、Help に About Qt を足したことだけです。

**2. ビューアの画素**

- 7 モード × 明るさ 3 段階 × Prepare / Profile Check / Adjust の Filtered で、ペイン画像を比べました。
  - test.mp4: 226/226 がバイト一致 (5,207 万画素)。
  - 1080p: 226/226 がバイト一致 (4.69 億画素)。
- Preview Area の Filtered も一致しました。
- ビューアを選択枠と Noise Level ラベル込みで grab した画像は差 0 画素です。
- 窓全体の grab は、Prepare の 3 状態と Profile Check が差 0 です。Adjust ページは 6.7% 違いますが、これは Filter Settings パネルが N3b の仮パネルだからです。

**3. 照合**

| 照合 | 結果 |
|---|---|
| `verify_bridge_ui.py` | 17,775/17,775 ビット一致 |
| prof5 の単体照合 (分担作業者の実行) | 630/630 |
| prof5 の bridge 照合 (分担作業者の実行) | 110/110 |
| `verify_bridge.py --only prof5,ui --quick` | ALL MATCH (統合後に私が実行) |
| `make -C native verify` 11 本 | 回帰なし (クリーンビルド警告 0) |

11 本の内訳: quality 4000/4000、band 5706/5706、curve 1094+4000+20554+25、lzma 667/667、flat 362/362、flat_cancel 676/676、model 1244+28、temporal 474/474、spatial 1287/1287、profile 39/39、aft はすべて OK。

**4. selftest**

- 34 項目すべて PASS (initTestCase / cleanupTestCase を含めると 36 件)。所要約 3 秒です。
- qtui のクリーンビルドも警告 0 でした。

**5. スクリーンショット**

- `sandbox/omo/wp_qtui_a/cmp/sbs_t/` と `cmp/sbs_v02/`: 左が Python、中央が C++、右が差 ×8。
- `cmp/sbs_v02/preview_half_*.png`: 縮小版。
- `widgets/shots/`: 部品単位の比較。7 状態すべて画素一致です。

## 仕様との相違点
- Adjust ページの Filter Settings・書き出し・Noise Level Check は N3b、Preferences・Profile Viewer・Batch Profiler は N3c としました。プレビューは現在のパラメータで動きます。
- C++ 側の AFT は C の経路だけです。C の BuildNoiseModel が受け付けない領域では、Python 版は Python の AFT に切り替えますが、C++ 版はエラーを表示します。
- 設定ファイルは Python 版と共有しています (`NeatRepro/nv_ui_qt`、試験では `NV_UI_QT_SETTINGS` で別ファイル)。

## 質問
1. 設定ファイルを Python 版と共有してよいか、別の名前にすべきか。
2. polyfit は OpenBLAS の演算順に固定しました。numpy や BLAS を更新したときは照合のやり直しが要りますが、それでよいか。

## 未解決・注意
- 照合の前提は numpy 2.1.3 / scipy 1.15.3 / OpenBLAS 0.3.29 (arm64) です。
- 新しい Q_OBJECT ヘッダを足したときは、cmake の再構成が要りました (AUTOMOC の取りこぼしを観測)。
- 実際の画面での確認はまだです。手順:
  1. `cmake -S qtui -B qtui/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build qtui/build -j 2`
  2. `qtui/build/MeatVideoQt.app/Contents/MacOS/MeatVideoQt test_v02.mp4` で起動する。
  3. 自動プロファイルと AFT を待つ → 空をドラッグで選択 → Build Profile → パネルの数値・Profile Check・Adjust ページのプレビューを、Python 版 (`cd repro && python3 -m nv_ui_qt.main ../test_v02.mp4`) と並べて見る。

## 再現手順 (比較)
```
QT_QPA_PLATFORM=offscreen qtui/build/qtui_dump test_v02.mp4 OUT_C 0,832,128,128 0.25
QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/py_dump.py test_v02.mp4 OUT_P 0,832,128,128 0.25
python3 qtui/tools/compare_ui.py OUT_P OUT_C SBS_DIR
QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest
```

---

主なファイルとフォルダ:
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/qtui/`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/nvcore_c_ui.h`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/nvcore_c_prof5.h`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/verify_bridge_ui.py`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/sandbox/omo/wp_qtui_a/` (比較画像は `cmp/`、11 本照合のログは `log_native_verify.txt`)

---

# 追補: 統括の 5 条件への対応 (作業者の最終報告より転記)

## 条件への対応

1. **dgelsd**: 時間内に C++ で再実装し (`native/nv_lstsq.{h,cpp}`)、numpy.polyfit とビット一致しました。照合は OpenBLAS 0.3.29 のオラクルと numpy の両方です。
   - 通る形はすべて照合済みです。2〜8 点の全部分集合 × 次数 1/2 と、両端の外挿用の 2〜3 点の 1 次。rcond は polyfit の既定値です。
   - 値の分布 7 種と合わせて 46,600/46,600 件が一致し、演算順の変異 5 種はすべて検出できました。
   - 退化・同値の入力を明示的に網羅したかは、分担作業者の報告からは確認できていません。必要なら追加します。
2. **OpenBLAS のリンク**: 1 で一致したので使っていません。同梱も不要です。
3. **既存関数**: nvcore_c の既存関数の挙動は変えていません。足したのは `nvcore_c_prof5.h` と `nvcore_c_ui.h` の新関数だけで、ABI の版は 1 のままです。
   - `verify_bridge.py` に prof5 群と ui 群を足し、既定の `--only` にも入れました。
   - prof5 群: build_profile_v5 の dict の全数値を実写 (test_v02・test.mp4) で比較し、110/110 一致。モードは auto/spatial/temporal、area はなし・正方形・長方形です。
   - 合成入力は `verify_prof5.py` 側で照合し、630/630 一致でした。
   - ui 群: 17,775/17,775 一致です。
4. **N3a 本体**: 途中段階にせず完成しました。詳細は下の合格条件の表です。
5. **native/ の影響範囲** (W2a との突き合わせ用):
   - 変更は 4 ファイルです。
     - `Makefile`: LIB_SRC と LIB_HDR への追加
     - `nv_curve.cpp`・`nv_curve.h`: up2 と ratio_eps の公開ラッパを追加。挙動は変えていません
     - `verify_bridge.py`
   - 新規: `nv_ui.*`、`nvcore_c_ui.*`、`nv_prof5.*`、`nv_lstsq.*`、`nvcore_c_prof5.*`、`verify_bridge_ui.py`、`verify_prof5.py`、`verify_bridge_prof5.py`
   - 変更後に `make -C native verify` の 11 本を回し、回帰なし、警告 0 でした。

## 合格条件の結果

| 条件 | 結果 |
|---|---|
| 動画オープン → 選択 → Build Profile → パネル表示 | test.mp4 と 1080p で、4 つの状態それぞれ表示文字列 19 項目がすべて一致。プロファイル JSON は全数値がビット一致し、JSON の文字列も同一 |
| ビューアの画素 | 7 モード × 明るさ 3 段階 × (Prepare / Profile Check / Adjust の Filtered) のペイン画像が 226/226 バイト一致 |
| 窓全体のスクリーンショット | Prepare の 3 状態と Profile Check は差 0 画素。Adjust ページは Filter Settings パネルが N3b の仮パネルなので 6.7% 異なる |
| Qt Test の selftest | 34 項目すべて PASS (offscreen) |
| ビルド | qtui のクリーンビルドで警告 0 |

- **About**: Qt の LGPL v3 の表記を入れ、Help に About Qt も足しました。
- **Qt 6.12.0**: aqtinstall で `~/Qt/6.12.0/macos` に qtbase だけを入れました (188 MB)。aqtinstall 自体は `~/Qt/.aqt-venv` (36 MB) です。brew と anaconda には触れていません。
- **メニュー**: 114 項目の文言・順序・ショートカットは Python 版と同じです。違いは 2 点だけです。N3b/N3c の項目 6 つを無効にしていること、Help に About Qt を足したことです。
- **スクリーンショット**: 左が Python、中央が C++、右が差分です。
  - `sandbox/omo/wp_qtui_a/cmp/sbs_t/`・`sbs_v02/`: 窓全体
  - `sandbox/omo/wp_qtui_a/widgets/shots/`: 部品単位。7 状態とも画素一致

## Python 版との相違
- Filter Settings パネル・書き出し・Noise Level Check は N3b、Preferences・Profile Viewer・Batch Profiler は N3c に回しました。Adjust ページのプレビューは、現在のパラメータでそのまま動きます。
- AFT は C の経路だけです。C 側が受け付けない領域では、Python 版は Python の AFT に切り替えますが、C++ 版はエラーを出します。
- 設定ファイルは Python 版と共有しています (`NeatRepro/nv_ui_qt`)。試験では `NV_UI_QT_SETTINGS` で別のファイルを使います。

## 質問
1. 設定ファイルは Python 版と共有したままでよいですか。別の名前にすべきですか。
2. polyfit を OpenBLAS 0.3.29 の演算順に固定しました。numpy や BLAS を更新したら照合をやり直す前提になりますが、それでよいですか。

## 注意と未確認
- **照合の前提**: numpy 2.1.3、scipy 1.15.3、anaconda の OpenBLAS 0.3.29 (arm64) です。
- **cmake**: Q_OBJECT を含むヘッダを新しく足したときは、cmake を構成し直す必要がありました。AUTOMOC が新しいヘッダを拾わない事象を観測しています。
- **実画面での確認**: まだです。手順は次のとおりです。
  1. `cmake -S qtui -B qtui/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build qtui/build -j 2`
  2. `qtui/build/MeatVideoQt.app/Contents/MacOS/MeatVideoQt test_v02.mp4` で起動する
  3. 自動プロファイルと AFT が終わるのを待ち、空の部分をドラッグで選んで Build Profile を押す
  4. パネルの数値・Profile Check・Adjust ページのプレビューを、Python 版 (`cd repro && python3 -m nv_ui_qt.main ../test_v02.mp4`) と並べて見比べる

## 比較の再現
```
QT_QPA_PLATFORM=offscreen qtui/build/qtui_dump test_v02.mp4 OUT_C 0,832,128,128 0.25
QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/py_dump.py test_v02.mp4 OUT_P 0,832,128,128 0.25
python3 qtui/tools/compare_ui.py OUT_P OUT_C SBS_DIR
QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest
```

## 変更・新規ファイル
- 新規 `qtui/` 一式
  - CMakeLists.txt
  - src/core: pyobj・state・nvcore・video・engine
  - src/app: mainwindow・viewer・worker
  - src/widgets: rows・curve_editor・timeline・noise_graph・profile_panel・band_pixmaps.inc。2 人目の分担作業者が担当
  - tests: selftest.cpp・dump.cpp
  - tools: py_dump.py・compare_ui.py・gen_band_pixmaps.py
- native/ の新規・変更は「条件への対応」5 のとおりです。
- `.gitignore` に追記しました。qtui のソースを許可し、build/ を除外する最小限の変更です。
- コミットしていません。AGENTS.md・MIGRATION.md・スナップショット・repro/ には触れていません。

分担作業者の報告も REPORT ファイルを書けなかったので、`sandbox/omo/wp_qtui_a/prof5/REPORT_prof5.md` への転記もお願いします。

主な場所:
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/qtui/`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/nvcore_c_ui.h`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/nvcore_c_prof5.h`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/native/verify_bridge_ui.py`
- `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n3a/sandbox/omo/wp_qtui_a/` (比較画像は `cmp/`、11 本の照合ログは `log_native_verify.txt`)
