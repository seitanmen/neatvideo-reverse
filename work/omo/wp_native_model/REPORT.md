# wp_native_model 報告書 (N1e: BuildNoiseModel の 1 面モデル nv_model の C++ 移植)

日付: 2026-10-27。担当: N1e 作業エージェント (サブエージェント)。REPORT.md は作業者が書かなかったため、最終応答を統括が転記した。

- 範囲: N1e1/N1e2 には分けず、4096 語すべてを 1 回で移植した。族 A/R/dc、族 B/C、自己相似 FUN_00ad319c、FUN_00ad1ae0 を含む。
- 結果 (観測):
  - Python 版との照合は 1244/1244 件でビット一致した (不一致 0、警告 0)。
  - 本家のキャッシュ済みダンプとも 753/753 件で一致した (本家は実行していない)。
- 守ったこと:
  - repro/・neatvideo_ofx_614/・処理コア・AGENTS.md・native/Makefile には書き込んでいない (読み込みのみ)。
  - git の書き込み、本家バンドル・nvh の実行もしていない。

## 1. やったこと
`repro/nv_model.py` の `model_full` を C++ に移植した。部品の `nv_model_special.py` (`ad319c`) と `nv_model_bm.py` (`ad1ae0`) も含む。どちらも既定の FAST = True の経路を写した。

| C++ (`native/nv_model.cpp`、名前空間 `nvm`) | Python の移植元 |
|---|---|
| `model_full` / `level_info` / `borders` / `crop` / `q99` / `extrap` / `Pyramid` | nv_model の同名 |
| 族 A・R・dc (`robust_weight` = FUN_009ef0d0、`argmax_dirs` = FUN_009ee5fc、`dfilt`) | nv_model |
| 族 B/C (`up2` = FUN_00993df0、`blend_add_10` = FUN_009ed978) | nv_model |
| `make_kernel` / `general_filter` / `linefilt` (mode 1) | nv_lines.LineKernel / `_general` (N1b の nv_curve.cpp から写した) |
| `minmax` / `image_u8` / `box3_fused` / `local_rms` / `pattern_remove` / `select_greedy` / `offsets_at` / `block_sum32` / `block_std32` / `grid_cands` / `tile3` / `ad319c` | nv_model_special (`_fast` 版) |
| `make_table` (FUN_00acf420) / `window` / `pad_mirror` / `positions` / `sad_grid` / `best_direction_all` / `ratio_sums` / `norm_image` / `ad1ae0` | nv_model_bm (FAST 版) |
| `ratio_eps` (FUN_009ee04c) / `ramp` (FUN_009ee93c) | nv_pixnoise |

- **再利用**
  - N1a (`nvb`) から再利用したもの: `model_levels` (語 0..54)・`box_rows`・`box_cols`・`down2`・`band_from`・`quantile`・`mean_exact`・`std_exact`。
  - `nv_band.*` と `nv_curve.*` は変更していない。nv_curve の線カーネルは無名名前空間にあって外から呼べないため、コードを複製した。
- **演算と NaN の順序**
  - nv_model の fadd/fsub/fmul/fdiv と、`_nanfix` による FPProcessNaNs の規則は、インラインアセンブラ `A`/`S`/`M`/`D` (被演算子の順を固定) で再現した。
  - fma32 は `fmadd` のアセンブラ `FMA(a,b,c)` にした。NaN の優先は加数 c、次に a、b。
  - 定数 1.0/0.0 との積和 (blend_add、local_rms の 1.0×) は、コンパイラに畳ませないよう `opaque()` に通した。
  - FUN_009ef0d0 と FUN_009ee04c は、先頭 n&~3 要素を FRECPE+FRECPS (`vrecpes_f32`/`vrecpss_f32`)、端数を真の除算にした。
  - ramp は、NEON 部を FMA → FMIN → FMAX (NaN 伝播)、端数部を fcsel の模倣にした。
  - minmax は、レーン 8 本 + FMIN/FMAX で結合し、端数部をスカラーで処理する。
