# N1a 報告: 帯域測度 (nv_band) の C++ 移植

作成: 2026-10-26 (N1a ワーカー = サブエージェント)。報告ファイルは書けなかったため、最終応答を統括が転記した。
指示書 `.claude/tasks/native_n1_split.md` の「共通規約」「検証の合格ライン」「N1a」。
repro/・処理コア・git・本家バンドル・nvh には触れていない (repro は読み込みのみ)。

## 1. やったこと
`repro/nv_band.py` の `model_levels` (FUN_00995250 の語 0..54) と、その部品を C++ に移植した。
同じ部品で書ける `band_levels` (FUN_009980e8。N1d の FindFlatArea が使う) も移植し、4096 語すべてを照合した。

| C++ (`native/nv_band.{h,cpp}`、名前空間 `nvb`) | Python の移植元 |
|---|---|
| `model_levels` / `band_levels` / `init_struct` | `nv_band.model_levels_bm` / `band_levels_bm` / `init_struct` (B = 1) |
| `box_rows` / `box_cols` (内部 `lanes_run`・`box3_row`) | `nv_lines.box_rows_bm` / `box_cols_bm` (`_lanes_run`・`_box3_rows_bm`。SIMD = True の経路) |
| `down2` | `nv_pixnoise.down2` / `nv_band._down2_into` |
| `band_from` (内部 `up2_scalar`) | `nv_band._band_into` (NEON 経路) と `nv_pixnoise.band_from_bm` の scalar 経路 (`_up2_scalar`) |
| `mean_exact` / `std_exact` | `nv_band.mean_bm` / `std_bm` (2048 値のブロック、4 レーン×2 組、`_reduce8`、double のブロック和、std の FMLA = `std::fma`) |
| `quantile` / `probit` | `nv_band.quantile` (q < 1) / `probit` |
| `cv` | `nv_band.cv_bm` (`_sub_m` の FMLA・端数部の非融合を含む) |
| 内部 `borders` / `local_rms_crop` / `measure` / `extrapolate` | `nv_band._borders` / `_local_rms_crop` / `_measure_bm` / `_extrapolate` |

- 定数はすべて Python 側のビットパターンから写した (`bits(0x...)`)。probit 表の 102 語は `nv_band.PROBIT.view(uint32)` の出力をそのまま貼った。
- 未移植: `quantile` の q ≥ 1 (FUN_009efdd4 の最大値 `_vmax`。model/band では使わない。C++ は false)、`div < 0` (Python は ValueError、C++ は false)、SIMD = False の経路、`Work` (C++ は std::vector)。
- 分位点の「v ≤ t の個数」はソート済みの写しに upper_bound で数える。ソートは LSD 基数ソート 3 パスで q99 と q50 で共有。NaN の閾値のときは Python と同じく、n ≤ 4096 なら n (numpy searchsorted)、n > 4096 なら 0 (count_nonzero) を返す (§5 の 1)。

### 1.1 移植で効いた落とし穴 (他の N1 タスクにも関係する)
1. **clang -O2 は fast-math なしでも fadd / fmul の被演算子の順を入れ替える** (観測)。最初の版 (`dbg/nv_band.cpp.v1`) で NaN の payload を含む面の band_levels 3 件が不一致になった。箱フィルタのプレフィックス和 `S + d0` が、Python では S の NaN を残すのに C++ では d0 の NaN になっていた (`dbg/dbg.py` で段ごとに比べて特定)。LLVM の Reassociate パスが可換な演算の被演算子を順位で並べ替えるためとみられる (推定)。ARM の FPProcessNaNs は最初の被演算子の NaN を返すので、両方が NaN だと結果のビット列が変わる。対策として、両方の被演算子が NaN になり得る加算・乗算をインラインアセンブラの `A(a,b)` (= `fadd s, a, b`)・`M(a,b)`・`AD(a,b)` (double) で書いた。定数との積・二乗・減算 (非可換) はそのまま。ベクトル化は効かなくなるが、時間の大半は分位点 (§3) なので影響は小さい。
2. **numpy の float32 スカラー演算は NaN を逆の順で伝える** (numpy 2.1.3 / arm64 で観測)。配列の `a + b` は a の NaN を返すが、`np.float32` スカラーどうしの `a + b`・`a * b` は b の NaN を返す (`a - b`・`a / b` は a)。Python の `quantile`・`probit` はスカラーで書かれているので、C++ ではそこだけ `AS(a,b) = A(b,a)`・`MS(a,b) = M(b,a)` にした (`t = step + t`、補間の式、`g`、probit の和)。これで NaN の q や NaN のデータでも分位点が一致した (§5 の 2)。
3. `x * 1.0`・`fma(x, 1.0, c)`・`first * r` (r = 1) は LLVM が畳み込み、シグナリング NaN の静音化と NaN の優先順位が変わる。定数を `opaque()` (空のインラインアセンブラ) に通して実際の演算として残した。

