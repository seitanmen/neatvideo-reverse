# G1 (C++ ファースト版) 独立検証 判定 — Round 3 (verify-g1、2026-10-31)

対象: worktree `n3fix-g1cpp` のステージ済み差分 (`git diff --cached origin/main`、基点 5dabc1f、21 ファイル)。
仕様: `.claude/tasks/native_n3_fixes2.md` の G1 (手順 2〜4 が C++ ファースト方針で更新済み)。報告: `sandbox/omo/wp_qtui_fix/REPORT_G1.md`。
記号: **[観測]** = 自分で実行・読んで確かめたこと、**[推測]** = 確かめていないこと。

## 判定: **APPROVE**

重大度 M の指摘はない。L が 1 件ある (手順 4 の自己試験が、それだけでは変更前との証明になっていない)。ただし、変更前との一致は別の独立な経路で確かめられたので、マージは止めない。I は 4 件。

---

## 0. 差分の範囲 [観測]
- `repro/` は、ステージ済みの差分も未ステージの差分も空。
- index と作業ツリーは一致している (`git diff --stat` は空)。REPORT の「index を揃えてください」は解消済み。`qtui/tools/g1_py_eq.py` はなく、`native/verify_bridge_prof5.py` は main と同じ。
- 製品コードの C++ は、Round 1・2 で検証した n3fix-g1 版と同じ。
  - `native/nv_aft.cpp`・`native/nvcore_c_rgb.cpp`・`qtui/src/core/state.cpp` (L-1 の `check_rgb_keys` を含む)・`curve_editor.cpp`・`profile_panel.cpp` はファイルが一字一句同じ。
  - `nvcore.cpp`・`mainwindow.cpp` の G1 の差分は、Round 1 と同じ変更 (`ui_rgb_weights`・`aft_levels`・Quality の写像・`after_aft`・`reset_fine_tune`・`auto_complete`)。
  - ヘッダはコメントだけが変わっている。
- ビルドは警告 0。

## 1. 手順 4 の証明 `g1EditMatchesAftCombine` は本当に証明になっているか [観測]
比べている 2 本の経路は次のとおり。

| | 経路 |
|---|---|
| A (UI) | AppState → `rgb_effective_profile` → `nvc_rgb_effective_curve` → `nva::rgb_effective` → **`nva::combine_levels`** |
| B (「変更前の結合」) | `nvc_aft_combine` → `nva::combine` → `color_weights` + **`nva::combine_levels`** |

- G1 で `nva::combine` を分けたので、B も今は A と**同じ `combine_levels` を呼ぶ** (`native/nv_aft.cpp:687` と `:711`)。c_cj も、どちらも同じ `color_weights` から来る。
- したがってこの試験が独立に確かめているのは、計算そのものではなく、その周りだけである。
  - after_aft・set_factors の clamp・rgb_reference と変更なしなら base を返す判定・PyObj の受け渡し・tune・sigma の別実装。
  - それぞれのプロファイルで処理したフレームの一致。
- 「変更前の結合と同じ」ことは、この試験だけでは示せていない (→ **L-1**)。
- ただし、変更前との一致は次の 2 つの独立な経路で成り立つ。
  1. `native/verify_rgb_eq.py --before-lib` の legacy 群 120/120。
     - 使ったのは、**自分で 5dabc1f から作り直した** dylib (作業者の `g1cpp_before` は使っていない)。
     - 新旧の dylib が別のイメージとして読み込まれていることを確かめた: ハンドルが違い、`nvc_aft_combine` のアドレスが違い、旧 dylib には `nvc_rgb_weights` がない。
  2. **私が作った、C++ ではない基準** (`independent3/oracle_edit.py`)。
     - `nv_aft_v5.apply_aft` (Python、変更していない) のデータの入口 2 つだけを `mock.patch` で差し替えた。`nv_profile.build_profile` には C の生プロファイルを返させ、`nv_aft.auto_fine_tune` には編集した float32 レベルを書き込んだ C の AFT バイト列を返させた。
     - これで apply_aft 自身の結合 (q99 → c_cj、補間、√、曲線との積、sigma) が基準になる。
     - 同じレベルを factors に持つ RGB 形式のプロファイルを、C++ UI (`from_json` → AppState → effective → フレーム処理) に通した。
     - 編集は作業者と違う 6 種 (両端 0.2・3.0、1 ビン 1.37、B の全ビン 0.55、ランダム 12 ビン、3 面のビン 5、混在) × test.mp4:600・test_v02:450 の 2 フレーム。
     - **12/12 で、Y/Cb/Cr の曲線・sigma・処理フレームがビット一致**した。
     - 照合は空振りしない: 編集のたびに曲線が AFT 後から変わり、フレームの最大差は 0.93〜4.77。基準の曲線を 1 ulp ずらすと検出した。

