# N3 fixes2 REPORT — G3・G4・G5・G6・G2 (作業者 fix2-a)

転記: 統括 (2026-10-30)。作業者は REPORT.md に書けないため、作業者の最終報告をそのまま転記した (検証の指摘 L1〜L3・I3・I5 への対応を含む最新版)。統括の再確認と独立検証は末尾に追記する。

PR 用に `REPORT.md` から改名した。G2〜G6・G7/G8・G9 の 3 つの PR が同じパスに別の内容を足して衝突しないようにするため。共通の `REPORT.md` へのまとめは G1 の完了後に行う。

# fix2-a 最終報告: G3・G4・G5・G6・G2 と検証指摘 (L1〜L3・I3・I5) への対応

`n3fix-a` の G2〜G6 と、検証の指摘 L1〜L3・I3・I5 への対応がすべて終わりました。共通規約の 4 検証(Python 自己試験、qtui_selftest、`make -C native verify` と verify_bridge、書き出しのバイト一致)も通っています。

- **追補(指摘への対応)の再確認:** すべて一致・合格です。qtui の再ビルドは警告 0、`qtui_selftest` は 110/110、`compare_ofx_args.py` は 50 ケースすべて一致し、実際の `python3 -m nv_ui_qt.main` とも traceback が一致しました。極端サイズの比較は 4810 ジョブすべて一致、Python 自己試験(native ON)は 97/97 で終了コード 0 でした。
- **基準:** 「観測」は実行した結果です。「推測」と書いたもの以外はすべて観測です。
- **作業の範囲:** コミット、git stash、ブランチ操作はしていません。AGENTS.md・MIGRATION.md・処理コアの Python・`repro/out/` には触れていません。
- **ファイルの場所:** ログは `sandbox/omo/wp_qtui_fix/a/logs/`、変更前の控えは `a/before/`、進捗は `sandbox/omo/wp_qtui_fix/PROGRESS_a.md` です。
- **背景資料:** 指示された `VERDICT_n3c.md` と `independent/ind_ofx.py` は見つかりませんでした。L-2 のケースは `wp_qtui_c/REPORT.md` と `l1/log_ind_ofx_args.txt` から取りました。

## G3: OFX 引数解析を Python の int()/float() に合わせた

- **実装:** 新規の `qtui/src/core/pynum.{h,cpp}` に CPython 3.13 の手順を移しました。
  - int: 前後の空白、符号、数字の間に 1 個ずつの `_` を許します。`0x10` のような接頭辞は受け付けず、4300 桁を超えると ValueError です。
  - float: `_` を検査したうえで 10 進の文法で読みます。inf・infinity・nan は大文字小文字を問わず受け付けますが、16 進の浮動小数と `nan(…)` は受け付けません。値は C ロケールの `strtod_l` で正しく丸めます。
  - argv が UTF-8 として不正な場合は C++ では U+FFFD、Python では lone surrogate になりますが、どちらも usage で終わります。
- **Unicode の対応範囲:** Python 3.13.5 の unicodedata(Unicode 15.1.0)にある非 ASCII の Nd 全部です。67 区間 × 10 の 670 文字で、先頭は `0x660` から `0x1FBF0` まであります(`PY_ND_ZEROS`)。非 ASCII の空白 19 文字は空白として扱い、それ以外の文字はそこで打ち切ります。
- **付随の修正:** OFX ラベルの FPS が `Infinity` と表示されていました(Python は `inf`)。`py_float_str` を追加して、ラベル・`info.fps`・書き出しの sigma 表示に使いました。
- **検証:**
  - `qtui/tools/compare_ofx_args.py` の 50 ケースはすべて一致しました(端値 16/16、追加 20/20、サイズ 14/14)。比べたのは終了コード、stdout のバイト列、MSG と INFO の行、stderr の最終行、traceback のフレームです。
  - 端値には、L-2 の 4 件(1e400・1e-400・1e-310・`0x10`、Unicode 数字の W)と、N3c の相違点 3 の `1_000`・`0x1p4` を入れています。
  - ファズ 800,159 文字列で不一致 0 でした(`a/pyfuzz/`、`logs/log_pynum_fuzz.txt`)。compare_ofx も全件一致です。
