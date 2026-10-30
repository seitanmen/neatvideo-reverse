# wp_native_spatial / wp_native_temporal 報告書 (N1h: spatial_v5 / N1g: temporal_v5 の C++ 移植)

日付: 2026-10-27。担当: N1h+N1g 作業エージェント (サブエージェント、1 人で N1h → N1g の順)。
REPORT.md は作業者が書かなかったため、最終応答を統括が転記した。N1h と N1g は共通部品を共有するので、同じ本文を
`sandbox/omo/wp_native_spatial/REPORT.md` と `sandbox/omo/wp_native_temporal/REPORT.md` の両方に置く。

## 結論
N1h と N1g の C++ 移植は、どちらも Python とビット一致した (作業者の観測)。
- 照合件数: 空間 1287/1287 件、時間 474/474 件。NaN を含むケースではペイロードまで比べた。
- 実行: `make -C native -f Makefile.v5 verify` の一発で通る (`make -C native -f Makefile.v5 && python3 native/verify_spatial.py && python3 native/verify_temporal.py` と同じ)。キャッシュがあれば合計で約 35 秒。
- 警告は 0 件 (clean build で確認)。

守った規則 (作業者の申告):
- repro/ と neatvideo_ofx_614/ は読んだだけで、nvfilter.py も変更していない。
- 本家バンドル・nvh の実行、git の書き込み、AGENTS.md と native/Makefile の編集はしていない。
- Python の並列度は 3 以下に抑えた。

---

## 共通部品 (N1h・N1g で共有)

### 作ったもの
`native/nv_ndimage.{h,cpp}` (名前空間 `nvn`)。この環境の scipy 1.15.3 と numpy 2.1.3 (conda、arm64 ビルド) の部品の演算を写した。

- **gaussian_filter(σ=1)**: 半径 4、reflect。軸 0 → 軸 1 の順で、間は float32 に丸める。
- **uniform_filter**: サイズ 3/5/7/9、reflect または nearest。
- **map_coordinates**: order=1、nearest。
- **median_filter**: 3×3、nearest。
- **np.interp / curve_sigma**
- **_b3 / atrous、_down2 / _fit / _to_grid**
- **np.maximum(配列, スカラー)・np.clip・np.exp・Python の max/min**

### 根拠
- scipy と numpy の C ソースは、該当タグのものを取得して読んだ。置き場所は `sandbox/omo/wp_native_spatial/ref_src/`。
  - scipy v1.15.3: ni_filters.c、ni_support.c、ni_interpolation.c、ni_splines.c
  - numpy v2.1.3: compiled_base.c、loops_utils.h.src、loops_minmax、clip.cpp、reduction.c、nditer
- 被演算子の順 (NaN ペイロードを決める) は、インストール済みの .so を `otool -tv` で逆アセンブルして読んだ。

### 確定した挙動
「観測」は、逆アセンブルか numpy/scipy との直接比較で確かめたもの。

1. **gaussian_filter は FMA を使う (観測)**。
   - `NI_Correlate1D` の対称分岐は `d0 = x0*w0` のあと、k=4..1 で `fmadd(x[-k] + x[+k], w[k], d0)` を double で計算する。
   - 打ち消し合うように作った窓 40 件で、scipy の出力は融合版と 40/40 一致し、非融合版とは 0/40。
   - 重みは `_gaussian_kernel1d` の double ビットパターンを写した。カーネルは厳密に対称なので対称分岐に入る。
2. **uniform_filter (観測)**: double の移動和。
   - `tmp = 0; tmp += x…; out = tmp/size` で始める。
   - 以降は `tmp = tmp + (x_in − x_out)` を double で回す。
3. **map_coordinates (観測)**
   - 重みは `x = cc − floor(cc)`、`w0 = 1 − x`、`w1 = 1 − w0`。w1 は x そのものではない。
   - 和は `t = 0; t += (v*wy)*wx` を (0,0)(0,1)(1,0)(1,1) の順に足す。境界のタップはクランプする。
4. **median_filter**: NI_Select (Hoare 分割の quickselect) を double バッファでそのまま再現した。NaN を含んでも一致する。
5. **np.interp は FMA を使う (観測)**。
   - `fmadd(slope, x − xp[j], fp[j])` と、結果が NaN のときの再計算も再現した。
   - 作った表では、numpy と融合版が 8/8 一致した。
