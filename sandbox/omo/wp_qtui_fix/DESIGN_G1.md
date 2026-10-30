# G1 設計メモ: イコライザの R/G/B 化 (案 (a))

作成: 2026-10-30、g1-design (worktree `n3fix-g1`、df46dda)。製品コードは変更していない。
検証スクリプトは `sandbox/omo/wp_qtui_fix/g1/` (`check_combine.py`・`check_q99.py`・`check_probe.py`)。
記号: **[観測]** = コードを読んだか実行して確かめたこと、**[推測]** = 確かめていないこと。

---

## 0. 要約

- R/G/B の 9 ビンの値 L_j[k] を、本家と同じく**ビンのレベルそのもの**として UI に持たせる (乗数ではない)。
  AFT は L を AFT のレベルで上書きする。手動の編集は L を直接書き換える。Reset で L = 1.0 に戻る。
- 有効曲線 = **AFT 前の v5 曲線** × √(max(Σ_j c_cj·interp(L_j), 0)) (apply_aft の 5〜6 段と同じ式)。
  その後に tune を掛ける (f32 で ×f32(tk)。いまの tune だけの経路と同じビットになる)。
- AFT のレベル 27 個は、apply_aft の出力から逆算できない。そこで「9 ビン中心を curve_centers にした代理のプロファイル」で
  apply_aft を **1 回だけ** 呼んで取り出す (Python UI)。C++ UI は AFT のバイト列から直接読む。
  そのあと実際の曲線を UI 側の結合式で作る。この経路は apply_aft とビット一致する [観測: 下の §5 の A]。
- AFT 前のプロファイルの c_cj は、ビルド時に `nv_band.model_levels(R/G/B の切り出し)[5]` から作る。
  これは AFT の BuildNoiseModel の q99 とビット一致する [観測]。かかる時間は 512² の 3 面で 0.06 秒 [観測]。
- 既定の出力は変わらない。編集がない (L が基準と同じで tune が 1) ときは base をそのまま返す (構造上の恒等)。
  加えて、L = 1 を結合式に通しても曲線はビット単位で変わらない (427,917/427,917 件) [観測]。
- ユーザーに決めてほしい点は §7 にある。Quality の写像、Reset の意味、ファイル形式など 10 件。

---

## 1. 現状

### 1.1 イコライザのデータの流れ (Python 版 `repro/nv_ui_qt/`) [観測]

1. **編集**: `widgets/curve_editor.py` の EqualizerPlot が knot をドラッグする。`move_knot` (`curve_editor.py:184`) は
   `[0.2, 3.0]` に clamp してから `edited(c, fs, i)` か `editedAll(fs, i)` を出す。値は `round(…, 3)`
   (`curve_editor.py:122`)。線は Y/Cr/Cb の 3 本で、トグルは All/Y/Cr/Cb。
2. **state**: `main_window.py:290-291` が `AppState.set_factors` / `set_factors_all` (`state.py:383-396`) に渡す。
   `ProfileState.factors` は 3×9 の乗数で、並びは `CHANNELS` = Y, Cb, Cr (`state.py:21`)。
   pinned (編集したビン) も持つ。編集は QUndoStack の 1 手になる。
3. **曲線倍率**: `state.effective_profile` (`state.py:181-202`)
   - factors がすべて 1 (|f−1| < 1e-12) で tune もすべて 1 なら、base をそのまま返す。
   - そうでなければ k = f32(np.interp(centers, CURVE_X, f_c)·tk_c) を求め、curve_c = f32(curve_c·k) とする
     (f32 どうしの積)。sigma を計算し直し、`ui_factors`・`ui_tune` を注記として付ける。
   - base は AFT を適用した後の dict。
4. **v5 処理**: プレビュー、Assist、NLC、書き出しのいずれも `state.effective_profile()` の dict を
   `backend.process_frame` / `nvfilter` に渡す。曲線は Y/Cb/Cr の絶対 σ (centers 16, 48, …, 240) [観測]。
5. **書き出し**: `.ui_profile.json` は有効プロファイル (曲線は掛け算済み) そのもの。
   `export.py:43-69` が `nv.build_profile_v5` をこれに差し替えて、`denoise_video_v5.main` を動かす。
   CLI の `--params` はフィルタのパラメータの JSON で、プロファイルは含まない (`denoise_video_v5.py:392`)。
