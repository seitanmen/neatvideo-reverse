# N3 fixes2 REPORT — G1 R/G/B イコライザ (作業者 g1-design)

転記: 統括 (2026-10-30)。作業者は REPORT に書けないため、作業者の最終報告をそのまま転記した。設計メモ (U1〜U10、ユーザーが全項目 ★ 案で承認) は `DESIGN_G1.md`。ほかの PR と同じパスに別の内容を足さないよう `REPORT_G1.md` とした。統括の再確認と独立検証は末尾に追記する。

# G1 報告: イコライザの R/G/B 化 (U1〜U10 すべて ★ 案で実装)

G1 の実装と検証は終わりました。共通規約の 4 つの検証と、設計メモ §6.2 の試験はすべて一致しています。編集しない既定の流れ (ビルド → 自動 AFT → 書き出し) の出力は、変更前のコードの出力と両 UI でバイト一致しました。出力が変わるのは U5 (Reset Fine-Tuning) だけで、変化の大きさは §3 に書きました。

作業した場所は worktree `.claude/worktrees/n3fix-g1` (branch n3fix-g1、df46dda から) です。コミット・stash・ブランチの操作はしていません。処理コアの Python (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) と `repro/nv_native.py` は変更していません。設計メモは `sandbox/omo/wp_qtui_fix/DESIGN_G1.md` (末尾に決定事項を追記) で、経過は `sandbox/omo/wp_qtui_fix/PROGRESS_g1.md` にあります。

**NVC_ABI_VERSION は上げていません。** C ABI には関数を 3 つ追加しただけで、既存の関数の引数と戻り値は変えていないからです。G7・G8 が版を 2 に上げているので、マージ時に揃えてください。追加した関数は次の 3 つです。
- `nvc_rgb_weights(const float* rgb, int H, int W, const int* area4, double* cw9)`
- `nvc_rgb_effective_curve(const double* centers, int n, const double* src, const double* cw9, const double* levels27, const double* tune4, double area_level, const double* tot_ratio, double* curve_out, double* sigma_out)`
- `nvc_rgb_combine(const double* centers, int n, const double* curve, const double* cw9, const double* levels27, double* face_out, double* curve_out)`

## 1. 実装の要点 (両 UI 共通、[観測] は実行で確かめたもの)

**イコライザが持つ値**
- 新しく作るプロファイルの `ProfileState.factors` は、R/G/B の 9 ビンの「レベルそのもの」です。乗数ではありません。
- AFT・手動の編集・Reset は、どれもこのレベルを上書きします。
- 曲線の表示とトグルは本家と同じく R/G/B で、トグルは [RGB][R][G][B] です。AFT の後は AFT のレベルが表示されるので、曲線が曲がって見えます。いまの repro は AFT の後も平らに見えていました。

**有効曲線の作り方**
- 有効曲線 = AFT 前の曲線 × √(max(Σ_j c_cj·interp(L_j), 0))。これは `apply_aft` の 4〜6 段と同じ演算順です。
- その後に tune を float32 で ×f32(tk) します。L が基準のままのときは、いまの tune だけの経路と同じビットになります [観測]。
- 次の 2 つの場合は、計算せずに元の値をそのまま返します。
  - L が基準 (AFT 前は 1.0、AFT 後は `aft_levels`) と等しく、tune が 1 のとき → `base` をそのまま返す。
  - AFT 後に L が 1.0 で tune が 1 のとき → AFT 前の曲線と sigma をそのまま返す (U5)。

**AFT のレベルの取り出し**
- Python UI の `backend._run_aft` は、9 ビンの中心を curve_centers にした代理のプロファイルで `apply_aft` を 1 回だけ呼び、レベル 27 個を得ます。そのあと実際の曲線を結合式で作ります。結果は `nv_aft_v5.apply_aft` を直接呼んだものとビット一致しました [観測: 自己試験の ON/OFF]。
- C++ UI は AFT のバイト列から直接読みます。

**プロファイルの base に足したキー**
- ビルド時: `ui_rgb_weights`。c_cj を `nv_band.model_levels` の q99 から作ります。AFT の c_cj と一致しました [観測]。
- AFT の後: `aft_levels`。

**決定事項の実装**
- Quality (U3・U4):
  - レベル語は、手で編集したビンだけ L、それ以外は 1.0 です。tune は入れません。
  - 旧モードは現状維持です。並びの食い違い (U10) もそのまま残しました。