## 2. 検証
`make -C native -f Makefile.band && python3 native/verify_band.py` の最終実行 (2026-10-26):
```
  ms    mean/std     : 1080/1080 bit-exact
  quant quantile     : 1080/1080 bit-exact
  rand  model_levels : 1665/1665 bit-exact
  rand  band_levels  : 1665/1665 bit-exact
  real  model_levels : 108/108 bit-exact
  real  band_levels  : 108/108 bit-exact
5706/5706 bit-exact
time: Python 15.4 s (4 procs), C++ 0.81 s (one process); nv_band_test: model 0.293 s, band 0.299 s, quantile 0.012 s, mean/std 0.000 s
```
全体 17.9 秒 (ビルド込み、負荷平均 6〜7)。不一致 0。

- 比較はすべて uint32 のビット列。model_levels は合格基準が語 0..54 だが、語 55 以降 (初期化パターン) の差も不合格に数えた (実際は 4096 語すべて一致)。band_levels は 4096 語。quantile は (成否, 結果のビット)、mean/std は 2 語。
- 乱数・境界 (`random_cases`): model / band 各 1665 面。
  - サイズ 68 通り: 1×1〜512×512、奇数・細長い面・1024×36。2048 値のブロックの境界 (72×40 → crop 64×32 = 2048 など)、分位点の n ≤ 49 経路と n > 4096 経路、縁の縮小の境界 (35×5・39×9 など) を含む。
  - 種類 26 通り: 正規・一様・有色・スパイク・直流・定数・ほぼ定数・直流 1e6・1e-30・非正規数・1e30・1e37〜3.4e38 (オーバーフロー)・整数・2 値・ランプ・段・負値・NaN・±inf・全面 NaN・−NaN・シグナリング NaN・payload 付き NaN・−0・混合・NaN 20% の密な payload。
  - ほかに乱数サイズ 40 個と、levels 1〜4 の面を一部含む。
- 実写 (`real_cases`): model / band とも 108 面。12 フレーム (test_v02 n=0/100/200/312/450/576/700、test.mp4 n=0/150/300/450/750) × 3 領域 (左上 128²・中央上寄り 128²・中央 320²) × Y/Cr/Cb (`nv_flat.transform(..., 1)`)。フレームは ffmpeg で rgb48le を切り出して /257 (`quality_draft._frame` と同じ)。`tmp/*.npy` にキャッシュ。
- 分位点・平均/標準偏差の直接照合: n = 0〜9000 × 40 回 (境界 49/50、2047/2048/2049、4095/4096/4097 を含む)。q ∈ {0.99, 0.5, 0.01, 0.9, 0.3, 0, −0.1, 0.999, 0.75, NaN (payload 3 種)}、flag ∈ {0,1,2,3}、div ∈ {0,1,4,7,100}。同点を含む。
- 感度試験 (`mutate.py`): 1 か所ずつ変えた 23 種のうち 18 種を検出 (不一致 11〜1802 件)。
  - 検出: 加算の順 (asm なし)、std の FMLA の有無 (レーン部・端数部)、`_reduce8` の順、ブロック長、3 タップ経路・プレフィックス和経路の無効化、n ≤ 49 の補間、小標本係数の添字、lim の下限、scalar 拡大の閾値、fac の clamp、CV のオクターブ数、down2 の NEON 列数、`_sub_m` の FMLA、縁の縮小の条件、外挿の CV の複写、スカラーの NaN 順 (`t = step + t`)。
  - 未検出の 5 種は出力に現れない等価な変更であることを確認: `count_nan` (NaN の閾値の個数。t は NaN のまま)、`up2_odd_col` (奇数列は縁 bx ≥ 2 の外)、`scalar_order_g` (g が NaN なら `g > t` が偽で t が選ばれる)、`probit_order` (両項は同じ NaN から来る)、`box_mode0_start` (r = 1 では first·2 と S = first が同じ NaN)。

## 3. 計測時間
`bench.py` (M2 Max、負荷平均 6.8〜7.0、1 プロセス、正規ノイズの面、1 面あたり)。