- **タスク文との違い:** タスク文の例(`ValueError: invalid literal…`)と違い、実際の Python は ValueError を捕まえて usage を出し、終了コード 1 で終わります。実際の挙動に合わせました。

## G6: エラー出力の整形

- **最初の読み込みで落ちるサイズ:** frame_size を 128 ビットで計算し、Python の `stdin.read(fs)` が出す例外を traceback ごと再現しました。最終行は次のとおりです。
  - fs が 2^63 以上: `OverflowError: cannot fit 'int' into an index-sized integer`
  - fs が 2^63−1−33 を超える: `OverflowError: byte string is too large`
  - 確保に失敗: `MemoryError`。C++ でも `malloc(33+fs)` を実際に試して判定しています。
- **極小フレーム(新たに見つけた出力の差):**
  - 動きブロック(16px)未満のフレームで、1 フレームのときや時間フィルタが off のとき、Python は書き出しますが C++ は書き出しませんでした。
  - `nvcore.cpp` に Python の `nvfilter.temporal_v5` と同じ分岐と例外文を入れました。例外文は np.gradient、np.pad の空の軸 0/1、zero-size の max の 4 種です。近傍がないときは同じ float32 の式でそのまま通します。
- **検証:**
  - `compare_emit_sizes.py`(新規ターゲット `qtui_emit_sizes`)の 4810 ジョブはすべて一致しました。バイト一致が 2967 件、例外文 4 種が計 1843 件です。わざと壊した版では 440 件の差を検出しました。
  - compare_ofx_args のサイズ 14 ケースも一致しています。

## G4: 自己試験の強化

- **#7 componentPanesEveryMode:** 7 モードすべてについて、ペイン名とその順番を Python の `PANES` と照合します。
- **#9 pcheckRecomputesOnNoiseLevel:** スライダーで値を変え、次を確かめます。テスト用に `pc_cached()` を追加しました。
  - 結果がキャッシュに入ること
  - 画像が `rot90(engine.assist(...))` と一致すること
  - undo でキャッシュの画像に戻ること
- **#96 bitDepth10ExportEqualsProcessFrame:** ソースのフレームをファイルから読み直し、書き出しが残した `.ui_profile.json` を使って照合します。
- **項目 20:** 新規の `nlcPanelYellowAndDisabled` を追加しました。
- **わざと壊すデモ(`a/g4_demo/`):** 3 件とも、旧項目は PASS、新項目は FAIL でした。壊し方は `*.break.diff`、結果は `SUMMARY.txt` にあります。ソースは元に戻してあります。

## G5: 単項目実行で落ちないようにした

- **仕組み:** `init()` で、ウィンドウがなければ test.mp4 を開き、Auto Profile と AFT まで済ませます。前の項目に依存していた項目には補助関数を付けました(make_v10・ensure_v10_at_10・show_filtered_adjust・build_rect_profile・ensure_nlc_on)。
- **全体実行での補助関数の挙動(L3 で訂正):** 前回の報告の「全体実行では何もしない」は不正確でした。正しくは次のとおりです。
  - `ensure_window` と、表示待ち以外の補助関数は、全体実行では条件がすでに満たされているので何もしません。
  - ただし `show_filtered_adjust` は performanceTable で動いています。すでに Adjust タブにいて、filtered の表示が出るまで待つだけなので、状態を変えず判定にも影響しません。
  - `make_v10` は bitDepth10Reload で src10.mkv を作ります。これは元からの動作です。
- **検証:** `qtui/tools/selftest_each.py` で全項目を 1 つずつ実行し、108/108 PASS でした。直す前は 95/105 でした。

## G2: ffmpeg 欄(両 UI)

- **共通化:** 新規の `repro/nv_ui_qt/ffmpeg_path.py` に、探索・PATH への前置・ダイアログの文言をまとめました。探索順は `NV_FFMPEG` → `prefs/ffmpeg` → 固定の 3 か所 → `PATH` です。PySide6 の `main.py` も起動時に探索し、見つからなければ C++ と同じダイアログを出します。`packaging/main.py` はこのモジュールを使う形にしました。
- **Preferences の行:** 両 UI に「ffmpeg (directory or binary)」の欄と Browse… ボタンを付けました。
  - ffmpeg と ffprobe がないパスでは、警告を出してダイアログを閉じず、何も保存しません。
  - 有効なパスなら保存して、その場で使います。
  - 行を両 UI で揃えるため、Python 版に「Auto Fine-Tune after Auto / Build Profile」の行も足しました。