6. **Quality/Completeness**: `quality._curve_blob` (`quality.py:140-213`) が、本家の曲線バイトを次のように作る。
   - レベル語 = F32(F32(factors[c][k])·F32(tk[c]))
   - AFT 後は `aft_bin_flags` (R/G/B 面順) の旗を書く。
   - このとき**チャンネルの並びが食い違っている**。factors は Y, Cb, Cr の順なのに、tk は ("Y", "Cr", "Cb") の順で取る
     (`quality.py:189-194`)。C++ の `native/nv_ui.cpp:522-527` も同じ並びを写している。
     両 UI の結果は一致するので UI 間のずれではないが、既存の不具合とみられる。
7. **AFT**: `backend._run_aft` (`backend.py:304-324`)
   - `ui_pre_aft` があれば、曲線を AFT 前に戻してから `nv_native.apply_aft` か `nv_aft_v5.apply_aft` を呼ぶ。
   - 結果に `ui_pre_aft` = 呼ぶ前の curve と sigma を付ける。
   - 終わると `main_window.py:1283-1293` が factors を 1.0 に、pinned を空に戻す。
   - したがっていまのイコライザは、**AFT 後も平らに見える** (AFT の効果は base の曲線に入っている)。
     本家は AFT 後に曲線が曲がって見える (`genuine_ui/OBSERVED.md:59`)。
8. **Reset Fine-Tuning** (`main_window.py:1325-1337`): factors を 1 にし、`base["aft_reset"] = True` を立てる。
   base の曲線は AFT 後のまま残る (いまの repro では、Reset しても出力から AFT の効果は消えない)。

C++ 版 (`qtui/src/`) も同じ構成である [観測]。
- `core/state.cpp:316-346` は `nvc_effective_curve` (`native/nv_prof5.cpp:686-711`) を使う。
- `core/nvcore.cpp:234-314` の run_aft は、`nvc_profile_build` → `nvc_aft` → `nvc_aft_combine` を自前で並べている。
- Quality は `core/nvcore.cpp:545-575` → `nvc_quality_numbers`。
- AFT 完了・Auto Complete・Reset はそれぞれ `app/mainwindow.cpp:1723`・`1749`・`1769-1771`、
  イコライザは `widgets/curve_editor.cpp`。

### 1.2 RGB → Y/Cb/Cr の結合の場所 [観測]

- Python: `repro/nv_aft_v5.py`
  - `_read_model_q99`: `:35-38`
  - `_color_weights`: `:41-55`。c_cj = a_j²·m_cj / Σ_j a_j²·m_cj。分母は Python の `sum()` (Neumaier 補償加算)。
  - 面曲線の補間: `:99-107`。np.interp(v5 の中心, linspace(0, 256, 9), レベル)。
  - 結合: `:111-115`。√(max(0 + Σ c·f, 0)) を float64 で計算する。
  - 曲線への積: `:118-120`。(f32 曲線 × float64) を f32 に丸める。
  - sigma: `:123-126`。旗: `:136-147`。
  - m_cj は BT.601 の係数の二乗で、ストリームの matrix 引数は使わない。
- C++: `native/nv_aft.cpp:625-697` (`nva::combine`、N1i で 69/69 一致)。C ABI は `native/nvcore_c.h:58-63`
  (`nvc_aft_combine`)。入力は raw と aft のバイト列で、レベルを外から渡す口はない。

---

## 2. 案 (a) の具体化

### 2.1 UI が持つ値

- `ProfileState.factors` の意味を、`channels == "RGB"` のときは **R, G, B の各 9 ビンのレベル L_j[k]** に変える。
  - 乗数ではなく、本家の曲線語 0 (レベル) に当たる値。
  - 範囲は [0.2, 3.0] のまま。AFT の後処理 `FUN_009bdf30` も同じ範囲に切り詰める。
- 互換のため、`channels == "YCbCr"` (旧形式) では従来どおり Y/Cb/Cr の乗数とする (§4)。
- base に足すキー (どちらも R/G/B 面順):
  - `aft_levels` (3×9): AFT が書いたレベル。float(f32) をそのまま入れる。
  - `ui_rgb_weights` (3×3): ビルド時の c_cj。AFT 前のプロファイルで使う。
  - 位置は、AFT の結果では `aft_bin_flags` の直後 (`ui_pre_aft` の前)。ビルドの結果では末尾。
    両 UI で同じ順にする (PyObj の比較とバイト一致のため)。