- Auto Complete (U6): 固定点は「編集したビン ∪ AFT の測定ビン」です。
- 新しいファイル形式 (U8): `"format": "nv_repro_profile_rgb", "version": 2, "channels": "RGB"`。
- 旧形式のファイル (U9): Y/Cr/Cb の旧モードで読み、数値は従来とビット一致します。Reset・AFT・ビルドで RGB に移ります。

## 2. 検証結果 (すべて最終コードで実行)

| 検証 | 結果 |
|---|---|
| Python 自己試験 | 108/108 (NV_NATIVE=1)・108/108 (NV_NATIVE=0)。G1 の 12 項目を追加し、既存の 3 項目を RGB の意味に合わせて直した |
| qtui_selftest | 108/108 (G1 の 3 関数を追加) |
| `make -C native verify` 11 本 | すべて一致。quality 4000・band 5706・curve ALL・lzma 667/667・flat ALL・flat_cancel 676・model 1244・temporal 474・spatial 1287・profile 39/39・aft (結合 69/69 は分割後も不変) |
| verify_bridge | curve・profile・aft・frame・cancel・fallback・ui・depth は全語一致。prof5 は検証側の C ラッパに `ui_rgb_weights` がなく 0/1 になったので、検証スクリプトを直して 110/110 |
| `native/verify_rgb_eq.py` (新規。Python と C ABI の照合) | 8983/8983 |
| `qtui/tools/g1_compare.py` (新規) | ALL SAME (詳細は下) |
| compare_export a・b・c1 | ALL SAME。6 ファイルとも変更前の書き出しとバイト一致 |
| compare_ui (py_dump と qtui_dump) | RESULT EQUAL。画面のグラブ 0 画素差、ペイン 232/232。Help メニューの "About Qt" 1 件の差は G1 と無関係の既存の差 |

lzma の genuine blob の照合は skip です。お知らせのとおり、この worktree に blob が無いためです。

`g1_compare.py` は test.mp4 n=300 と test_v02 n=100 で、次の 7 ケースを比べました。
- 編集なし (default)
- R bin 4 を +20%
- G の全ビンを +20%
- All bin 2 を +20%
- B bin 6 を −30% にして tune all を 1.1
- Reset
- R bin 4 +20% の後に Auto Complete

各ケースの結果です。
- プロファイル JSON・有効プロファイル・パラメータ・Quality・処理したフレームは、両 UI でバイト一致しました。
- 書き出しは、Python (native ON)・Python (OFF)・C++ の 3 つがバイト一致しました。
- 編集なしのケースは、処理したフレームも書き出しも、変更前のコードの出力と両 UI でバイト一致しました。

旧形式のフィクスチャ (変更前のコードが保存したもの) は、両 UI の自己試験で、変更前と同じ有効プロファイル JSON のテキストとして読めました。

compare_ui を実行したのは、最後の 2 つの小さな修正 (未使用の定数と docstring、`quality.level_words` の stand-in への対応) の前です。実在の ProfileState の動作には影響しません。

## 3. U5 (Reset) で出力がどう変わるか (test.mp4 n=300、両 UI で同じ値)

| | 変更前 | 変更後 |
|---|---|---|
| Reset 後の曲線 | AFT 後のまま。出力は編集なしと同一 | AFT 前の曲線と sigma に正確に戻る。出力は AFT を掛けないプロファイルの出力とビット一致 [観測] |
| Reset 後の曲線比 (変更後 ÷ 変更前) | — | Y 0.968〜1.0016、Cb 0.9686〜0.9996、Cr 0.9729〜0.9929 |
| 処理したフレームの差 (変更後 − 変更前) | — | Y 最大 1.247・RMS 0.064、Cb 最大 0.095・RMS 0.005、Cr 最大 0.156・RMS 0.017 |
| Quality / Completeness | 25% / 0% | 25% / 0% (変わらない) |

## 4. 変更したファイル

