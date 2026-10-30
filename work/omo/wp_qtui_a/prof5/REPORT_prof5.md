# prof5 REPORT: Build Profile の残りの C++ 移植

Build Profile のうち Python に残っていた部分を C++ に移した。中身は build_profile_v5 の残り (FindFlatArea の分岐と find_flat_area_gradient を含む)、noisiest_frame / hh1_sigma、effective_profile、_wn_band_std。
C ABI `native/nvcore_c_prof5.h` から呼べ、Python とすべての値がビット単位で一致した。np.polyfit も一致したので、OpenBLAS の同梱は要らない。
コミットはしていない。AGENTS.md・MIGRATION.md・repro/・既存の native ファイルは変更していない。REPORT ファイルは作業者の制約で書けなかったので、この報告を `sandbox/omo/wp_qtui_a/prof5/REPORT_prof5.md` に転記してください。

## 照合結果 (観測)
| スクリプト | 結果 | 内容 |
|---|---|---|
| `native/verify_prof5.py --jobs 2` | 630/630 一致 (54 s) | 下の 4 群 |
| ・polyfit | 46,600 系 | `_fill_curve` に現れる全部分集合 × 次数 × 7 種の値の分布 |
| ・synth | 490 件 | 10 サイズ × 7 種の合成画像 × 7 通り (mode・area・more)。4:2:0 / 4:4:4、full range、3 種の行列、-j 1 / 4 |
| ・real | 63 件 | test_v02・test.mp4 の実フレーム |
| ・small | 76 件 | wn・候補選び・hh1・取り消し・effective_profile |
| `native/verify_bridge_prof5.py --jobs 2` | 110/110 一致 (42 s) | backend の native ON 経路と比較。build_profile 70、noisiest 12、hh1 4、effective_profile 24 (AFT 後の profile を含む) |
| `make -C native verify` の 11 本 | すべて一致 | 回帰なし、警告 0 |

- 通った経路: flat_status は 0 / 1 / 2 / 3 / None。mode は auto / spatial / temporal / spatial+noise-free、sub は 1 / 2。状態 4 (クリップ) の画像は作れておらず、未照合。
- 感度試験: polyfit の演算順を 5 通り変えると、すべて検出できた (不一致 3685 / 16 / 2809 / 2795 / 160 件)。

## C ABI (`native/nvcore_c_prof5.h`)
出力の配置、Python の dict のキー順と型はヘッダに書いてある。
```c
#define NVC_P5_I 16
#define NVC_P5_D 72
typedef int (*nvc_p5_cancel_fn)(void);
int nvc_build_profile_v5(const float* const* planes, int H, int W, int Hc, int Wc, const float* const* pair,
                         int n_more, const float* const* more, int full_range, int matrix, int mode, int levels,
                         int area_n, const int* area, int threads, nvc_p5_cancel_fn cancel, int* out_i,
                         double* out_d);   /* 0 ok, 5 取り消し, 負 エラー */
int nvc_wn_band_std(int levels, double* out);
int nvc_hh1_sigma(const float* Y, int H, int W, double* out);          /* 1 = 値あり, 0 = None */
int nvc_noisiest_candidates(int n, int* out);                           /* 候補の数 (最大 32) */
int nvc_effective_curve(const double* centers, int n, const double* curve, const double* factors,
                        const double* tune, double area_level, const double* tot_ratio, double* curve_out,
                        double* sigma_out);                              /* 0 = base のまま, 1 = 新しい値 */
```
- **引数**: mode は 0 auto / 1 spatial / 2 temporal。matrix は 0 bt709 / 1 bt601 / 2 bt2020。area_n は 0 = 自動探索 (取り消すと 5 を返す)、3 = 正方形、4 = 長方形。
- **UI 側の作業**:
  - `ui_frame` は付けない。backend と同じく UI が足す。effective_profile の `ui_factors` / `ui_tune` も UI が足す。
  - Most Noisy Frame は 2 つの関数を組み合わせる。候補ごとに取り消しを確かめ、`nvc_hh1_sigma` で点数を出す。初期値 -1.0 で、最大の最初の候補を選ぶ。float32 に直すのは候補のフレームだけでよい。

## np.polyfit の再現
- 新規の `native/nv_lstsq.{h,cpp}` で、OpenBLAS 0.3.29 の dgelsd を写した。対象は `_fill_curve` が通る経路 (m ≥ n、n ≤ 25、右辺 1 本) だけ。
- 演算順は anaconda の libopenblas を逆アセンブルして決めた。gfortran が FMA にした箇所と、neoversen1 の BLAS カーネル (dnrm2・dgemv・dger・drot・dscal) の順を合わせた。dgemm の small kernel はこの CPU では使われない。
- 参考: 同じ呼び方で Accelerate の dgelsd を使うと、9068/9320 件が不一致だった。

## 今回わかった numpy の挙動 (観測)
- **配列全体の和・平均 (axis=None)**: float32 は 8192 個ずつまとめて足される。非連続なビューも C 順に平らにしてから 8192 個ずつ。最初に `_wn_band_std` が合わなかった原因はこれで、直して一致した。
- **末尾の軸に沿った mean**: ブロックごとに pairwise 和を取ってから割る。割り算は double で行い、float32 に戻す。
- **percentile**: q も添字も float32 で計算される。
- **noise_curve_temporal**: 差分 d は float64 になる。理由は `np.sqrt(2.0)` が強い型だから。

## 速度 (1080p、test_v02 f=6、負荷平均 4〜6)
| 条件 | backend (native ON) | C 1 スレッド | C 4 スレッド |
|---|---|---|---|
| auto・探索あり | 1.22 s | 1.42 s | 0.70 s |
| spatial・探索あり | 0.85 s | 1.20 s | 0.48 s |
| auto・手動領域 216×512 | 0.81 s | 0.55 s | 0.37 s |

4 スレッド版は探索と 3 フレーム分の空間曲線を並列にしている。スレッド数を変えても結果は同じ (照合済み)。

## 未解決・注意
- 照合はこの環境が前提: numpy 2.1.3 / scipy 1.15.3 / anaconda の OpenBLAS 0.3.29 (neoversen1 カーネル)。numpy 側の BLAS や CPU が変われば、Python 側の結果が変わりうる。
- NaN や inf を含むフレームは照合していない。UI のフレームは uint8 由来なので有限。
- C 側には FindFlatArea のキャッシュがない。結果は同じで、同じフレームを再度探すと遅いだけ。
- dgelsd の未移植の経路では `polyfit` が false を返す。`_fill_curve` からは到達しない。

## ファイル
`native/` の新規ファイル:
- `nv_prof5.{h,cpp}`、`nv_lstsq.{h,cpp}`、`nvcore_c_prof5.{h,cpp}`: プレースホルダだったものに中身を入れた。
- `verify_prof5.py`、`verify_bridge_prof5.py`: 照合スクリプト。後者は `run(tmp, jobs, quick) -> (ok, total)` を公開しているので、verify_bridge.py への組み込みをお願いします。

作業記録は `sandbox/omo/wp_qtui_a/prof5/` にある。主なもの:
- `cmp_lsq2.py`: polyfit の照合
- `mut/`: 感度試験
- `probe_sum.py`: 和の順序の調査
- `coverage.py` / `bench.py`: 経路の集計と速度
- `dl_src/` / `dis/`: 取得した LAPACK ソースと逆アセンブル
