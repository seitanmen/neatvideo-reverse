# G1 独立検証 判定 (verify-g1、2026-10-30)

対象: worktree `n3fix-g1` (基点 df46dda) の未コミット変更 (`.omc/` を除く 20 ファイル変更 + 新規 11 件)。
仕様: `.claude/tasks/native_n3_fixes2.md` の G1 と共通規約。設計: `sandbox/omo/wp_qtui_fix/DESIGN_G1.md` (U1〜U10 すべて ★ 案で承認済み)。
記号: **[観測]** = 自分で実行・読んで確かめたこと、**[推測]** = 確かめていないこと。

## 判定: **APPROVE**

重大度 M の指摘はない。L が 1 件ある (形の崩れたファイルを読むと C++ UI が落ちる)。マージは止めないが、直すことを勧める。I は 5 件。

---

## 1. 差分の読み (仕様・承認済み ★ 案との照合) [観測]

| 観点 | 判定 | 根拠 |
|---|---|---|
| 結合式の演算順が apply_aft と同じか | 同じ | `backend.rgb_combine` は `nv_aft_v5.apply_aft` の 4〜6 段と一行ずつ同じ。np.interp(v5 中心, linspace(0,256,9), float64 レベル)、`sum(cw[ci][j]*faces[j])` (0 起点・左から)、`np.sqrt(np.maximum(ws, F32(0)))`、`(f32 曲線 × float64).astype(F32)`。c_cj は `nv_aft_v5._color_weights` をそのまま呼ぶ。C++ の `nva::combine` は `color_weights` と `combine_levels` に分けただけで、演算は変わっていない (N1i の結合 69/69 は分けた後も一致) |
| tune の掛け方 | 設計 §2.3 どおり | 結合の後に f32 × np.float32(tk) を掛ける。C は `M(curve, float(tk))`。tk = all·c を double で計算し、成分は Y, Cb, Cr の順 (両 UI と ABI で同じ) |
| 変更なしなら base をそのまま返すか | 両 UI で同じ | 「L == rgb_reference かつ tk がすべて 1」なら base を返す。「AFT 後に L = 1 かつ tk = 1」(Reset) なら AFT 前の curve と sigma を返す (U5) |
| AFT のレベルの取り出し | 設計 §2.6 どおり | Python は 9 ビン中心の代理プロファイルで AFT を 1 回呼ぶ。C++ はバイト列 `0xbf44+0x120j+0x20k` から読む。base のキー順 (…, aft_bin_flags, aft_levels, ui_pre_aft) は両 UI で同じ |
| 旧形式 (Y/Cr/Cb) の互換 | U9 どおり | `nv_repro_profile` は channels "YCbCr" として読み、従来の `effective_profile` を通す。AFT・ビルド・Reset (rgb_ready のとき) で RGB に移る |
| 新形式のフラグ | U8 どおり | `format: nv_repro_profile_rgb, version: 2, channels: "RGB"`。channels がない場合と色の重みがない場合は拒否する。キー順は両 UI で同じ。変更前の UI はこの形式を拒否する (下の §2.6) |
| Quality | U3・U4・U10 どおり | RGB では、手で編集したビン (pinned) にだけ L を書き、ほかは 1.0。tune は入れない (Python `quality.level_words`、C++ `nvcore.cpp profile_numbers`)。旧モードは並びの食い違いも含めて従来のまま |
| Auto Complete | U6 どおり | 固定点 = pinned ∪ AFT の測定ビン (marker が偽)。ただし RGB・AFT 後で、aft_reset がないときだけ |
| C ABI の 3 関数 | 境界は扱えている | NULL・n ≤ 0・面積 0・枠の外は −2 を返し、例外は ABI の外に出ない。NaN・inf は numpy と同じビットで通る (§2.2) |
| 処理コアの変更 | なし | `nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`・`nv_native.py` は差分なし。書き出しは新しいキー (`ui_rgb_weights`・`aft_levels`) を読まない (grep で確認) |
| 新しいコードの警告 | なし | `nvcore_c_rgb.cpp`・`nv_aft.cpp` を -Wall -Wextra でコンパイルして警告 0 |