### 2.2 結合の入力

| 入力 | AFT 前 | AFT 後 |
|---|---|---|
| 元の曲線 src_c | `base["curve"]` | `base["ui_pre_aft"]["curve"]` (= apply_aft に渡した曲線) |
| c_cj | `base["ui_rgb_weights"]` | `base["aft_color_weights"]` |
| 基準のレベル ref | 1.0 × 27 | `base["aft_levels"]` |
| a_j | `nv_band.model_levels(R/G/B 切り出し)[5]` | BuildNoiseModel の面 j の語 5 |

- c_cj は `nv_aft_v5._color_weights` で計算する (読むだけで、変更しない)。
- a_j の 2 つの求め方はビット一致する [観測: §5 の B と `check_q99.py` 10/10]。
- 切り出しには `_run_aft` と同じ RGB を使う: `np.clip(nv.yuv_to_rgb(frames[ui_frame], matrix, full_range), 0, 255)` の
  `prof["area"]` の範囲。

### 2.3 有効曲線の式 (Python を正とする)

```
if base is None: return None
tk_c = tune.all * tune[c]                       (c = Y, Cb, Cr。いまと同じ)
ref  = aft_levels if aft_applied else unit
if L == ref (厳密に等しい) and all tk_c == 1:
    return base                                  ← 既定の出力は構造的に不変
src = ui_pre_aft.curve if aft_applied else base.curve
face_j = np.interp(np.array(centers), np.linspace(0, 256, 9), np.array(L_j))   (float64)
for c in (Y, Cb, Cr):
    ws   = sum(cw[c][j] * face_j for j in 0..2)          # 0 + ... の左から順の加算 (apply_aft と同じ)
    f    = np.sqrt(np.maximum(ws, F32(0)))
    cur  = (np.array(src_c, F32) * f).astype(F32)        # apply_aft 6 段
    cur  = cur * F32(tk_c)                                # f32 × f32。いまの tune 経路と同じ丸め
sigma_c = float(np.interp(area_level, centers, cur_c) * tot_ratio_c)
注記: ui_channels = "RGB", ui_factors = L (R/G/B), ui_tune = tune
```

### 2.4 AFT と手動編集の合成順

本家と同じく**合成しない**。どちらもレベルを上書きする。

- **AFT**: L ← `aft_levels`、pinned ← 空。いまの「factors を 1 に戻す」に当たる。
  - 曲線は src (AFT 前) × 結合(L) なので、AFT を 2 回押しても 1 回と同じ (いまの性質を保つ)。
- **手動**: L_j[k] ← ドラッグの値。base は変えない。
  - AFT 後の 1 ビンを編集すると、AFT の他のビンは L に残る。
- **Reset Fine-Tuning**: L ← 1、pinned ← 空、`aft_reset` を立てる。
  - 曲線は src × 結合(1) = **AFT 前の曲線**になる。いまの repro は AFT 後の曲線のまま残すので、**挙動が変わる** (§7 の U5)。
- **Auto Complete**: 案は 2 つ (§7 の U6)。
  - いまの定義: pinned 以外を pinned から線形補間する。AFT 後に使うと、AFT のレベルを上書きする。
  - 別案: AFT の測定ビン (marker が偽) も「測定済み」として固定点に含める。
- **All トグル**: いまの set_factors_all と同じく、3 面の同じビンを同じ値にする (§7 の U7)。

### 2.5 tune (Overall・成分別 Noise Level) との関係

- tune は結合の**後**に Y/Cb/Cr の曲線へ掛ける (§2.3)。
  - L が基準のままのとき、結果はいまの tune だけの経路とビット一致する [観測: §5 の 4、1065/1065]。
  - したがって、tune を動かした既存の操作の数値は変わらない。
- 別案として「tune_all を R/G/B のレベルに掛ける」は採らない。√ の内側に入るので +20% が σ +9.5% になり、
  スライダーの意味が変わる。
- Quality の写像との関係は §7 の U4。

### 2.6 AFT の 27 レベルの取り出し (Python UI)

[観測] apply_aft は dict にレベルを残さない。残すのは v5 の 8 中心で補間した `aft_face_curves` で、
8 個の値から 9 ビンは逆算できない。処理コアは変更禁止なので、次のようにする。