## 2. 既定の流れが変更前の C++ ビルドとバイト一致すること [観測]
- 変更前のビルドは、`git archive origin/main native qtui` を scratchpad に展開して作り直した (`g1v3/before`)。旧の `qtui_export` が旧の dylib を読んでいることは rpath で確かめた。
- 両方のツリーに同じドライバ `independent3/flow_default.cpp` をリンクした。
  - 変更前のツリーでは、旧 MainWindow の AFT の手順 (factors を 1、pins なし) をそのまま書いた。
  - 変更後のツリーでは `after_aft` を使う。
- 比べたフレームは、作業者が使っていない test.mp4:600・test_v02:450・test.mp4:150。

| 比較 | 結果 |
|---|---|
| default: 処理フレーム・パラメータ・Quality | 3 フレームとも一致 |
| default: 有効プロファイル | curve・sigma と既存のキーがすべて一致 (追加は `ui_rgb_weights`・`aft_levels` だけ) |
| default: 書き出し (CRF 0、4 フレーム、新の qtui_export vs 旧の qtui_export) | **バイト一致** (507,414 / 2,296,538 / 507,555 B) |
| AFT 後に tune だけ (all 1.1・Cb 0.9) | 処理フレーム・パラメータ・曲線・sigma・**書き出しがバイト一致** |
| AFT 後に tune だけ: Quality | 変わる (例 41,65 → 37,48)。承認済みの U4 (RGB では tune をレベル語に入れない) どおりで、tune の Quality は default と同じになった |
| 決定論 (test_v02:450 を 2 回) | 全 10 ファイルが一致 |

## 3. `verify_rgb_eq.py` の参照の演算順 [観測]
- `ref_combine` は `nv_aft_v5.apply_aft` の 4〜6 段と一行ずつ同じ。
  - `np.interp(v5 中心, linspace(0,256,9), float64 レベル)`
  - `sum(cw[ci][j]*faces[j])` (0 起点・左から)
  - `np.sqrt(np.maximum(ws, F32(0)))`
  - `(np.array(curve, F32) * f).astype(F32)`
- `ref_effective` は、その後に `× F32(tk)`、sigma は `float(np.interp(...) * tot)`。apply_aft の 7 段と同じ。
- `ref_weights` は `nv_aft_v5._color_weights` そのものを呼ぶ。
- Round 1 で読んだ旧 `backend.rgb_combine` と、`ref_combine` は同じ文面。
- apply_aft 群は、apply_aft (Python の AFT を含む) を直接呼ぶ。編集なしのレベルだけだが、本物の基準になる。
- 実行結果: **ALL SAME 9234/9234** (combine 5000・effective 4000・weights 112・apply_aft 2・legacy 120。legacy は新 dylib と、私が作った旧 dylib の両方)。

## 4. 互換性の判断 (旧形式を旧モードで読む) は妥当か [観測 + 判断]
- **妥当と判断する**。根拠は次のとおり。
  - 読み替えは厳密にできない。結合は面ごとに補間してから √ を取る非線形の式で、Y/Cb/Cr の 9 ビン係数から R/G/B レベルを一意に逆算できない。
  - C++ ファースト方針で `repro/` (PySide6 版) は変更しないので、Python 版は今後も旧形式を書き続ける。明示エラーにすると、それが C++ UI で読めなくなる。
- 実測:
  - worktree 内の保存プロファイル 220 件 (n3fix-g1 の保存物とフィクスチャ) を、旧 C++ と新 C++ の両方で `from_json` → 有効プロファイルに通した。
  - 旧 C++ が受理した旧形式 **71 件は、71/71 とも新 C++ で YCbCr として受理され、有効プロファイル JSON (長さとハッシュ) が一致**した。
  - 新形式の 90 件は新 C++ だけが受理する。59 件はどちらも拒否する (意図的な不正ファイルなど)。
- 仕様の文面は「読み替えか明示エラーのどちらか」で、旧モードはその第 3 案にあたる。REPORT に理由が書いてあるので、統括・ユーザーが追認すればよい (→ I-1)。