- **libSystem の関数**: `atan2`・`__sincos_stret`・`tanf` は、Python が ctypes で呼ぶのと同じ macOS libSystem の関数を volatile 関数ポインタ経由で呼ぶ。
- **定数**: すべて Python のビットパターンから `bits(0x...)` で写した。
  - 例: 1.1 = 0x3f8ccccd、3.8989797 = 0x407988e2、0.85 = 0x3f59999a、1/9 = 0x3de38e39、DAT_012445a8 = {0x3f87ae14, 1×4}。
- **戻り値**: `nvm::model_full(plane, W, H, set_, out[4096], levels)` は状態を返す。照合では状態も比べる。今回の入力で Python が状態 ≠ 0 になった例はない。

  | 状態 | 意味 |
  |---|---|
  | 0 | OK |
  | 1 | NotImplementedError 相当 (FUN_00ad11b0 で条件を満たすビンがない) |
  | 2 | ValueError 相当 (辺が 2..512 の外か、levels が 1..5 の外) |

## 2. 作ったファイル
- `native/nv_model.h`・`native/nv_model.cpp`: 移植本体。
- `native/nv_model_main.cpp`: 試験ドライバ `nv_model_test`。
  - `nv_model_test in out`: 一括照合。入力は {W,H,set,levels,float[W*H]}、出力は {status, 4096 語}。
  - `nv_model_test time in [reps]`: 計時。
- `native/Makefile.model`: `nv_model_test: nv_model_main.cpp nv_model.cpp nv_band.cpp`。CXXFLAGS は native/Makefile と同じ。`verify`・`clean` あり。
- `native/verify_model.py`: 照合スクリプト。オプションは `--quick`・`--no-real`・`--jobs N` (3 以下)・`--exe`・`--genuine`・`--only-genuine`。
- `sandbox/omo/wp_native_model/` (作業用)
  - `mutants.py`・`mutants.log`: 感度試験。
  - `bench.py`: 速度の計測。
  - `quick.py`・`find_tail.py`: デバッグ用。
  - `tmp/` (約 42 MB): `pycache/` (Python の結果のキャッシュ 1244 件)、`run1.log`・`final.log`・`bench.log`・`mut/` (変異体のビルド)。

## 3. 検証結果
最終の実行 (2026-10-27) は `make -C native -f Makefile.model && python3 native/verify_model.py --genuine`。クリーンビルドで警告 0。

```
  levels set -1  : 8/8      levels set 0-4 : 8/8
  rand   set -1  : 148/148  rand   set 0-4 : 708/708
  real   set -1  : 56/56    real   set 0-4 : 280/280
  tail   set -1  : 6/6      tail   set 0-4 : 30/30
1244/1244 bit-exact (218 planes x sets; 254 runs with NaN in the input; 5095424 words compared)
genuine (cached nvh dumps): 753/753 bit-exact (0 cases skipped)
```

- **比較の方法**: 状態と 4096 語すべてを uint32 で比べた。初期化パターンと未使用語も含む。
- **乱数・境界の面 (rand)**: 147 面 × set_ = −1, 0..4。
  - サイズ (36 通り)
    - 小さい面: 2×2・3×5・8×8・9×9
    - 族 A の境界: 16〜24
    - dc/B の境界: 25〜31
    - 族 C と FUN_00ad319c の境界: 28・37・44・45・48
    - タイル化の境界: 88×88・89×89・89×88・88×89
    - 奇数サイズ: 97×61・101×77・133×97・257×255・77×301
    - 最大は 320²
  - 種類 (30 通り)
    - 分布: 正規・一様・有色・方向のある縞・周期パターン (自己相似の検出を通す)
    - 形: スパイク・直流・定数・ほぼ定数・ランプ・段・2 値・整数・負値
    - 大きさ: 1e6 の直流・1e-30・非正規数・1e30・オーバーフロー
    - NaN・inf: NaN・±inf・全面 NaN・−NaN・シグナリング NaN・payload 付き NaN・密な NaN・NaN の塊
    - その他: −0・混合・周期 + 特殊値
  - 周期パターンの面 8 枚 (96²・128²・45×60・180×140) は set −1/4 のみ。