- `backend._run_aft` は、代理のプロファイル
  `{"area": prof["area"], "curve_centers": [0, 32, …, 256], "curve": {Y/Cb/Cr: [1.0]*9}, "tot_ratio": …, "area_level": …}`
  で `nv_native.apply_aft` (失敗したら `nv_aft_v5.apply_aft`) を **1 回**呼ぶ。
- np.interp を節点で評価するとレベルがそのまま返るので、`aft_face_curves` = 27 レベルになる [観測]。
  `aft_color_weights`・`aft_bin_flags` は、実際の dict で呼んだときと同じになる [観測]。
- 次に、実際の dict を作る。
  - curve = §2.3 の結合 (tune なし) で計算する。
  - `aft_face_curves` = 8 中心での補間。
  - `aft_color_weights`、`aft_band` = 0、`aft_bin_flags`、`aft_levels` を入れる。
  - これは apply_aft の結果とビット一致する [観測: §5 の A]。
- C++ UI (`nvcore.cpp` の run_aft) は aft のバイト列から CURVE[j] + 0x20·k の f32 を読む。
  float に広げた値は、代理の呼び出しで得る値と同じ。
- 退けた案:
  - apply_aft を 2 回呼ぶ (AFT が 2 倍の時間になる)。
  - `nv_aft.auto_fine_tune` をモジュールの外から差し替えて捕まえる (行儀が悪い)。
  - `nv_native.apply_aft` にキーを足す (verify_bridge の AFT 26/26 で、dict の一致が崩れる)。

---

## 3. Python と C++ のビット一致

1. **共有の方法**: C++ UI は C ABI を使う。Python UI は numpy で同じ式を書く (8 点なので軽い)。
   照合スクリプトで Python と C ABI を突き合わせる (verify_bridge_prof5 の effective_profile と同じ方式)。
   - `native/nv_aft.cpp` の `nva::combine` を、次の 2 つに分ける。分けた後も combine の結果は不変で、
     N1i の結合 69/69 で確かめる。
     - `color_weights(q99[3], cw)`: Neumaier の分母を含む。
     - `combine_levels(cw, levels27, centers, n, src_curves, …)`
   - 新しい ABI は次の 2 つ。`nvc_abi_version` を上げる。
     - `nvc_rgb_weights(rgb, H, W, area4, band, double cw[9])`: nvb::model_levels → color_weights。
     - `nvc_rgb_effective_curve(centers, n, src[3n], cw[9], levels[27], tune[4], area_level, tot[3], curve_out, sigma_out)`
   - 恒等の判定 (L == ref かつ tk == 1) は state 層 (両 UI) に置く。ABI は常に計算する。
2. **float の演算順**: §2.3 の 1 行ごとに次を固定する。既存の `nva::combine` がすでにこの演算順で、照合済み。
   - face: float64 の Interp。np.interp と同じ。節点では fp をそのまま返す。
   - ws = AD(AD(AD(0, MD(cw0, f0)), MD(cw1, f1)), MD(cw2, f2))
   - max は NaN をそのまま通す。sqrt は double。f32(MD(double(src), f))。
   - tune: f32 の M(cur, f32(tk))。nv_prof5 の effective_curve と同じヘルパ。
   - sigma: MDS(Interp(...), tot)。
   - レベル L は double で持つ (JSON の repr で往復しても厳密)。
3. **NV_NATIVE=0**: Python UI の結合は numpy だけで、native を使わない。
   - c_cj は常に Python の `nv_band.model_levels`。C の nv_band とは N1a で 5706/5706 一致している。
   - AFT は代理の呼び出しが `nv_aft_v5.apply_aft` (Python) に落ちるだけ。
   - したがって ON/OFF で同じ値になる。ON/OFF の自己試験と書き出しの一致で確かめる。

---

## 4. 互換性