## 2. 再実行した検証 (すべて自分で実行) [観測]

ログと出力は scratchpad `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/g1v/` に置いた。スクリプトは `sandbox/omo/coord_g1/independent/` にある。検証の前に `native/libnvcore.dylib` と `qtui/build_main` を作り直した。

### 2.1 作業者と同じ検証
| 検証 | 結果 |
|---|---|
| Python 自己試験 NV_NATIVE=1 / 0 | **108/108 / 108/108** (G1 の 12 項目は ON/OFF とも PASS。run_aft の項目は native True / False をそれぞれ確認) |
| `qtui_selftest` (NV_TEST_VIDEO=test.mp4) | **108 passed, 0 failed** (g1FreshAndAft・g1EditsResetComplete・g1QualityAndFiles を含む) |
| `make -C native verify` (11 本) | **すべて一致**。quality 4000・band 5706・curve ALL・lzma 667/667 (genuine blob はこの worktree になく skip)・flat 362/362・flat_cancel・model・temporal 474・spatial 1287・profile 39/39・aft (shuffle 4097・sim 3000・finish 3000・aft 67×2・**combine 69/69**) |
| `native/verify_rgb_eq.py` | **ALL SAME 8973/8973** (combine 5000・effective 3853・weights 112・aft 8)。作業者の報告は 8983 で件数が違う。saved_bases が worktree 内の JSON を glob で拾うので、件数はファイル構成で変わる [推測] |
| `verify_bridge.py --only prof5,aft,ui,frame` | **ALL MATCH**。aft 26/26 (999,948 語)・frame 72/72 (306,334,426 語)・prof5 110/110・ui 34,755/34,755 |
| `qtui/tools/g1_compare.py`。作業者が使っていないフレーム **test.mp4:600・test_v02.mp4:450** で 7 ケース | **ALL SAME**。状態 JSON・有効プロファイル・パラメータ・Quality・処理フレームが両 UI で一致。書き出しは py on / py off / C++ の 3 つがバイト一致 (6 ケース × 2 クリップ) |

### 2.2 C ABI の境界と端の値 (`independent/abi_edges.py`、自作)
- 次の 14 通りがすべて −2 を返す: NULL (curve・out・tune・sigma・rgb・area)、n = 0、n = −1、H = 0、面積の h = 0 か w = 0、領域が枠の外、大きさが負。
- 結合 4000 件で、Python `backend.rgb_combine` と `nvc_rgb_combine` のビットが一致した。入力は次のとおり。
  - レベル: NaN・±inf・−1・−0・0・非正規数・1e308・DBL_MAX・50・−3 を混ぜた。
  - c_cj: q99 に 0・NaN・inf・1e-40・3e38 を入れたもの、重み自体に端の値を入れたもの。
  - 曲線: NaN・inf・3.4e38・1e-45 を混ぜた。
  - 中心: 9 節点、1 点、範囲外 (−10・300)。
- 節点 500 件: 中心 = 9 ビンのとき、C の face がレベルとビット一致した。C++ UI がレベルをバイト列から読む経路の前提を確かめた。
- 有効曲線 3000 件: `state.rgb_effective_profile` と `nvc_rgb_effective_curve` で、curve と sigma のビットが一致した。tune は 0・−1・NaN・inf・1e±30、area_level は −5・0・300・NaN を入れた。
- 重み 300 件: `backend.rgb_weights` と `nvc_rgb_weights` が一致した。入力は乱数の大きさの面、枠の端で切れる領域、1×1 などの小さい領域、一定・0・255 の面、3 種の行列、full / limited range。
- 合計 **7814/7814**。わざと C の結果を 1 ulp ずらすと (`--mutate`)、その 1 件が DIFF になった (照合は空振りしない)。