**変更 (19 件)**
- Python UI: `repro/nv_ui_qt/state.py`・`backend.py`・`main_window.py`・`quality.py`・`selftest.py`・`widgets/curve_editor.py`・`widgets/profile_panel.py`
- native: `native/nv_aft.{h,cpp}` (combine を `color_weights`・`combine_levels` に分け、`rgb_effective` を追加)・`native/Makefile` (LIB_SRC と LIB_HDR に追加)
- C++ UI: `qtui/src/core/state.{h,cpp}`・`qtui/src/core/nvcore.cpp`・`qtui/src/widgets/curve_editor.{h,cpp}`・`qtui/src/widgets/profile_panel.cpp`・`qtui/src/app/mainwindow.cpp`・`qtui/tests/selftest.cpp` (G1 の試験は末尾にまとめた)・`qtui/CMakeLists.txt` (`qtui_g1eq` を追加)

**新規 (11 件)**
- `native/nvcore_c_rgb.{h,cpp}`・`native/verify_rgb_eq.py`
- `qtui/tests/g1_eq.cpp`・`qtui/tools/g1_py_eq.py`・`qtui/tools/g1_compare.py`
- `qtui/tests/fixtures/g1_legacy_{r4,b6tune}.{nvprof,ui_profile}.json`

**検証スクリプトの修正**
- `native/verify_bridge_prof5.py`: C 側に `ui_rgb_weights` を足した (上の prof5 の件)。

**統括が突き合わせるときの注意**
- fix2-b が触っている `backend.build_profile` に 1 行足しています。
- fix2-a が触っている `selftest.cpp` には、関数を末尾に追加しました。
- `nvcore_c_prof5` には手を入れていません。
- `.omc/state/*` の変更は hooks によるもので、私の変更ではありません。

## 5. 文書の更新案 (AGENTS.md・MIGRATION.md は触っていません)

**AGENTS.md の nv_ui_qt 節**
- 「本家と違う点: イコライザは Y/Cr/Cb」を削除する。
- 代わりに次を追記する。
  - イコライザは R/G/B のレベルで、`state.rgb_effective_profile` が apply_aft と同じ結合式で Y/Cb/Cr の曲線を作る。
  - 新形式は `nv_repro_profile_rgb`、旧形式は旧モードで読む。
  - Reset は AFT 前の曲線に戻す。
  - Quality のレベル語は、編集したビンだけ L で、tune は入れない。

**MIGRATION.md**
- G1 の節を足す。内容は、追加した ABI 3 関数、`nva::combine` の分割 (69/69 は不変)、検証の数値。

## 6. 残っている点と注意

- U1・U2 は記録だけです。結合式は L を分散の乗数として扱っているので、All を +20% にすると σ は ×√1.2 になります。R/G/B の曲線は Y の中心で評価しています。
- 新形式のファイルは、変更前の UI (たとえば N2b の .app) では "not a repro profile file" として拒否されます。これは U8 で決めたとおりの動作です。
- `sandbox/omo/wp_qtui_fix/g1/` の容量は合計約 1.1 GB です (before 101 MB・after 719 MB・final 287 MB)。ほとんどは after のダンプと verify_bridge の一時ファイルです。削除していないので、要否を決めてください。

---
## 統括の再確認 (2026-10-30 夜)
- `make -C native lib`・`cmake --build qtui/build_main`: 警告 0。`qtui_selftest` 108/108 (`sandbox/omo/coord_g1/rerun_lead.log`)
- `native/verify_rgb_eq.py`: ALL SAME 8973/8973 (combine 5000・effective 3853・weights 112・aft 8)。作業者の 8983 と件数が違うのは、入力の一部を `**/*.json` の保存済みプロファイルから glob で集めるため (ディスク上のファイルで件数が変わる。乱数のシードは固定)
- Python 自己試験 (NV_NATIVE=1): 108/108、rc 0 (`sandbox/omo/coord_g1/rerun_py_st_on.log`)
- マージの下見 (`git merge-tree`): n3fix-b・n3fix-c とは衝突なし。n3fix-a とは `qtui/CMakeLists.txt` の末尾 (試験用の実行ファイルを 1 つずつ追加) だけが衝突し、両方残せば解消する
- ABI の版数: G7/G8 が 2 に上げている。G1 は関数を 3 つ追加するので、統合時に 3 に上げることを提案する
- 独立検証: `sandbox/omo/coord_g1/VERDICT_g1.md` — **APPROVE** (M 0 件、L 1 件、I 5 件)。L-1: 形の崩れた RGB 形式のプロファイル (`ui_rgb_weights` などの形・型の誤り) を読むと C++ UI が SIGSEGV、両 UI で結果も違う。UI が書いたファイルでは起きない。PR に追加のコミットで直す