- **levels**: levels を 1〜4 にした 8 面 (set −1/4)。
- **tail**: 最後の画素の近くに NaN/inf を置いた 6 面 (W·H % 4 ≠ 0)。
- **実写 (real)**: 8 フレーム × 7 面 × 6 組。
  - フレームは test_v02 n=0/100/312/576 と test.mp4 n=0/300/450/750。N1a のキャッシュ (`sandbox/omo/wp_native_band/tmp/*.npy`、rgb48le/257) を読み込みのみで使った。
  - 7 面の内訳は、128² の Y/Cr/Cb (`nf.transform(…,1)`)、192² の R/G/B、101×77 の Y。
- **本家キャッシュとの照合 (`--genuine`)**: 753/753 一致。
  - 方法: `model_verify/cases.py` の全ケースを読み込みのみで生成し (フレームは model_verify/data から読むだけ)、`model_verify/tmp/gcache` にある本家の `nvh model` ダンプ 753 件と比べた。
  - 対象には 512² や 1 行に近い面、synth・bound・fuzz の各群も含む。
  - これは Python を介さず、C++ が本家とビット一致したという観測である。

## 4. 感度試験 (`sandbox/omo/wp_native_model/mutants.py`)
nv_model.cpp の写しに誤りを 1 か所ずつ入れて別にビルドし、Python のキャッシュに対して照合した。**21 種中 16 種を検出**した。

**検出した 16 種**

| 注入した誤り | 不一致の件数 |
|---|---|
| FUN_009ef0d0 で FRECPE+FRECPS の代わりに真の除算 | 565 |
| FUN_009ee04c で FRECPE+FRECPS の代わりに真の除算 | 150 |
| 集約 (FUN_00ad15f0) の FMLA を非融合に | 60 |
| ブロック標準偏差 (FUN_009ef664) の FMLA を非融合に | 54 |
| argmax の ≥ を > に | 27 |
| crop を 1px 広げる | 565 |
| DAT_012445a8[0] = 1.0 | 565 |
| u8 変換の丸めを floor に | 216 |
| select_greedy で最後の最小を選ぶ | 84 |
| pad_mirror を symmetric に | 224 |
| FUN_00acf420 の隣接ビンなし | 284 |
| 外挿の係数を 0.5 に固定 | 604 |
| sad_grid の範囲判定 (位置ごと) を < に | 224 |
| 端の列を「1/cnt の乗算」に | 128 |
| up2 の奇数列の複写元を 1 列ずらす | 95 |
| タイル化時の閾値 0.15 を 0.1 に | 56 |

**検出しなかった 5 種**: いずれも出力に現れないと判断した (推定を含む)。

| 変異 | 検出しなかった理由 |
|---|---|
| blend_nofma | fma(B,1,0) と (B·1)+0 は、−0 と NaN を含めて同じビットになる (等価) |
| box3_last | 片方が非 NaN なら、x+0 と 0+x は同じビットになる (等価) |
| sad_bound | 変えたのは「位置を全部飛ばすか」の前判定だけで、位置ごとの判定は変わらない。強い版の sad_bound2 は検出した |
| lines_order | 変わるのは、走査和の立ち上がりで両方 NaN のときの payload の選び方だけ。q99 は fmaxnm を通るので、payload は出力に出ないとみられる |
| robust_tail | 端数部で NaN の z を「> 1」側に入れる変更。336 面で探索したが、C++ の変異体と正規版が一致した (`find_tail.py`)。z が NaN で a が有限になる画素が最後の n&3 画素にできても、その影響は crop の外に留まるとみられる。実質等価と推定するが、証明はない |

## 5. 計測時間
条件: M2 Max、負荷平均 8〜9 (他の作業者と並行)、1 プロセス。C++ は 3 回の最良値、Python は 1 回 (`bench.py`、`tmp/bench.log`)。