6. **np.exp(float32) は libm の expf**。40 万値でビット一致した (観測)。volatile ポインタ経由で呼んでいる。
7. **np.maximum(配列, 0.0) はスカラー経路**。
   - NEON でもこの場合は SIMD を通らず、`(a >= b || isnan(a)) ? a : b` になる。
   - そのため max(−0, +0) = −0 (ソースと照合で確認)。
8. **numpy の加算リダクションの順 (観測)**。Python で総当たりし、NaN ペイロード 2 種で被演算子の順も決めた。
   - **pairwise 和 (n<8)**: ソースは `res = −0.0` から始めるが、コンパイラが `−0.0 + x` を畳み込むので先頭値がそのまま入る。そのため signalling NaN は静音化されない。
   - **8 本の累積器をまとめる木の加算**: 各対が (右, 左) の順にコンパイルされている。
   - **外側の累積**: `pw + out` の順。
   - **_down2 (mean((1,3)))**: 通常は `(+0 + pw(a,b)) + pw(c,d)`。出力が 1 列幅 (W//2 == 1) のときだけ、`((((a+b)+c)+d)+0)` の逐次になる。40×40 以下の全形状と 4K などで確認した。
   - **_search の sum((1,3))**: ブロック行ごとの pw を `+0` に足していく。nbx == 1 のときだけ、blk² 個の値をまとめて pw する。
   - **_refine の sum((2,3))**: blk² 個の値をまとめて pw する。
9. **LLVM の被演算子入れ替えへの対策**
   - N1a/N1b の報告と同じ問題なので、fadd・fsub・fmul・fdiv・fsqrt・fmadd (float と double) をすべて AArch64 インラインアセンブラで書いた (`A/S/M/D/SQRT/AD/SD/MD/DD/FMAD`)。
   - 定数は Python のビットパターンから `bits()` / `dbits()` で写した。

---

## N1h: v5 空間カーネル (spatial_v5 + Mix)

### やったこと
`nvfilter.spatial_v5` と、`process_frame_v5` の Mix を移植した。
- 中身は a-trous 5 帯域、7×7 の局所 Wiener、noise_level / channel_noise / band_ratio / band_noise / band_amounts。
- `amt` は Python の min/max を double で計算してから float32 に丸める。
- Python の数値は numpy の weak scalar と同じく演算の前に float32 へ変換し、左から順に計算する。

### 作ったファイル
- `native/nv_spatial.h`・`native/nv_spatial.cpp`: `nvs::spatial_v5`・`nvs::mix_v5`、構造体 `V5Profile` / `V5Params`。
- `native/nv_ndimage.{h,cpp}`: 共通部品。
- `native/nv_v5_main.cpp`: 照合ドライバ `nv_v5_test` (N1g と共用)。部品単体も呼べる。書式はファイル先頭のコメント。
- `native/Makefile.v5`: N1g と共用。フラグは native/Makefile と同じ。
- `native/v5_cases.py`: 両 verify の共通部 (ブロブ形式、ffmpeg でのフレーム切り出し、プロファイルの JSON キャッシュ)。
- `native/verify_spatial.py`

### 検証結果: `verify_spatial.py` 1287/1287 ビット一致
455 件は期待値に NaN を含み、その NaN 語は 930,522 語。

| 区分 | 件数 | 中身 |
|---|---|---|
| comp (部品) | 1197 | uniform 512、to_grid 144、atrous 180、curve 66、gauss 66、mapc 61、median 60、exp 60、down2 48 |
| rand | 60 | spatial 30 + spatial+Mix 30 |
| real | 30 | 3 フレーム × 2 形式 × 5 パラメータ組、計 139,968,000 語 |

- **comp の入力**
  - 15 形状 (1×1〜128×96) × 4 種 (正規、特殊値、NaN が密なもの、−0)。
  - 特殊値の中身は、NaN ペイロード 6 種・sNaN・±inf・±0・非正規・±1e38。
  - 加えて、上の挙動 1・3・5 と uniform の加算順を突く構成入力 (`adversarial_cases`) を入れた。
- **rand の入力**
  - 色差の格子は 4:2:0 / 4:4:4 / 4:2:2、大きさは 2×2〜130×131。
  - 種類は正規、整数、定数、特殊値、密な NaN、−0、1e30 規模。rv にも NaN・inf・0・負値を混ぜた。
  - 帯域段数は Y・色差とも 1..5。プロファイルとパラメータはすべて乱数で、luma/chroma は −0.3〜1.4、band_amounts は −0.2〜1.3、mix は 0 / 乱数 / 1 / −0.2。
- **real の入力**
  - フレームは test_v02 の n=100/312/576 で、yuv420p と yuv444p の両方。
  - プロファイルは、そのフレームの `build_profile_v5` (FindFlatArea と次フレーム)。
  - 入力は `temporal_v5(n−2..n+2)` の出力とその rv。
- **real のパラメータ 5 組**

  | 組 | 設定 |
  |---|---|
  | default | 既定値 |
  | weak | luma 0.4、chroma 0.6、spatial 1.0、noise_level 1.3、channel_noise 変則 |
  | bands | band_amounts / band_noise 変則、spatial 3.5 |
  | mix | mix 0.3 |
  | clamp | luma −0.5、chroma 2.0、noise_level 0、channel_noise に 0 を含む、spatial 0、mix 1.0 |

---

## N1g: v5 時間カーネル (temporal_v5、estimate_flow_v5 など)

### やったこと
次を移植した。
- `estimate_flow_v5`
  - gaussian、_down2 ×2。
  - _search: SAD、3×3 nearest の uniform、λ 罰則、厳密な小なりで選択。
  - _refine ×3: SSD、argmin は最初の NaN か最初の最小。
  - 放物線によるサブ画素 (np.maximum / clip / where)、median_filter。
- `dense_flow`、`warp_plane`。
- 類似度 "v5" (5×5 uniform → maximum → 符号反転 → expf) と "pyramid" (`pyramid_weight`)。
- 色差の `_to_grid(F)/2` と `_to_grid(w)`、acc / sw / sf の累積、out と rv (残留分散) の式、`curve_sigma`。
- `process_frame_v5` (窓の切り詰め → 時間 → 空間 → Mix)。
- flows のキャッシュは `std::map<int, Flow>*` で受け渡す。Python の dict と同じく、無ければ推定して格納する。

### 作ったファイル
- `native/nv_temporal.h`・`native/nv_temporal.cpp`。照合用の入口 `nvt::detail::block_sum13` と `refine_test` を含む。
- `native/verify_temporal.py`
- ドライバ・Makefile・v5_cases.py は N1h と共用。

### 検証結果: `verify_temporal.py` 474/474 ビット一致
比較したのは、出力 3 面、rv 3 面 (temporal のみ)、呼び出し後のキャッシュに入っている全近傍の (vy, vx)。147 件は期待値に NaN を含む。

| 区分 | 件数 | 中身 |
|---|---|---|
| comp | 368 | blocksum 120、refine 120、flow 44、dense 44、pyramid 24、warp 16 |
| rand | 64 窓 | temporal 48、process_frame 16、フロー 184 組 |
| real | 42 | temporal 36、process_frame 6、フロー 174 組、計 1.09 億語 |

- **comp の中身**
  - blocksum と refine: _search / _refine のリダクションと argmin を numpy と直接比べる。
    - blk は 1〜32。pairwise が 128 個を超えて再帰する blk=17/32 も入れた。
    - 格子は 1×1 / 1×3 / 3×1 など。NaN ペイロードも混ぜた。
  - flow: bs/search の 6 組。
- **rand の窓**
  - 中身: 動くテクスチャ + ノイズ、2〜7 フレーム、4:2:0 / 4:4:4、16×16〜128×160。nbx=1 / nby=1 の形状も含む。
  - パラメータの振り方:

    | パラメータ | 値 |
    |---|---|
    | similarity | v5 / pyramid |
    | radius | 0..3 |
    | temporal | 1, 0.6, 1.3, 0, −0.2 |
    | threshold | 4, 2, 8, 0.5, 0 |
    | pyramid_scale | 1.5, 1, 2.5, 0 |
    | bs | 4 / 8 / 12 / 16 / 32 |
    | search | 0 / 1 / 3 / 4 / 8 / 16 |

  - 一部の窓は、任意のベクトルを入れたキャッシュを事前に渡す。
  - 特殊値: NaN (Y・色差、ペイロード混在)、±inf (Y)、1e6 倍、定数フレーム。
  - inf 入りの 3 窓では、inf − inf の NaN がブロックベクトルまで届き、median にも NaN が入った。それでも一致した。
- **real の窓**
  - フレームは test_v02 n=310..316 (1080p yuv420p) と、test.mp4 n=298..304 (640×360、yuv420p と yuv444p)。
  - temporal は、中心を 3 通りにした 5 フレーム窓 × 4 組。組は次の 4 つ。
    - default
    - pyramid
    - threshold 2 + bs 8 + search 8 + temporal 0.6
    - pyramid + scale 2.5 + noise_level 1.3 + bs 32 + search 12
  - process_frame_v5 は 7 フレーム全体で 2 組: default と、pyramid + mix 0.2 + radius 3。

---

## 感度試験 (両方)
- スクリプト: `sandbox/omo/wp_native_spatial/mutate.py`。変異を 1 つずつ入れてビルドし、`--no-real` で両 verify を回した。
- 結果: **22 種中 19 種を検出**。

| 結果 | 変異 (検出時の不一致件数) |
|---|---|
| 検出 | gauss の FMA なし (3)、gauss の加算順 (12)、uniform の和の順 (32)、mapc の w1=x (1)、median の sort 化 (21)、interp の FMA なし (6)、down2 の 1 列特例なし (31)、_b3 の結合順 (203)、np.maximum の fmax 化 (11)、exp の double 計算 (23)、pairwise 木の順 (77)、pairwise の −0 畳み込み (2)、_search の nbx=1 特例なし (24)、argmin で後の最小を選ぶ (19)、dense の (bs−1)/2 を整数除算 (66)、warp の上側 clip なし (2)、pyramid の ε (28)、sf の結合順 (27)、Wiener の g の clamp なし (34) |
| 未検出・等価 | uniform の ÷size を ×(1/size) にする。float32 の数値の少数個の和では、商が float32 の丸め境界に乗るとき double の誤差が吸収されるので等価とみる (推定。検算はした) |
| 未検出・等価 | rv の加算順。両辺の NaN が同じペイロードから来るので結果に出ない |
| 未検出・等価 | spatial の `e + 1e-6` の順。対照群で、定数は NaN にならないので等価 |

補足:
- FMA と w1 の違いは、乱数入力ではほぼ現れない (float32 の丸め境界付近でしか出ない)。そのため構成入力を作って検出できるようにした。
- _search の和の順は、下流では整数ベクトルにしか表れない。そのため blocksum / refine の直接比較で検出した。

---

## 計測時間
条件: M2 Max、負荷平均 6〜9、test_v02 1080p の 1 フレーム、5 回測定。C++ は 1 スレッドのスカラー asm で、ベクトル化していない。計測は `sandbox/omo/wp_native_spatial/dev/bench.py`。

| 処理 | Python | C++ | 速度比 |
|---|---|---|---|
| spatial_v5 | 0.177〜0.192 秒 | 0.106〜0.116 秒 | 約 1.7 倍 |
| temporal_v5 "v5" | 1.87〜1.98 秒 | 0.437〜0.453 秒 | 約 4.4 倍 |
| temporal_v5 "pyramid" | 2.01〜2.15 秒 | 0.557〜0.574 秒 | 約 3.7 倍 |
| process_frame_v5 | 2.07〜2.14 秒 | 0.542〜0.564 秒 | 約 3.8 倍 |

照合全体の時間:

| 照合 | Python | C++ |
|---|---|---|
| 空間 | 8.0 秒 (3 プロセス、CPU 10.7 秒) | 2.4 秒 (3 スレッド) |
| 時間 | 15.9 秒 (CPU 40.6 秒) | 3.7 秒 |

---

## Python 側への不具合報告・注意 (直していない)
1. **型昇格の落とし穴 (numpy 2 の NEP 50、観測)**
   - P やプロファイルの値が numpy の float64 スカラーだと、float32 配列 × np.float64 が float64 になり、以降の計算が倍精度に変わる。
   - C++ が再現しているのは、Python の float / int (float32 へ変換) の場合だけ。
   - プロファイルを作る関数 (`build_profile_v5`・`nv_aft_v5` の `.tolist()`) は Python の float を返している。
   - `nv_ui_qt/backend.py:62` の `P["noise_level"] * tn` は、tn が numpy の型なら該当する (未確認)。
2. **Python の結果が numpy/scipy のビルドと版に依存する**
   - 依存するのは、FMA の有無、加算リダクションの順、NaN の被演算子の順。いずれもこのビルドで観測した。
   - 加算リダクションの順は、numpy の反復子が形状で変える (1 列幅と nbx=1 の特例)。
   - numpy/scipy を更新したり x86 に移したりすると、Python 自身の最下位ビットが変わり得る。
   - 照合の前提は、この環境 (numpy 2.1.3 / scipy 1.15.3、arm64) に固定される。
3. **フレームが 1 ブロック (bs) より小さいと例外**
   - `estimate_flow_v5` は `_refine` の空配列の max で ValueError になる (観測)。
   - C++ も同じ所で `std::invalid_argument` を投げる。
4. **一本の inf でその行が NaN になる**
   - uniform_filter は移動和なので、一度 inf が入ると、窓を抜けるときの inf − inf でその行や列の残り全体が NaN になる。
   - scipy の性質で、Python 版も同じ。実写では起きない。
5. **pyramid の帯域段数**
   - Y の band_ratio が 3 段未満だと、Python は IndexError になる。
   - C++ は 5 段の配列を読み、足りない分は 0 になるので、Python と動作が違う。
   - 通常のプロファイルは 5 段。

---

## 未移植・未検証
- インラインアセンブラは AArch64 だけ。他のアーキテクチャでは通常の演算になり、NaN ペイロードと被演算子の順は保証しない (未検証)。
- フレーム内の並列化はしていない (ドライバのケース単位の並列だけ)。ベクトル化もしていない。
- similarity が "v5" / "pyramid" 以外のとき、Python は ValueError、C++ (整数 0/1) は 1 以外を v5 として扱う。
- 時間カーネルの 4:2:2 は未照合 (空間の乱数では照合済み)。実写の 4:4:4 は test.mp4 だけ。
- curve_centers は厳密に単調増加の前提 (節点が重複するときの線形探索との違いは未照合)。
- 0 行・0 列の面は対象外 (Python も失敗する)。

---

## 参考・統合時の注意
- 作業物:
  - `sandbox/omo/wp_native_temporal/dev/probe_*.py`: リダクション順と NaN 順を総当たりで決めた探査。
  - `sandbox/omo/wp_native_spatial/dev/test_comp.py`: 部品の初期試験。
  - `sandbox/omo/wp_native_spatial/ref_src/`: 参照ソース。SHA256 の先頭 16 桁は次のとおり。

    | ファイル | SHA256 (先頭 16 桁) |
    |---|---|
    | ni_filters.c | e698ae4e27d16170 |
    | ni_support.c | 3d8291f4407bc78f |
    | ni_interpolation.c | 5ec01d71453bc777 |
    | ni_splines.c | 7c49045c917d827d |
    | compiled_base.c | bbf953627a8c7df3 |
    | loops_utils.h.src | ed85d5b99f0b457c |
    | loops_minmax.dispatch.c.src | 231c46b276b326e2 |
    | clip.cpp | 28d217d287823031 |
    | reduction.c | 1b3afe26fec8390e |

- キャッシュ: `sandbox/omo/wp_native_spatial/tmp/` が約 748 MB (フレーム、rv、プロファイル)、`sandbox/omo/wp_native_temporal/tmp/` が約 111 MB。消しても次回に再生成される。
- .gitignore (観測): `git check-ignore` によると、`native/Makefile.v5` は `.gitignore:22 native/**` に掛かって無視されている。.cpp/.h/.py は未追跡として見えている。コミット時に許可リストへの追加が必要。
- `native/__pycache__` には、作業者が作った pyc を残していない。