## 5. ABI の版数を 2 のままにした影響 [観測 + 判断]
- 既存の関数の引数と戻り値は変わっていない。`nvc_aft_combine` の挙動が変わらないことは、N1i の結合 69/69 と legacy 群 (旧 dylib と比較) で確かめた。
- 2 のままにするのは妥当で、**むしろ上げてはいけない**。
  - 版を確かめているのは `repro/nv_native.py:77` だけで、条件は `!= ABI_VERSION` (2) の完全一致。
  - 3 に上げると、変更できない `repro/` の native 経路が黙って Python 経路に落ちる [推測。コードからの読み]。
- qtui は版を確かめず、dylib を直接リンクしている。
  - 新しい qtui を古い dylib で動かすと、`nvc_rgb_*` の未定義シンボルで起動時に止まる。黙って違う結果になることはない [推測。dyld の通常の挙動]。
  - ctypes で使う側は `hasattr` で判別できる (verify_rgb_eq はそうしている)。
- 機能の有無を版数で判別できない点だけを、I-2 に残す。

## 6. L-1 (Round 1 の指摘) の修正が残っていること [観測]
- `qtui/src/core/state.cpp` は Round 2 で確かめた版と一字一句同じ。
- `qtui_selftest::g1MalformedKeys` は PASS。
- Round 2 の不正ファイル 35 件を新 C++ に読ませた (`loadcheck`)。受理・拒否と文言が、**Round 2 の C++ の結果と 35/35 一致**した (拒否は 25 件)。

## 7. Round 1・2 の性質のうち、C++ 側で今も成り立つもの [観測]

| 性質 | 結果 |
|---|---|
| C ABI の境界 (NULL・n ≤ 0・面積 0・枠の外で −2) と、端の値 (NaN・inf・負・巨大・非正規数) のビット一致 | **7814/7814**。参照は Round 1 の Python UI ではなく `verify_rgb_eq` の `ref_*`。1 ulp ずらすと DIFF を検出 |
| 既定の出力が変更前と一致 | §2 (3 フレーム、書き出しも) |
| tune だけの経路が変更前と一致 | §2 |
| 旧形式のファイルが変更前と同じ値で読める | §4 (71/71) |
| 不正ファイルの拒否 | §6 (35/35) |
| 両 UI の一致 | C++ ファースト方針で義務は廃止されたので、対象外 |

## 8. 再実行した共通の検証 [観測]

| 検証 | 結果 |
|---|---|
| `qtui_selftest` (NV_TEST_VIDEO=test.mp4) | **115 passed, 0 failed** (g1 の 5 関数を含む) |
| native の照合 11 本 (`make all` の後に 1 本ずつフォアグラウンドで) | すべて一致。quality 4000/4000・band 5706/5706・curve ALL・lzma 667/667・flat ALL・flat_cancel ALL IDENTICAL・model 1244/1244・temporal 474/474・spatial 1287/1287・profile 39/39・aft ALL (**combine 69/69**) |
| `verify_bridge --only aft,prof5,fallback` | ALL MATCH (aft 26/26・999,732 語、prof5 110/110、fallback 9/9) |
| `verify_rgb_eq.py --before-lib <自作の 5dabc1f の dylib>` | ALL SAME 9234/9234 |

Python の自己試験 (97/97) は、`repro/` が変わっていないので再実行していない。

---

## 指摘

### L-1: `g1EditMatchesAftCombine` は、それだけでは「変更前の Y/Cb/Cr 結合と同じ」の証明になっていない
- 状況:
  - 試験のコメントと REPORT は、比べる相手を「the Y / Cb / Cr combination of the code before G1 (nvc_aft_combine)」としている。
  - しかし実際に呼ぶのは、G1 で分けた後の今のツリーの `nvc_aft_combine` で、計算の芯 (`nva::combine_levels`・`nva::color_weights`) は UI の経路と同じ関数である。
  - 計算に誤りが入っても、2 本の経路に同じように入るので、この試験は検出できない。
- 再現: `native/nv_aft.cpp:687` (`combine` の中) と `:711` (`rgb_effective` の中) が、どちらも `combine_levels` を呼ぶ。
- 影響:
  - 手順 4 の「C++ 内部で証明」の主張が強すぎる。
  - 変更前との一致そのものは、verify_rgb_eq の legacy 群 (旧 dylib 120/120) と私の基準 (apply_aft 12/12、フレームも一致) で成り立っているので、出力の誤りではない。