| 面 | Python model_levels | C++ | Python band_levels | C++ |
|---|---|---|---|---|
| 128² | 4.80 ms | 0.21 ms (22.5 倍) | 5.00 ms | 0.23 ms (21.8 倍) |
| 320² | 10.30 ms | 1.42 ms (7.2 倍) | 11.24 ms | 1.47 ms (7.6 倍) |
| 512² | 15.27 ms | 3.57 ms (4.3 倍) | 15.85 ms | 3.71 ms (4.3 倍) |
| 1920×1080 | 65.35 ms | 32.00 ms (2.0 倍) | 67.28 ms | 31.67 ms (2.1 倍) |

- 1080p の内訳 (`dbg/` の ctypes、最初の std::sort の版): 箱フィルタ 1.4〜2.4 ms、down2 0.3 ms、band 1.6 ms、mean/std 0.3 ms、cv 2.3 ms、分位点 1 回 49 ms。基数ソートを q99・q50 で共有して 136 ms → 32 ms。残りも分位点のソートが主とみられる (推定)。
- さらに速くする手 (未実施): asm の加算をベクトルの asm (`fadd v.4s`) にする、ソートの並列化。

## 4. Makefile.band のルール
```
CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra   (native/Makefile と同じ)
all: nv_band_test
nv_band_test: nv_band_main.cpp nv_band.cpp nv_band.h
	$(CXX) $(CXXFLAGS) -o $@ nv_band_main.cpp nv_band.cpp
verify: nv_band_test → python3 verify_band.py
clean: rm -f nv_band_test
```
- 警告 0 (-Wall -Wextra)。インラインアセンブラは `__aarch64__` のときだけ。それ以外では通常の演算に落ち、NaN の payload は保証しない。
- `verify_band.py` のオプション: `--quick` (約 330 件、3 秒)、`--no-real`、`--jobs N` (最大 4)、`--keep`、`--exe PATH`、`--cache` (Python の期待値を tmp に保存して再利用。変異試験用)。
- 中間ファイル (band_cases.bin / band_results.bin) は `sandbox/omo/wp_native_band/tmp/` に書き、最後に消す。
- 統合時の注意: `.gitignore` が `*` (許可リスト式) なので、native/nv_band.* などは今のままでは git に載らない (`git check-ignore` で確認)。

## 5. Python 側への不具合報告 (直していない)
1. `nv_band.quantile` の `count(t)` は NaN の閾値の扱いが n で食い違う: n ≤ 4096 は `np.searchsorted` で NaN の t に n、n > 4096 は `count_nonzero(v <= t)` で 0。t は一度 NaN になると NaN のままなので結果には現れない (感度試験の `count_nan` が等価だったことと整合)。変わるのは反復の回数だけ。C++ は両方をそのまま写した。
2. `quantile`・`probit` はスカラーの numpy float32 演算で書かれていて、両方の被演算子が NaN のとき後ろの NaN を返す (§1.1 の 2)。本家の fadd の被演算子の順と一致するかは本家との照合 (`nvh quantile` 933 件) の範囲でしか言えない。numpy の版やビルドが変わると NaN の payload が変わり得る。model_levels / band_levels では分位点が fmaxnm で 1e-6 に置き換わるので、NaN の payload は出力に出ない。
3. 不具合ではないが、`nv_band._sub_m` の「全部有限なら加算、それ以外は FMLA」の分岐は結果に影響しない (有限値では fma(x, 1, c) = x + c)。C++ は常に FMLA + 端数部の非融合で書いた。

## 6. 残る懸念
- 照合相手はこの機械の Python 移植 (numpy 2.1.3)。本家との直接照合はしておらず、本家との一致は Python 側が保証している範囲に限る。
- インラインアセンブラは AArch64 前提。x86 などでは NaN の payload の一致を保証しない (有限値は一致するはずだが未確認)。
- 速度は 1080p で Python の約 2 倍 (分位点のソートが主)。FindFlatArea (N1d) のように小さな面を多数測る用途では 7〜22 倍。

## 7. ファイル
- `native/nv_band.h`・`native/nv_band.cpp` — 移植本体
- `native/nv_band_main.cpp` — 照合用ドライバ。種別 0 model / 1 band / 2 quantile / 3 mean・std。標準エラーに部品ごとの時間
- `native/Makefile.band` — ビルド規則
- `native/verify_band.py` — 照合スクリプト
- `sandbox/omo/wp_native_band/mutate.py` — 感度試験、`bench.py` — 速度の計測
- `sandbox/omo/wp_native_band/dbg/` — 段ごとの比較 (ctypes、`capi.cpp`) と最初の版 `nv_band.cpp.v1`
- `sandbox/omo/wp_native_band/tmp/` — フレームのキャッシュ (約 177 MB)