- **プリセット** (`nv_repro_filter_preset`) はフィルタのパラメータだけを持つ (`state.py:270`)。イコライザを含まないので影響はない [観測]。
- **プロファイル JSON** (`.nvprof.json`、Default Profile の QSettings、クリップボード、Variants の `d["profile"]`。
  いずれも `ProfileState.from_json` で読む):
  - 旧形式 (`channels` なし) は**旧モード**で読む。
    - factors を従来どおり Y/Cb/Cr の乗数とし、`effective_profile` も従来の関数を使う。数値はビット単位でいまと同じ。
    - イコライザは Y/Cr/Cb の 3 本で、"legacy Y/Cr/Cb profile" と表示する。
    - AFT を走らせるか、ビルドか Reset をすると RGB モードに移る (factors はどうせ 1/AFT 値に置き換わる)。
    - 旧形式の AFT 済みプロファイルは `aft_levels` を持たない。この点は §7 の U9。
  - 新形式 (案、§7 の U8 で決める):
    `{"format": "nv_repro_profile_rgb", "version": 2, "channels": "RGB", "device", "base", "factors" (R/G/B 3×9), "pinned", "tune"}`
- **古い UI が新形式を読んだとき** [観測: いまの `from_json` は format を見て、version は見ない]:
  - format を `nv_repro_profile` のままにすると、古い UI は R/G/B のレベルを Y/Cb/Cr の乗数として読む。
    その結果、AFT 済みの base にもう一度 AFT のレベルを掛けることになり、**黙って違う出力になる**。
    これは避けるべき。
  - format を変えれば、古い UI は "not a repro profile file" で拒否する (明示的なエラー)。
- **CLI** (`denoise_video_v5.py --params`): パラメータの JSON なので影響はない。
  プロファイルを読むのは `nv_ui_qt/export.py --ui-profile` だけで、読むのは有効プロファイル (曲線は掛け算済み)。
  新しく増える注記 `ui_channels`・`ui_factors`・`aft_levels`・`ui_rgb_weights` は読まない (処理は v5 の dict のキーだけを見る) [観測]。
- **書き出しの `.ui_profile.json`**: 形式は変えない。有効プロファイルの dict に注記が増えるだけ。
  `ui_channels: "RGB"`、`ui_factors` = R/G/B のレベル、`ui_tune`。旧モードでは従来の注記のまま。

---

## 5. 影響

### 5.1 既定の出力は変わらない (論拠と実測)

- **構造上の恒等**: 編集がなければ base を返す。
  - AFT 前は L = 1 = ref、AFT 後は L = `aft_levels` = ref。tune = 1 のとき `effective_profile` は base を返し、
    出力はいまと同じオブジェクトになる。
  - AFT 後の base の曲線は、新しい `_run_aft` が結合で作る。これは apply_aft の結果とビット一致する (下の A)。
  - したがって、ビルド → 自動 AFT → 書き出しという既定の流れは、いまとバイト一致すると見込める [推測。実装後に確かめる]。
- **数値上の恒等** [観測、`g1/check_combine.py`]:
  1. 保存済みの AFT プロファイル 9 種で、`ui_pre_aft` の曲線 × 結合(`aft_face_curves`) = base の曲線と sigma: **9/9 ビット一致**。
  2. L = 1 を結合に通す: 曲線 × c_cj の組で **427,917/427,917 ビット一致**。
     - 曲線は実物と、乱数の float32 に極端値 (1e-45・1.17e-38・3.4e38・0) を混ぜたもの。
     - c_cj は実物 9 と、乱数の q99 から作った 2000 (一部は q = 0)。
     - c_cj の行の和は 6027 行中 1098 行で 1.0 ちょうどにならないが、√ の後に f32 へ丸めると恒等になる。
  3. 1 ビンを編集したとき、そのビンの補間に関わらない中心の値は不変: 223,815/223,815。
  4. tune だけの経路は、いまと新しい式で一致: 1065/1065。
- **実フレームでの確認** [観測、`g1/check_probe.py`、test.mp4 n=300、Python の経路、AFT 8 秒 × 2]:
  - A. 代理の呼び出しで得たレベルを結合した結果が apply_aft の curve・sigma と一致。
    cw・旗・`aft_face_curves` も一致。np.interp を節点で評価するとレベルが厳密に返る。
  - B. `model_levels` の q99 から作った c_cj が、AFT の c_cj と一致。

### 5.2 編集の意味の変化 [観測、`check_combine.py` の 5・6]

- All +20% (R = G = B = 1.2): 曲線は ×√1.2 = ×1.0954。旧モードの Y/Cb/Cr では ×1.2 だった。
  −50% は ×0.7071 (旧 ×0.5)、+100% は ×1.4142 (旧 ×2)。