- 推奨 (どちらか):
  - (a) 5dabc1f のビルドで、同じ入力から `nvc_aft_combine` の出力 (4 通りの編集の Y/Cb/Cr 曲線と sigma) を作り、固定のフィクスチャとして `qtui/tests/fixtures` に置く。自己試験はそれと比べる。
  - (b) 試験のコメントと REPORT を「分けた後の結合 (計算の芯は共通) と UI の周りの一致。変更前との一致は verify_rgb_eq --before-lib」と書き直す。

### I-1: 互換性の扱いが仕様の文面 (読み替えか明示エラー) にない第 3 案
- 理由と実測 (§4) から妥当と判断する。仕様からの逸脱にあたるので、統括・ユーザーが追認してほしい。

### I-2: ABI の版数で G1 の関数の有無を判別できない
- 2 のままにするのが正しい (§5。上げると `repro/nv_native.py` が native を使わなくなる)。
- 判別は `dlsym`・`hasattr` に頼ることになる。MIGRATION.md に一行残すとよい。

### I-3: 設定ファイルを共有している PySide6 版 (変更しない) は、C++ UI が書いた新形式のプロファイルを拒否する
- 両 UI は QSettings (NeatRepro / nv_ui_qt) を共有している (`qtui/src/app/mainwindow.cpp:157`)。
- C++ UI で Default Profile を割り当てると、PySide6 版を開いたときに「default profile ignored: not a repro profile file」と出て無視される。
  - [観測] PySide6 版の `ProfileState.from_json` は C++ の RGB ファイルを拒否する。
  - [推測] Default Profile の経路でこの文言が出ること。
- 黙って値を変えることはない。PySide6 版は参照用なので I とする。

### I-4: 「+20%」の照合は float32 の 1.2 で行っている
- AFT のバイト列は float32 しか持てないので、変更前の結合と比べられるのは float32 のレベルだけである。
- エディタが実際に作る double の 1.2 (小数 3 桁) は、`verify_rgb_eq` の combine 群・effective 群 (小数 3 桁を含む) で `ref_*` と比べている。作業者の報告どおりで、記録だけ。

---

検証用のスクリプトは `sandbox/omo/coord_g1/independent3/` にある (`flow_default.cpp`・`loadcheck_any.cpp`・`oracle_edit.py`・`abi_edges_r3.py`・`CMakeLists.txt`)。`sandbox/omo/coord_g1/independent/` には Round 1・2 の `indep_eq.cpp`・`loadcheck.cpp` の写しを置いた。
出力は scratchpad の `g1v3/` にある (`before/`・`flow/`・`orc_*`・`lc_*.txt`・各ログ)。ソースの編集と git の操作はしていない。

---

# Round 4 (Round 3 の L-1 の修正の確認、2026-10-31)

対象: 未ステージの `qtui/tests/selftest.cpp` の差分と、未追跡の `qtui/tests/fixtures/g1_before_combine.json` (5,554 B) と `qtui/tools/g1_fixture/` (`g1_fixture.cpp`・`make_g1_fixture.sh`)。

## 判定: **APPROVE** (Round 3 の L-1 は解消)

## 1. フィクスチャの再生成 [観測]
- `make_g1_fixture.sh` は出力先が worktree の `qtui/tests/fixtures/` に固定されていて、実行するとソース (置かれているフィクスチャ) を上書きする。そのため、スクリプトの手順を scratchpad で手で再現した。
  - 使ったのは**自分で作った 5dabc1f のツリー** (Round 3 で `git archive origin/main` から作ったもの。`origin/main` = 5dabc1f326d6… であることを確認)。
  - その写し (`g1v3/fixgen`) に `g1_fixture.cpp` を足し、スクリプトと同じ 3 行の CMake ターゲットを足して、`make lib` と `cmake` でビルドした (警告 0)。
  - `g1_fixture test.mp4 4 <scratch>` を実行した。
- 結果: **置かれているフィクスチャとバイト一致** (sha256 `52afccad5c5d1931…3abc5752` が両者で同じ)。
- 実行中に読み込まれた dylib は、`DYLD_PRINT_LIBRARIES` で調べると 5dabc1f の写しの `libnvcore.dylib` だけだった。その dylib には `nvc_rgb_*` のシンボルがない (`nm` で 0 件)。