### 2.3 作業者が使っていない編集で両 UI のビット一致 (`independent/gen_cases.py`・`run_cases.py`・`indep_eq.{py,cpp}`)
上の g1_compare で作った default (AFT 後) を出発点に、16 通りの ProfileState JSON を作った。それを Python (NV_NATIVE=1・0) と C++ (`indep_eq.cpp`、libnvui.a にリンク) で `from_json` → `AppState` → effective → process_frame → Quality と通し、test.mp4:600 と test_v02:450 で比べた。

| ケース | 内容 |
|---|---|
| m1 | R のビン 0 = 0.2、ビン 8 = 3.0、G のビン 4 = 0.2 (範囲の両端) |
| m2 | 27 ビンすべてを乱数 (小数 3 桁) + tune all 0.5・Y 1.7・Cr 1.3・Cb 2.9 |
| m3 | B の全ビンを −80% + tune Cb 0.6 (負の編集) |
| m4 | ファイルから範囲外のレベル 0・−2・50・1e-9・3.5・0.1 (曲線が 0 になるビンがある) |
| m5 / m6 | AFT 前の RGB プロファイルの編集 + tune / tune だけ |
| m7 | AFT 後に tune だけ (all 1.1・Cb 0.9) |
| m8 / m8b | Reset + tune 1.3 / Reset だけ |
| m12 | aft_levels のない AFT 済みの base (G1 より前の AFT) を RGB で編集 |
| m13 | 3 面のビン 0 とビン 8 を 0.6 + tune Y 3.0・Cr 0.2 |
| l1 / l2 | 旧形式 (AFT 後 / AFT 前) の Y・Cr の編集 + tune |
| x1〜x3 | 拒否されるべきもの (channels なし、重みなし、NaN のレベル) |

結果:
- 2 クリップとも、16 ケースすべてで、状態 JSON の往復・有効プロファイル・パラメータ・処理フレーム・Quality が py1 / py0 / C++ でバイト一致した。
- x1〜x3 は、3 つとも同じ文言で拒否した。
- 書き出し (CRF 0・4 フレーム) は py on / off / C++ でバイト一致した。test.mp4 は 6 ケース、test_v02 は 4 ケース。
- 照合は空振りしない。
  - 編集したケースの曲線は default と 6〜24/24 値で違い、処理フレームの最大差は 0.72〜59.9 だった。
  - C++ のフレームを 1 バイト書き換えると、DIFF になった (`--mutate`)。

### 2.4 編集なしの既定出力が変更前とバイト一致すること (変更前のコード = `git archive df46dda repro` を scratchpad に展開)
変更前の UI の既定の流れ (ビルド → AFT → factors を 1) を `independent/before_default.py` で、test.mp4:600 と test_v02:450 について動かし、変更後と比べた。

| 比較 | 結果 |
|---|---|
| 処理フレーム (変更前 vs 変更後 py・C++) | 一致 |
| 有効プロファイルの curve・sigma | 一致 (キーは `ui_rgb_weights`・`aft_levels` が増えただけ) |
| パラメータ JSON | 一致 |
| 書き出し (変更前の export vs 変更後の py on・py off・C++) | **バイト一致** (507,414 B / 2,296,538 B) |
| AFT 後に tune だけ: 変更前 (tune7) vs 変更後 m7 (py1・py0・C++) | 処理フレーム・curve・sigma が一致 |
| 旧形式 l1・l2 を変更前のコードで読む vs 変更後の 3 つ | eff.json・planes・Quality・params がすべて一致 |
| 変更前のコードで新形式 (m2) を読む | "not a repro profile file" で拒否する (U8 どおり) |

---

## 3. 指摘

### L-1: 新しいキーの形と型を `from_json` が確かめない。形の崩れたファイルで、C++ UI が SIGSEGV で落ちるか、Python と黙って違う値になる
- 状況:
  - 両 UI の `from_json` (`repro/nv_ui_qt/state.py`・`qtui/src/core/state.cpp`) が RGB 形式で確かめるのは `rgb_ready` (キーがあるか) だけ。
  - `ui_rgb_weights`・`aft_color_weights`・`aft_levels` が 3×3 / 3×9 の有限の数か、は見ない。