- 1 面だけ +20% のとき (test_v02 系の実物の c_cj):

  | 編集 | Y | Cb | Cr |
  |---|---|---|---|
  | R +20% | ×1.0195 | ×1.0076 | ×1.0558 |
  | G +20% | ×1.0748 | ×1.0295 | ×1.0404 |
  | B +20% | ×1.0027 | ×1.0606 | ×1.0014 |

- これは apply_aft の「T_j がレベルに比例して √ の内側に入る」式に従った結果である (§7 の U1)。

### 5.3 Quality/Completeness の写像

- RGB モードでは、曲線バイトのチャンネル c = 面 R/G/B c になり、本家と同じ並びになる。
  - いまの Y/Cb/Cr と tk の並びの食い違い (§1.1 の 6) は、RGB モードでは起きない。
- レベル語の決め方 (§7 の U3) で、Completeness が変わる [観測、test.mp4 n=300、auto 領域 48²、AFT 後]:

  | レベル語 | Quality | Completeness | 備考 |
  |---|---|---|---|
  | 1.0 (いまの写像) | 43% | 70% | 19/27 |
  | AFT のレベル L | 45% | 80% | |

  - 未測定ビン (marker) の数は R/G/B で 6/6/7。
  - 違いは「次のビンの語 0 の下位バイト ≠ 0 → +1/81」の経路から来る。
- 本家の観測「全面 + AFT で完成度 85% = 23/27」(QUALITY.md §8.4) は、測定ビン 4 個の寄与の合計が 0 であることを意味する。
  - レベル語に AFT のレベル (下位バイトはほぼ非 0) を書くと、測定ビンは +1/81 されやすい。
  - この観測は「語 0 = 1.0 のまま」の写像と整合する [推測。本家の測定ビンのレベル語の値は未観測]。
- 手動の編集は、マーカーなしで「下位バイト ≠ 0 なら +1/81」になる。これは観測で確定している (QUALITY.md §8.5)。
  RGB モードでも、編集したビンの L をそのままレベル語に書けば本家と同じ。

### 5.4 本家の観測との整合

- [観測] `genuine_ui/OBSERVED.md:59-60`: 線は R・G・B の 3 本で点は 9 個、AFT の後は曲がる。
  下に [RGB][R][G][B] の選択と "90%" の欄 (= 完成度) がある。
- 本案は AFT 後の L を表示するので、曲線が曲がって見え、本家と合う (いまは平ら)。
- 線とトグルの色は、本家の RGB ペインの色に合わせる: R ピンク、G 緑、B 藤色 (`OBSERVED.md:34`) [推測。イコライザの線の色そのものは未確認]。
- 本家の σ マップは T_j[bin_j(x)] で、各面の自分の輝度のビンを引く (`wp_spatial/notes/spatial_analysis.md:65-73`)。
  repro は v5 の Y 輝度の 8 中心で R/G/B の曲線を評価する。これは apply_aft から引き継いだ近似で、本案でも変わらない (§7 の U2)。

---

## 6. 変更するファイル・見積もり・試験計画

### 6.1 ファイル (処理コア `nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py` と `nv_native.py` には触れない)

| ファイル | 変更 | 規模 |
|---|---|---|
| `repro/nv_ui_qt/state.py` | `channels`、RGB の有効曲線、from_json/to_json (新旧の形式)、set_factors_all、Auto Complete | +120 |
| `repro/nv_ui_qt/backend.py` | `_run_aft` を代理の呼び出し + 結合に、build に `ui_rgb_weights` | +60 |
| `repro/nv_ui_qt/main_window.py` | AFT 完了時 L ← aft_levels、Reset L ← 1、旧モードの表示と切り替え | +40 |
| `repro/nv_ui_qt/widgets/curve_editor.py`・`profile_panel.py`・`rows.py` (色) | R/G/B の線と [RGB][R][G][B]、旧モードは Y/Cr/Cb、docstring・tooltip | +60 |
| `repro/nv_ui_qt/quality.py` | RGB モードの写像 (U3・U4 の決定に従う) | +20 |
| `repro/nv_ui_qt/selftest.py` | 下の項目 | +150 |
| `native/nv_aft.{h,cpp}` | combine を color_weights + combine_levels に分割 | ±60 |
| `native/nvcore_c_prof5.{h,cpp}` か新しい `nvcore_c_rgb` | `nvc_rgb_weights`・`nvc_rgb_effective_curve`、ABI 版を上げる | +80 |
| `native/nv_ui.cpp`・`nvcore_c_ui` | quality_numbers の RGB モード (引数 1 個) | +20 |
| `native/verify_bridge_prof5.py` (か新しい `verify_rgb_eq.py`) | Python と C ABI の照合 | +120 |
| `qtui/src/core/state.{h,cpp}`・`nvcore.cpp` | 上と同じ (run_aft で aft_levels、build の weights、quality) | +150 |
| `qtui/src/widgets/curve_editor.cpp`・`profile_panel.cpp`、`app/mainwindow.cpp` | 表示と操作 | +100 |
| `qtui/tests/selftest.cpp`・`fixtures/` | 下の項目、旧形式の fixture は残し、新形式の fixture を追加 | +150 |
| `AGENTS.md`・`native/MIGRATION.md` | イコライザの節と「本家と違う点」を更新 | 文書 |