## 2. 生成側のツールが 5dabc1f の関数だけを使っているか [観測]
- `g1_fixture.cpp` が include するのは `nvcore.h`・`nvcore_c.h`・`pyobj.h`・`state.h`・`video.h` の 5 つで、どれも 5dabc1f にある。
- 呼ぶ関数は次のとおりで、5dabc1f のツリーで、そのツリーの `libnvui.a` と `libnvcore.dylib` だけにリンクしてビルドが通った。G1 の関数を使っていれば、このビルドは通らない。
  - probe・matrix_from_tags・read_frames・build_profile・run_aft・to_planes・to_rgb・reference_floor・native_threads・backend_params・default_params・temporal・finish
  - nvc_profile_build・nvc_aft・nvc_aft_combine
- 今のコードを経由していない。

## 3. 試験側が、テスト対象のコードを参照値にしていないか [観測]
- 新しい `g1EditMatchesAftCombine` は、期待値 (曲線・sigma・処理フレームの FNV-1a) を**フィクスチャから読む**。`nvc_aft_combine`・`nva::combine_levels`・`nvc_profile_build`・`nvc_aft` は呼ばない。
- 今のコードで作るのは入力 (build_profile + run_aft) だけ。その入力も、フィクスチャの `aft_levels` と `curve_after_aft` (5dabc1f の値) と完全に一致することを先に確かめる。
- 期待値そのものも、C++ ではない基準で確かめた。
  - `independent3/fixture_vs_oracle.py` で、Round 3 の apply_aft の基準 (`nv_aft_v5.apply_aft` を変更せず、データの入口 2 つだけを差し替え) を、フィクスチャの 4 つの編集に掛けた。
  - **曲線・sigma が 4/4 でビット一致**した (test.mp4 フレーム 4。AFT のレベルがフィクスチャと一致することも確認)。

## 4. 空振りしないこと (変異試験)
worktree を scratchpad に写し (`g1v3/mut`、NV_SOURCE_ROOT はその写し)、`qtui_selftest g1EditMatchesAftCombine` だけを動かした。変異を入れる前は PASS。

| 変異 | 結果 |
|---|---|
| フィクスチャ: 編集 3 の Cb 曲線 1 値を 1 ulp ずらす | **FAIL** (All bin 2 +20%) |
| フィクスチャ: 編集 1 のフレームのハッシュ 1 桁を変える | **FAIL** |
| フィクスチャ: 編集 4 の sigma Cr を 1 ulp ずらす | **FAIL** (B bin 6 −30%) |
| コード、UI 経路だけ: `nvc_rgb_effective_curve` でレベルを ×(1+1e-6) | **FAIL** |
| コード、UI 経路だけ: c_cj の Cb 行と Cr 行を入れ替える | **FAIL** |
| コード、**共有部** `combine_levels`: sqrt を float の sqrt にする | **FAIL** (入力の `curve_after_aft` の照合で) |
| コード、**共有部** `combine_levels`: 節点の間隔を 32 → 31.99 にする | **FAIL** (同上) |
| 比較: **Round 3 の版の試験** (`git show :qtui/tests/selftest.cpp`) に同じ float sqrt の変異を入れる | **PASS** (検出できない)。Round 3 の L-1 で指摘した循環が、今回の修正で解消したことの直接の証拠 |
| 参考: 共有部 `combine_levels` で 3 項の和の順を逆にする | PASS |
| 参考: UI 経路でレベルを ×(1+4e-16) | PASS |

最後の 2 件は、float32 に丸めた後の出力が変わらない、ほぼ等価な変異である。和の順を逆にする変異は、`verify_rgb_eq --quick` の 932 件でも一致したので、観測できる出力が変わらない変異と判断した。試験の弱さではない。

すべての変異を元に戻し、ソースが元どおりであること (cmp) と PASS を確かめた。worktree 本体には触れていない。

## 5. `qtui_selftest` の全件 [観測]
- worktree で `make -C native lib` と `cmake --build qtui/build_main` を実行した (警告 0)。
- 結果: **115 passed, 0 failed** (g1 の 5 関数を含む。NV_TEST_VIDEO=test.mp4)。

## Round 4 の追加の指摘 (I のみ)

### I-5: フィクスチャと生成ツールが未追跡
- `qtui/tests/fixtures/g1_before_combine.json` と `qtui/tools/g1_fixture/` は `??` (未追跡) で、`selftest.cpp` の変更も未ステージ。
- このままステージに入れずにマージすると、試験は「fixture is missing」で落ちる。マージ時に 3 つとも含めること。