| 面 | set | Python (秒) | C++ (秒) | 倍率 |
|---|---|---|---|---|
| 128² 正規 | −1 / 4 | 1.96 / 1.35 | 0.152 / 0.132 | 12.9 / 10.2 |
| 128² test_v02 n=312 の Y | −1 / 4 | 1.76 / 1.33 | 0.148 / 0.129 | 11.9 / 10.3 |
| 192² 正規 | −1 / 4 | 2.34 / 1.85 | 0.235 / 0.190 | 9.9 / 9.7 |
| 192² test_v02 の Y | −1 / 4 | 2.74 / 2.35 | 0.280 / 0.238 | 9.8 / 9.9 |
| 320² 正規 | −1 / 4 | 5.11 / 4.11 | 0.608 / 0.485 | 8.4 / 8.5 |
| 320² 周期 | −1 / 4 | 5.67 / 4.68 | 0.681 / 0.551 | 8.3 / 8.5 |
| 320² test_v02 の Y | −1 / 4 | 6.83 / 6.51 | 0.834 / 0.717 | 8.2 / 9.1 |

- 照合全体の時間
  - C++: 1244 件で 55.7 秒。
  - Python: 3 プロセスで計 892 秒 (キャッシュなしの初回)。
  - 本家キャッシュの 753 件: C++ で 101.5 秒 (512² を多く含む)。
- 速さは約 8〜13 倍に留まる。浮動小数点演算をすべてスカラーのインラインアセンブラにしたため、ベクトル化が効かない。最適化はしていない。
- 改善するなら、nv_curve のような 4 レーンのアセンブラ (`fadd v.4s`) か、線フィルタ・箱フィルタのレーン並列化 (推定)。

## 6. Python 側への不具合報告・注記 (直していない)
不具合は見つからなかった。Python の結果は、C++ を介して本家キャッシュ 753 件と整合している。

注記 (いずれも C++ は既定の FAST 経路に合わせた):
1. `nv_model_special._grid_ref` (参照経路) の `s8 = F32(s8 + F32(sd*sd))` は numpy のスカラー演算なので、両方が NaN のときは第 2 被演算子の NaN を返す。
   - FAST 経路は `nm.fadd(s8, …)` (第 1 優先) なので、両者は NaN の payload で食い違い得る。
   - rms はその後 `fdiv`/比較にしか使わないので、出力への影響はないとみられる (推定)。
2. 同じ参照経路の `best_direction` はスコアを `& 0xffffffff` で切り詰める。FAST 経路の `best_direction_all` は int64 のまま比べる。照合の範囲では差は出ていない。
3. `image_u8` の `off = s * (-mn)` は numpy のスカラー演算 (第 2 被演算子の NaN)。ただし NaN のときは全画素が 0 になり、payload は結果に出ないので、C++ は普通の float 演算で書いた。

## 7. 未移植・未検証
- **非 AArch64 は未対応**
  - FRECPE/FRECPS のソフトウェア版を入れていないので、例外を投げる。必要なら nv_curve.cpp の `frecpe_soft`/`frecps_soft` を流用できる。
  - NaN payload の一致は AArch64 前提。
- **Python も例外になる経路は書いていない**: 到達しない経路なので、C++ も例外にした。
  - `pad_mirror` で p ≥ 辺の場合 (エッジ複製の分岐)
  - `box3_fused` の分離型フォールバック (W < 4 か H < 2)
  - `up2` の Wc < 2
- **NotImplementedError 経路は未検証**: C++ は状態 1 を返すが、照合の入力で Python 側がこの経路に入った例はなく、一致は確かめていない。
- **set_ > 4**: Python は IndexError、C++ は状態 2。照合していない。
- **線カーネルの重複**: nv_curve.cpp と nv_model.cpp に同じコードがある。今回は nv_curve を変えないため複製した。統合時に共通化を検討する。
- **キャッシュ**: 線カーネルのキャッシュはプロセス内の静的な map (mutex 付き) で、上限がない。
- **スレッド**: スレッド並列化はしていない。