- **試験:** 両方の自己試験に項目を追加しました。`packaging/test_launcher.py` は 13/13 です。compare_n3c では Preferences が 11 行対 11 行で一致し、違うのは C++ で無効表示の native 欄だけです。grab の大きさも同じ(342×450)になりました。

## 指摘への対応(追補)

- **L2(行番号):** C++ の traceback の行番号を今の Python に合わせました。
  - 行番号は main.py が 157・34・73、ofx_bridge.py が 110・39 です。
  - Python 側の該当行の直前に、C++ がこの行の番号と文字列を写している旨の注記を入れました。注記の行を足した分、番号は指摘時の 154・33・71 からさらに動いています。
  - `compare_ofx_args.py` で、traceback の nv_ui_qt のフレーム(ファイル・行・関数・ソース行)を照合するようにしました。MemoryError の 1 件は、実際の `python3 -m nv_ui_qt.main` でも照合し、5/5 フレームが一致しました。
  - 行番号を古い値に戻した版では差として検出されます(`logs/log_compare_ofx_args_lineno_mutant.txt`)。
- **I5:** `selftest_each.py --only` に存在しない名前を渡したとき、または 1 件も実行しなかったとき、終了コード 1 になるようにしました。
- **I3(PATH の差):** 指摘どおり差が出ることを確かめ、直しました。
  - 再現した状況: 固定の 3 か所に ffmpeg がなく、元の PATH には Z にだけある環境です。Preferences を X にしたあと欄を空にすると、Python は自分で PATH の先頭に足した X をまた見つけていました。C++ は元の PATH から Z を見つけます。Python 側の結果は `a/i3/i3_scenario.py`(`log_before_fix.txt`)で確かめました。C++ 側の結果はコードから読んだものです。この機械では C++ の固定の場所(/opt/homebrew/bin)を外せず、C++ では再現していません。
  - 修正: `ffmpeg_path` の探索が、自分で前置する前の PATH を読むようにしました(`search_path`、`NV_FFMPEG_PATH0` と `NV_FFMPEG_PATH_SET`)。PATH が外から変えられていれば、今の PATH を読みます。
  - 修正後は両 UI とも Z になります(`log_after_fix.txt`)。`test_launcher.py` にこの状況の確認を足しました。
- **L1(MemoryError の範囲):** 前回の報告の「2^46〜2^47 はプロセスごとに違い得る」は誤りでした。正しくは次のとおりです(検証者の観測。SDK の版は私も otool で確かめました)。
  - fs が 2^46 以上、約 1.0507e14 未満のとき、実際のアプリどうしで stderr の最終行が必ず違います。Python は `no frames received on stdin`、C++ は `MemoryError` の traceback を出します。終了コード 1 と stdout(0 バイト)は同じです。
  - 原因は、実行ファイルがリンクされた SDK による malloc の上限の違いです。anaconda の python3.13 は sdk 11.1 で上限が約 1.0507e14、qtui は sdk 26.5 で約 2^46 です。
  - 64 TiB を超えるフレームは実用上ありえないので、コードは変えていません。`ofx.cpp` のコメントだけ、この内容に直しました。

## MIGRATION 追記案(既知差、ユーザーが反映)

> - **OFX の MemoryError の範囲(N3 fixes の G6、検証 L1):** C++ UI は、Python の最初の `stdin.read(fs)`(内部で `malloc(33+fs)`)を同じ大きさの malloc を試して再現している。malloc の上限は、実行ファイルがリンクされた SDK によって変わる(LC_BUILD_VERSION)。sdk 26 でリンクした qtui は約 2^46 バイト、参照に使う anaconda の python3.13(sdk 11.1)は約 1.0507e14 バイトである。このため 2^46 ≤ fs < 約 1.0507e14 では、Python が「no frames received on stdin」、C++ が `MemoryError` と出し、stderr の最終行だけが違う。終了コード 1 と stdout が空であることは同じ。この範囲外の OverflowError 2 種と MemoryError は一致する。参照に使う Python を替えたら上限を測り直すこと。