### I-6: `g1EditMatchesAftCombine` は test.mp4 のフレーム 4 に固定されている
- `NV_TEST_VIDEO` に別の動画を渡すと、AFT のレベルの照合で FAIL する (文言で原因は分かる)。
- 別の素材で自己試験を回す運用があるなら、SKIP にするか、その旨を文書に書くとよい。

### I-7: `make_g1_fixture.sh` は worktree のフィクスチャを直接上書きする
- 加えて、既定の作業場所 (`$TMPDIR/g1_fixture_build`) を `rm -rf` する。
- 再生成の手順としては正しい。ただし「置かれているものと一致するか確かめたい」ときは、出力先を引数で指定できると安全である。

---

# Round 5 (NVC_ABI_VERSION 2→3 と merge の確認、2026-10-31、軽め)

対象: `n3fix-g1cpp` の 2 コミット。4b5e510 (origin/main c6d497b の merge) と 53a52e1 (版数 2→3、統括のコミット)。

## 判定: **APPROVE**

## 確かめたこと [観測]
- **merge 4b5e510**
  - 枝に入ったのは `.claude/tasks/native_n3_fixes2.md` (+23 行) と `sandbox/omo/readonly_snapshot_before.sha256` (1 行) だけで、製品コードは変わっていない。
  - 8cb3ba3 の `qtui/`・`native/` のファイル 183 件を、Round 4 で検証した写し (`g1v3/mut`) と比べ、183/183 が一致した。
- **53a52e1 の差分**: 次の 2 行だけ。
  - `native/nvcore_c.h` の `#define NVC_ABI_VERSION 3` (コメントに G1 を追記)
  - `repro/nv_native.py` の `ABI_VERSION = 3`
- **版数を参照している箇所** (`native/`・`repro/`・`qtui/`・`packaging/`、ビルド出力の `__GXX_ABI_VERSION` を除く)
  - 定義: `nvcore_c.h:28`・`nvcore_c.cpp:79` (定数を返すだけ)
  - 比較: `repro/nv_native.py:77` の `!= ABI_VERSION` だけ
  - `native/x86_bridge_cap.py:172` は呼び出しを記録するだけで、`x86_bridge_replay.py:182-184` は fallback 群以外では数えない。どちらも値 2 を前提にしていない。記録ファイルは git に入っておらず、毎回取り直す。
  - qtui と packaging は版数を参照していない (packaging は `nv_native.AVAILABLE`/`LIB_PATH` を診断に出すだけ)。
  - **漏れはない**。
- **版数 3 の dylib を実際に読む**
  - HEAD で `make -C native lib` (警告 0) をした後、`nv_native.LIB_PATH` = `<wt>/native/libnvcore.dylib`、`AVAILABLE` = True、`nvc_abi_version()` = 3 だった。
- **版数 2 の古い dylib を置いたとき** (記録)
  - 使ったのは、Round 3 で自分で作った 5dabc1f の dylib (`nvc_abi_version()` = 2)。
  - Python 側: `NV_NATIVE_LIB` にそれを指定すると、`LIB_PATH` = None・`AVAILABLE` = False になる。`backend.native_enabled()` は NV_NATIVE=1 でも False。**警告もメッセージも出さず、黙って Python の経路に落ちる** (stderr は空)。
    - 結果は Python の経路とビット一致なので、出力は変わらず遅くなるだけ。
    - 気づけるのは packaging の診断 JSON (`native_available`) くらい。既存の設計どおりの挙動で、G1 で変わった点ではない。
  - C++ 側 (参考): 同じ dylib を `DYLD_LIBRARY_PATH` で読ませると、`qtui_export` は「Symbol not found: _nvc_rgb_effective_curve」で起動時に止まる。黙って違う結果になることはない。
- **`python3 native/verify_bridge.py --only fallback,cancel`**: **ALL MATCH** (fallback 9/9・691,209 語、cancel 49/49・199,807 語、付随の aft 2/2)。

## 指摘 (I のみ)

### I-8: 文書が版数 2 のまま
- `native/MIGRATION.md:235` と `AGENTS.md:595` は、版数 2 (G7/G8 時点) の記録のままで、G1 の 3 への更新が書かれていない。履歴としては正しいので、G1 の節を足すときに「3 (nvc_rgb_*)」と一行添えればよい。
- Round 3 の I-2「版数を上げてはいけない」は、`repro/nv_native.py` を同時に上げる前提で解消した。C++ ファースト方針のもとでも、版数の追従だけは `repro/` の変更として許容した、という扱いになる。統括の判断として記録しておくこと。