- 再現 (scratchpad `cases_bad/`。test.mp4:600 の m5 / m1 を書き換えたもの):

  | ファイル | Python | C++ |
  |---|---|---|
  | `x4_shape.json` (`ui_rgb_weights = [[0.5, 0.5]]`) | effective で IndexError | `indep_eq` が **rc 139 (SIGSEGV)** |
  | `x7_strw.json` (重みの 1 つが文字列 "x") | effective で TypeError | 例外を出さず、ある値で計算を続ける (Cb 曲線 1.0415…) |
  | `x6_nanw.json` (重みに NaN) | 計算する | 計算する。両 UI の出力は一致する |

  ```
  QT_QPA_PLATFORM=offscreen <scratch>/build/indep_eq test.mp4 600 <scratch>/cases_bad/x4_shape.json /tmp/x   # rc 139
  QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 sandbox/omo/coord_g1/independent/indep_eq.py test.mp4 600 <scratch>/cases_bad/x4_shape.json /tmp/y
  ```
- 影響:
  - UI が書くファイルでは起きない。手で編集したか壊れたファイルを読んだときだけ起きる。
  - 既存のキー (`aft_bin_flags` など) にも同じ種類の未検査があるとみられる [推測]。G1 はその範囲を広げた。
  - F2 の方針「未到達の経路は明示的なエラー」と、「両 UI は同じ出力」の規約に照らすと、直すのが望ましい。
- 推奨: 両 UI の `from_json` で次を確かめ、満たさなければ ValueError / runtime_error で拒否する。selftest に 1 項目足す。
  - `ui_rgb_weights` と `aft_color_weights` が 3×3 の有限の数。
  - `aft_levels` が 3×9 の有限の数。

### I-1: `verify_rgb_eq.py` の Reset の件数 48 は Python 同士の比較
- 「AFT 後に L = 1」の 48 件は、`state.rgb_effective_profile` の結果を `ui_pre_aft` と比べているだけで、C 側と突き合わせていない。
- C++ の Reset 経路は ABI ではなく `state.cpp` にある。両 UI の一致は g1_compare の reset と私の m8b・m8 で確かめたので、実害はない。
- 報告の 8983/8983 のうちこの分は、UI 間の照合ではない。

### I-2: NVC_ABI_VERSION を上げていない
- 作業者の報告どおり、関数を 3 つ足しただけで、既存の ABI は変えていない。
- G7・G8 で版を 2 に上げるので、マージ時に揃える必要がある (統括の作業)。

### I-3: 設計 §4 の旧形式の表示 "legacy Y/Cr/Cb profile" はツールチップだけ
- 旧モードであることは、トグルの文言 (All / Y / Cr / Cb) とツールチップ「a profile made before the R/G/B equalizer」で分かる。
- 設計にある明示のラベルはない。機能上の差ではない。

### I-4: `nvc_rgb_weights` は負の開始位置を 0 に切り詰める
- numpy のスライスなら負の開始位置は末尾から数える。
- 呼ぶのはビルド時だけで、領域は FindFlatArea か手動選択の非負の値なので、到達しない。ヘッダの「clamped to the frame」の注記どおり。

### I-5: `sandbox/omo/wp_qtui_fix/g1/` が約 1.1 GB
- 作業者の報告どおり。消すかどうかの判断が要る。

---

## 4. 結論
- 結合式は apply_aft と同じ演算順で、Python と C ABI が端の値も含めてビット一致した。
- 両 UI の処理フレームと書き出しは、作業者の 7 ケースに加えて、私が作った 16 ケース × 2 フレーム (作業者が使っていないフレーム) でバイト一致した。
- 編集なしの既定出力と、tune だけ・旧形式の出力は、変更前のコードとバイト一致した。
- 共通規約の 4 つの検証 (自己試験 ×2・make verify 11 本・verify_bridge・書き出し) もすべて通った。
- 判定は **APPROVE**。L-1 (形の崩れたファイルの検査) は、マージの前か直後に両 UI 同時に直すことを勧める。