## 共通規約の検証(全件、前回)

| 検証 | 結果 |
|---|---|
| Python 自己試験 | 97/97(native ON・OFF とも終了コード 0) |
| qtui_selftest | 110/110(以前は 105) |
| 単項目実行 | 108/108 |
| `make -C native verify` | 11 本すべて ALL BIT-EXACT |
| verify_bridge | ALL MATCH(687 秒) |
| 書き出し比較(compare_export) | 19/19 一致(ON・OFF とも) |
| compare_n3c | 一致(差は native 欄だけ) |
| qtui クリーンビルド | 警告 0 |

- **終了時の停止(既存の問題):** Python 版は native OFF のとき、まれに全項目 PASS のあと終了時に止まります。viewer の ThreadPoolExecutor が np.dot(OpenBLAS)から戻らないためです。HEAD の版でも 2 回中 1 回起きました(`logs/log_py_selftest_native0_HEAD_exit_hang.txt`)。私の変更が原因ではありません。

## マージ時の注意

- `repro/nv_ui_qt/ffmpeg_path.py` は `.gitignore` の `repro/**` に掛かるので、`git add -f` が必要です。漏れると main.py と packaging/main.py の起動が壊れます。

## 変更・新規ファイル

- **qtui の変更:** `src/app/ofx.{h,cpp}`・`mainwindow.{h,cpp}`、`src/core/nvcore.cpp`・`pyobj.{h,cpp}`・`export.cpp`・`video.{h,cpp}`、`tests/selftest.cpp`・`ofx_drv.cpp`・`dump_util.h`、`tools/compare_ofx.py`・`compare_n3c.py`・`py_dump_n3c.py`・`py_ofx_drv.py`・`compare_export.py`、`CMakeLists.txt`
- **qtui の新規:** `src/core/pynum.{h,cpp}`、`tests/emit_sizes.cpp`、`tools/compare_ofx_args.py`・`compare_emit_sizes.py`・`selftest_each.py`
- **repro の変更:** `nv_ui_qt/main.py`・`main_window.py`・`selftest.py`・`ofx_bridge.py`(注記行のみ)
- **repro の新規:** `nv_ui_qt/ffmpeg_path.py`
- **packaging の変更:** `main.py`・`test_launcher.py`

## 質問(ユーザーの返事待ち、現状のまま)

1. Python 版 Preferences に Auto Fine-Tune の行を足して、両 UI の行を揃えた点。
2. MemoryError の判定に malloc を実際に試す方式。
3. 終了時の停止(既存の問題)への対応。

---
## 統括の再確認 (2026-10-30)
- `cmake --build qtui/build_main` (差分ビルド) 成功。`qtui_selftest` 全体 110 passed / 0 failed (`sandbox/omo/coord_fix2a/rerun_cpp_st.log`)
- `qtui/tools/compare_ofx_args.py`: edge 16/16・extra 20/20・size 14/14、ALL SAME (`rerun_ofx_args.log`)
- Python 自己試験 (NV_NATIVE=0) 97/97、終了コード 0 (`rerun_py_st_off.log`)
- 独立検証: `sandbox/omo/coord_fix2a/VERDICT_g2_6.md` — Round 1 条件付き APPROVE (LOW 3 件) → Round 2 APPROVE
- 追補の後 (統括、2026-10-30 夕): qtui の差分ビルドは警告 0。`compare_ofx_args.py` は edge 16/16・extra 20/20・size 14/14 で ALL SAME (実際の `python3 -m nv_ui_qt.main` との traceback は 5/5 フレームが一致)。`packaging/test_launcher.py` は 13/13。ログ `sandbox/omo/coord_fix2a/rerun2*.log`
- I3 修正後の全体実行 (統括): `qtui_selftest` 110/110、Python 自己試験 (NV_NATIVE=1) 97/97。ログ `sandbox/omo/coord_fix2a/rerun3_*.log`
- 独立検証 Round 2: **APPROVE** (L1〜L3・I3・I5 すべて解消。新規は INFO 1 件 R2-I1: PATH_SET が今の PATH と同じで PATH0 が無関係な値のとき見つからない。手で設定しない限り起きない。記録のみ)