見積もり: 実装 1 セッション + 照合と試験 0.5 セッション (Python → C ABI → qtui の順)。

### 6.2 試験

1. **回帰**: `make -C native verify` 11 本。分割した combine で N1i の結合 69/69 が変わらないこと。
   verify_bridge の全語一致 (AFT 26/26 も)。
2. **新しい照合 (Python と C ABI)**:
   - L の作り方:
     - 乱数 (0.2/3.0 の境界と round(…, 3) の値を含む)
     - unit と ref (恒等で base が返ること)
     - AFT の再現 (保存済み 9 種 + 新規)
     - 1 ビンだけ・1 面だけ・All の編集
   - tune: 組み合わせを変える。
   - c_cj: 実物と乱数 (q = 0 で 1/3 の経路も)。
   - `nvc_rgb_weights` を `model_levels` → `_color_weights` と比べる。
3. **両 UI の自己試験に足す項目** (Python 96 → 約 106、C++ 105 → 約 115):
   - 新規ビルド: L = 1、`ui_rgb_weights` がある、有効曲線 is base。
   - AFT 後: L == `aft_levels`、有効曲線 == base (ビット)、イコライザが曲がって表示される。
   - `_run_aft` (代理 + 結合) == `nv_aft_v5.apply_aft` を直接呼んだ結果 (curve・sigma・旗。ON/OFF)。
   - ビルド時の c_cj == AFT の c_cj (同じフレーム)。
   - R の bin 4 を +20%: Y/Cb/Cr で変わるのは中心 112・144 だけ。値 = §2.3 の式。
   - All +20%: 曲線 ≈ ×√1.2 (rtol 1e-6)。Undo/Redo で元のビットに戻る。
   - Reset: 有効曲線 == `ui_pre_aft` の曲線 (U5 を採る場合)。Completeness 0%。
   - 新形式の保存 → 読み込みの往復 (ビット)。旧形式の fixture (いまのコードで保存したもの) が、いまと同じ曲線で読める。
   - 新形式を旧の format として読むと拒否する (U8 で format を変える場合)。
   - Quality/Completeness の RGB 写像の数値 (U3・U4 の決定に従う)。
   - 既存の "manual fine-tune + auto complete" と tune (d) の項目は、RGB の意味に書き換える。
4. **+20% 編集での UI 間のビット一致**: 次の 4 通りで、処理したフレームを PySide6 と Qt C++ で比べる
   (N3 のダンプ比較 `qtui/tests/dump*.cpp` の方式)。
   - R bin 4 +20%
   - G 全ビン +20%
   - All bin 2 +20%
   - AFT 後に B bin 6 −30% + tune all 1.1
5. **書き出しの一致**: 上の編集ありで、ON/OFF × 両 UI の 4 通りがバイト一致すること。
   編集なしで、変更前のコードの書き出しともバイト一致すること (既定の出力が不変であることの直接の証拠)。

---

## 7. 未解決の点と、ユーザーに決めてほしいこと (推奨は ★)

- **U1 結合式の T_j の次数**
  - 状況: apply_aft は √(Σ c·L) で、L を分散の乗数として扱う。本家の σ² = scale·Σ m²·a²·T_j は観測済みだが、
    T_j が L か L² かは未確認 (Agents.md でも「未確認」)。
  - ★ apply_aft と同じ L を使う (AFT の再現をビット一致させるために必須)。これは記録しておく。
    L² にすると、AFT の既定の出力が変わる。
