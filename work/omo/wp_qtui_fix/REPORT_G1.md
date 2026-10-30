# N3 fixes2 REPORT — G1 R/G/B イコライザ (C++ ファースト版、作業者 g1-design)

転記: 統括 (2026-10-31)。作業者は REPORT に書けないため、作業者の最終報告をそのまま転記した。2026-10-30 の C++ ファースト方針 (`native/MIGRATION.md`) により、実装は qtui/ (と qtui が呼ぶ native/ の C ABI) だけで、PySide6 版 (`repro/`) は変更していない。設計メモは `DESIGN_G1.md` (U1〜U10、ユーザーが ★ 案で承認)。両 UI 版 (PR #4) の報告は `REPORT_G1_pr4.md`。統括の再確認と独立検証は末尾に追記する。

# G1 (C++ ファースト版) 最終報告

G1 を n3fix-g1cpp でやり直し、検証もすべて済みました。共通の検証と G1 手順 4 の検証は全件一致です。既定の流れ (Auto Profile → AFT → 書き出し) の出力は、変更前の C++ ビルド (5dabc1f) とバイト一致しました。`repro/` には手を入れておらず、`git diff HEAD -- repro` は空です。コミット・stash・ブランチの操作はしていません。

## 検証結果

| 検証 | 結果 |
|---|---|
| qtui_selftest | 115/115 (main の 110 件に G1 の 5 関数を加えた数。L-1 の `g1MalformedKeys` を含む) |
| `make -C native verify` 11 本 | rc=0、すべて一致。quality 4000・band 5706・curve ALL・lzma 667/667 (genuine blob は前回と同じく worktree に無いので skip)・flat ALL・flat_cancel 676・model 1244・temporal 474・spatial 1287・profile 39/39・aft (結合 69/69) |
| `verify_bridge.py` 全群 | ALL MATCH。curve 20・profile 9・aft 26・frame 72・cancel 49・fallback 9・g8 152・prof5 110・ui・depth |
| Python 自己試験 (1 回) | 97/97 |
| `native/verify_rgb_eq.py --before-lib <5dabc1f の dylib>` | 9234/9234 (群ごとの内訳は下) |
| `qtui/tools/g1_compare.py` | ALL SAME (内容は下) |
| `compare_export` a・b・c1 | ALL SAME (C++ の書き出しと Python の書き出しがバイト一致) |

`verify_rgb_eq.py` の内訳です。
- combine 5000、effective 4000、weights 112
- apply_aft 2: `nv_aft_v5.apply_aft` をそのまま呼んだ結果と比べたもの
- legacy 120: 変更前の Y/Cb/Cr 結合 `nvc_aft_combine` と、今のライブラリ・5dabc1f のライブラリの両方で比べたもの

`g1_compare.py` で確かめたことです。
- **変更前のビルドとの一致**: test.mp4 n=300 と test_v02 n=100 の default で、処理したフレーム・曲線と sigma・Quality・パラメータ・4 フレームの CRF 0 書き出しが、5dabc1f ビルドとバイト一致しました。
- **決定論**: 7 ケースすべてで、2 回実行した結果と書き出しが一致しました。

ログは `sandbox/omo/wp_qtui_fix/g1cpp/` にあり、経過は `PROGRESS_g1.md` に書きました。

## G1 手順 4: 変更前の結合と同じになることを C++ の中で示した
- 新しい試験 `qtui_selftest::g1EditMatchesAftCombine` を作りました。中身は次のとおりです。
  1. 実フレームで AFT のバイト列を作る (`nvc_profile_build` + `nvc_aft`)。
  2. 編集した R/G/B レベルをそのバイト列の曲線語に書き込み、変更前の Y/Cb/Cr 結合 `nvc_aft_combine` に通す。
  3. 同じ編集を AppState の R/G/B イコライザで行い、有効プロファイルを作る。
- 比べた結果、Y/Cb/Cr の曲線と sigma が一致し、それぞれのプロファイルで処理したフレームもビット一致しました。
- 編集は 4 通りです。
  - R bin 4 を +20%
  - G の全ビンを +20%
  - 3 面の bin 2 を +20%
  - B bin 6 を −30%
- 「+20%」の値は `float(1.2)` にしました。AFT のバイト列は float32 しか持てず、エディタの 1.2 (double) はそのまま書き込めないためです。
- `verify_rgb_eq.py` の legacy 群は、5dabc1f のライブラリの `nvc_aft_combine` でも同じ結果になることを確かめています。今のライブラリと 5dabc1f のライブラリは別のイメージとして読み込まれていることを確認しました。

## 互換性の判断 (手順 3)
旧形式 (`nv_repro_profile`、Y/Cb/Cr の係数) の JSON は、「読み替え」と「明示エラー」のどちらでもない第 3 の扱いにしました。**旧モード**として、従来の Y/Cb/Cr の意味のまま読みます。曲線・処理・Quality は従来とビット一致し、フィクスチャ `g1_legacy_{r4,b6tune}` で確かめています。イコライザは Y/Cr/Cb を表示し、Reset・AFT・ビルドの時点で R/G/B に移ります。

この扱いにした理由は次の 3 つです。
- **読み替えは厳密にできない**: R/G/B から Y/Cb/Cr への結合は、面ごとに補間してから √ を取る非線形の式です。旧形式の 9 ビンの Y/Cb/Cr 係数から R/G/B レベルを逆算しても、同じ曲線は再現できません。解が [0.2, 3.0] の外や負になることもあります。読み替えると、旧ファイルの出力が黙って変わってしまいます。
- **明示エラーは失うものが大きい**: 既存の保存プロファイルが全部読めなくなります。PySide6 版 (main) が作るプロファイルも旧形式なので、それも読めなくなります。
- **旧モードなら壊さない**: 旧ファイルの出力を 1 ビットも変えず、編集をやり直すと自然に新形式に移ります。

明示エラーへの切り替えは `from_json` の分岐を 1 か所変えるだけです。方針を変えるならそうしてください。

## ABI の判断 (手順 4)
`NVC_ABI_VERSION` は 2 のままにしました。追加したのは次の 3 関数だけで、既存の関数の引数と戻り値は変えていません。
- `nvc_rgb_weights`
- `nvc_rgb_effective_curve`
- `nvc_rgb_combine`

`nva::combine` は内部で `color_weights` と `combine_levels` に分けましたが、`nvc_aft_combine` の動作は不変です (結合 69/69、legacy 群で 5dabc1f と一致)。`repro/nv_native.py` の版チェック (2) はそのまま通り、Python 自己試験と verify_bridge で確かめました。

## Python への依存を外した (手順 1)
- **`native/verify_rgb_eq.py`**: 参照を書き直しました。
  - `ref_combine`・`ref_effective`・`ref_weights` は、`nv_aft_v5.apply_aft` の演算順を写したものです。apply_aft は読むだけで、変更していません。
  - 参照のもとにするプロファイルは `qtui/tests/fixtures` だけにしました。
  - apply_aft を直接呼ぶ群と、変更前のライブラリと比べる群を足しました。
- **`qtui/tools/g1_py_eq.py`**: 削除しました。
- **`qtui/tools/g1_compare.py`**: C++ 専用に書き直しました (変更前ビルドとの一致と決定論)。
- **`native/verify_bridge_prof5.py`**: 作業ツリーを main の内容に戻しました。Python 側の build_profile が変わらないので、追加は不要です。

## ファイル一覧と index の状態
ステージの後にも作業ツリーを変えたファイルがあります。index を揃えてください。
- **ステージ後に作業ツリーを変更**: `native/nv_aft.h`・`native/nvcore_c_rgb.h` (コメント)・`native/verify_rgb_eq.py` (全面書き換え)・`qtui/src/app/mainwindow.cpp`・`qtui/src/core/nvcore.cpp`・`qtui/src/core/state.h`・`qtui/src/widgets/curve_editor.h` (コメント)・`qtui/tests/g1_eq.cpp` (コメント)・`qtui/tests/selftest.cpp` (`g1EditMatchesAftCombine` を追加)・`qtui/tools/g1_compare.py` (書き直し)
- **作業ツリーを main に戻した (ステージは変更ありのまま)**: `native/verify_bridge_prof5.py`
- **作業ツリーから削除 (ステージには追加のまま)**: `qtui/tools/g1_py_eq.py`
- **ステージのまま変更なし**: `native/Makefile`・`native/nv_aft.cpp`・`native/nvcore_c_rgb.cpp`・`qtui/CMakeLists.txt`・`qtui/src/core/state.cpp`・`qtui/src/widgets/curve_editor.cpp`・`qtui/src/widgets/profile_panel.cpp`・`qtui/tests/fixtures/g1_legacy_*` (4 件)
- **sandbox (未追跡)**: `DESIGN_G1.md`・`PROGRESS_g1.md`・`REPORT_G1_pr4.md`・`g1cpp/` (ログ)

変更前のビルド (5dabc1f の native・qtui・data に `g1_eq.cpp` を足し、`after_aft`・`reset_fine_tune`・`auto_complete` を旧 MainWindow と同じ動作で書いたもの) は scratchpad の `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/g1cpp_before` にあります。

---
## 統括の再確認 (2026-10-31)
- `git diff --cached origin/main -- repro` は空 (PySide6 版・処理コアは無変更)
- `make -C native lib`・`cmake --build qtui/build_main`: 警告 0。`qtui_selftest` 115/115 (`sandbox/omo/coord_g1/rerun_lead_cpp.log`)
- `native/verify_rgb_eq.py --before-lib <5dabc1f の dylib>`: ALL SAME 9234/9234 (apply_aft 2/2、legacy 120/120)
- `qtui/tools/g1_compare.py` を作業者が使っていない test.mp4 n=450 で実行 (default・r4・reset): default は変更前の C++ ビルドとフレーム・曲線/sigma・Quality・書き出し・パラメータがすべて同じ、各ケースの 2 回実行も同じ、ALL SAME (`rerun_lead_g1cmp.log`)
- 独立検証 Round 3 (`sandbox/omo/coord_g1/VERDICT_g1cpp.md`): **APPROVE** (M 0・L 1・I 4)
  - L-1: `g1EditMatchesAftCombine` の 2 経路が、共通の `combine_levels`・`color_weights` を呼んでいた。そのため、この試験だけでは変更前との一致の証明にならない。
  - 変更前との一致そのものは、別の照合で確かめられていた。旧 dylib との比較で 120/120、apply_aft を基準にした照合で 12/12。

## L-1 (Round 3) の修正 (作業者 g1-design、2026-10-31)
手順 4 の証明は、qtui_selftest の `g1EditMatchesAftCombine` で行う。

- **期待値:** 変更前のコード (5dabc1f) の `nvc_aft_combine` と nvcore が出した値を固定したフィクスチャ `qtui/tests/fixtures/g1_before_combine.json`。作り直しは `qtui/tools/g1_fixture/make_g1_fixture.sh` で行う。
- **比べるもの:** フィクスチャと次の 2 点をビット比較する。
  - 今の R/G/B イコライザ (AppState → `rgb_effective_profile`) が出す Y/Cb/Cr の曲線と sigma
  - 処理したフレームの FNV-1a 64 ハッシュ
- **入力の確認:** 最初に、今のコードの AFT の結果 (aft_levels と AFT 後の曲線) がフィクスチャと一致することを確かめる。
- **編集:** 4 通り。レベルは float32 (`float(1.2)`・`float(0.7)`) を使う。AFT のバイト列が float32 しか持てないため。
- **空振りしないこと:** フィクスチャ側とコード側に 1 ulp の変異を入れると、どれも FAIL になる。
- **結果:** qtui_selftest 115/115。
- **注意 (I-6):** フィクスチャは test.mp4 のフレーム 4 用。`NV_TEST_VIDEO` に別の動画を渡すと、入力の確認で理由を出して落ちる。

統括の再確認: qtui のビルドは警告 0、`qtui_selftest` は 115/115 (`sandbox/omo/coord_g1/rerun3_cpp_st.log`)。

独立検証 Round 4: **APPROVE** (新しい指摘は I 3 件)。
- 検証者が自分で取り出した 5dabc1f のツリーでフィクスチャを作り直し、sha256 まで一致した。
- 生成ツールは 5dabc1f のライブラリだけでビルドできた。
- Round 3 の試験では PASS のままだった共有部の変異 (float の sqrt) が、新しい試験では FAIL になる。循環は解消した。
- I-7: `make_g1_fixture.sh` は worktree のフィクスチャを直接上書きする。一致を確かめるだけのときのために、出力先を引数で指定できるとよい (記録のみ)。

---
## main の取り込みと ABI 3 (統括、2026-10-31)
- ユーザーの判断 (`.claude/tasks/native_n3_fixes2.md` の「統括の回答」) を受けて、次の 2 つを行った。
  - origin/main (c6d497b) を取り込んだ (4b5e510)。
  - `NVC_ABI_VERSION` を 3 に上げた (53a52e1)。
- `repro/nv_native.py` は版数の完全一致を確かめているので、同じコミットで `ABI_VERSION = 3` にした。
  - 上げないと、Python の native 経路が黙って使われなくなり、verify_bridge も通らない。
  - `repro/` への変更はこの 1 行だけ。
- 共通 4 検証 (統括の実行、ログは `sandbox/omo/coord_g1/abi3/`):
  - `make -C native lib`・qtui のビルドは、どちらも警告 0。`nv_native.LIB_PATH` は worktree の dylib を指し、`nvc_abi_version()` = 3。
  - `qtui_selftest`: 115/115。Python 自己試験 (NV_NATIVE=1): 97/97。
  - `make -C native verify` 11 本: すべて一致 (ALL BIT-EXACT)。
  - `verify_bridge.py` 全群 ALL MATCH: curve 20・profile 9・aft 26・frame 72 (3.06 億語)・cancel 49・fallback 9・g8 152・prof5 110 語・ui 34,755 語・depth 2.09 億語。
  - `compare_export.py`: ALL SAME (C++ と Python の書き出しのバイト一致)。
  - `g1_compare.py` (test_v02 n=100、default・r4): ALL SAME。default は変更前の C++ ビルドと一致した。
- 独立検証 Round 5 (`sandbox/omo/coord_g1/VERDICT_g1cpp.md`): **APPROVE** (指摘は I-8 の 1 件だけ)
  - 版数の値を比べているのは `repro/nv_native.py:77` だけで、参照漏れはない。
  - 版数 3 の dylib が実際に使われていることを確かめた。
  - 版数 2 の古い dylib を置いた場合:
    - Python 側は黙って Python の経路に落ちる。出力は同じで、遅くなるだけ (G1 より前からの挙動)。
    - C++ 側は `nvc_rgb_*` のシンボルが無いので、起動時に止まる。
  - `verify_bridge --only fallback,cancel`: ALL MATCH。
  - I-8: `native/MIGRATION.md`・`AGENTS.md` の版数の記述 (2) の更新は、ユーザーが行う。
- 統括の判断として記録する: C++ ファースト方針のもとで、`repro/` の変更は版数の追従 (`ABI_VERSION`) に限って行った。