- **U2 R/G/B の曲線をどの輝度で引くか**: v5 の Y の中心で評価する (近似)。★ 現状のまま (処理コアを変えられない)。
- **U3 AFT 後の Quality のレベル語**
  - (a) ★ 1.0 (未編集) のままにし、手動で編集したビンだけ L を書く。いまの数値 (例 43%/70%) を保ち、本家の 85% の観測とも整合する。
  - (b) 常に L を書く。構造は本家に近いが、test.mp4 で 45%/80% になる。本家で AFT 測定ビンのレベル語が観測されたら見直す。
- **U4 tune を Quality に入れるか**
  - いまは F32(factor·tk) をレベル語に入れている (並びの食い違いあり)。このため Overall の Noise Level を 1.1 にするだけで、
    Completeness が最大 +26/81 動く。
  - ★ RGB モードでは入れない (tune は repro 独自の乗数で、本家のビンのレベルではない)。
  - 旧モードは現状維持 (ビット互換)。並びの食い違いを直すかは別に判断する。
- **U5 Reset Fine-Tuning の意味**
  - ★ L ← 1 で AFT 前の曲線に戻す (本家ではレベルが 1.0 = σ マップを補正しない)。
    いまの repro は AFT 後の曲線のまま残すので、出力が変わる。
  - 別案: いまの挙動を保つ (L ← 1 の基準を AFT 後の曲線とする「相対モード」が要り、複雑になる)。
- **U6 Auto Complete の固定点**
  - ★ pinned ∪ AFT の測定ビン (`aft_bin_flags.marker` が偽)。本家の文言 "interpolating missing values using measured data" に近い。
  - 別案: いまと同じく pinned だけ (AFT 後に押すと、AFT のレベルを補間で上書きする)。
- **U7 All トグルのドラッグ**
  - ★ 3 面の同じビンを同じ値にする (いまと同じ)。
  - 別案: 3 面を同じ差分だけ動かす (AFT の面の差が残る)。本家の挙動は未観測。
- **U8 新形式のファイル**
  - ★ format を `nv_repro_profile_rgb` に変える (古い UI は明示的に拒否する。N1 の F2 の方針「未到達経路は明示的エラー」と同じ)。
  - 別案 1: format はそのままで、`channels: "RGB"` と `factors` を書く。古い UI は黙って AFT を二重に掛ける。不可。
  - 別案 2: format はそのままで、レベルを別のキーに置き `factors` を書かない。古い UI は黙って手動の編集を落とす。
- **U9 旧形式のプロファイル**
  - ★ 旧モード (Y/Cr/Cb) のまま読み、AFT・ビルド・Reset で RGB に移る。
  - 別案: 読み込み時に変換する。旧形式の AFT 済みのものは `aft_levels` を持たないので、平らな相対表示しかできない。
    AFT 前のものは、c_cj にフレームが要る (別のクリップでは不正確)。
- **U10 既存の不具合** (Quality の tk の並びの食い違い): ★ 旧モードは数値の互換のため現状維持で、記録だけ。
  直すなら両 UI 同時に直し、自己試験の期待値も更新する。

以上の決定 (特に U3・U5・U6・U8) の後に実装する。

---

## 決定 (2026-10-30): ユーザーが U1〜U10 をすべて ★ で承認

- U1: 結合式は apply_aft と同じ √(Σ c·L) (T_j の次数は未確認として記録)
- U2: R/G/B 曲線は v5 の Y 中心で評価 (現状のまま)
- U3: AFT 後の Quality のレベル語は 1.0、手動で編集したビン (pinned) だけ L を書く
- U4: RGB モードでは tune を Quality のレベル語に入れない (旧モードは現状維持)
- U5: Reset Fine-Tuning は L ← 1 で AFT 前の曲線に戻す (出力が変わる)
- U6: Auto Complete の固定点 = pinned ∪ AFT の測定ビン (marker が偽)
- U7: All トグルは 3 面の同じビンを同じ値にする (現状どおり)
- U8: 新形式は format "nv_repro_profile_rgb" (古い UI は明示的に拒否)
- U9: 旧形式は旧モード (Y/Cr/Cb) で読み、AFT・ビルド・Reset で RGB に移る
- U10: Quality の tk の並びの食い違いは旧モードで現状維持 (記録のみ)
