# N1b: EstimateNoiseLevelCurve の C++ 移植 (nv_curve) — 作業報告

作成: 2026-10-26 (N1b ワーカー = サブエージェント)。報告ファイルは書けなかったため、最終応答を統括が転記した。
指示: `.claude/tasks/native_n1_split.md` の「共通規約」「検証の合格ライン」「N1b」と、統括の追加の約束。
コミットはしていない。repro/・処理コア・native/Makefile・AGENTS.md は変更していない (読み込みのみ)。本家バンドル・nvh・RS バイナリは使っていない。

## 1. 成果物
| ファイル | 内容 |
|---|---|
| `native/nv_curve.h` | API: `genuine_noise_curve(rgb, W, H, stride_c, CurveResult*, threads)` (xs/ys の 8 点と診断用のビン別 sums・counts)、`is_clip_genuine(v, n)`、`CurveInterp` (= `noise_at` の補間器)、`pixnoise` (1 タイル、検証用)、`tile_layout` |
| `native/nv_curve.cpp` | 移植の本体 (nv_curve.py + nv_pixnoise.py + nv_lines.py を 1 ファイルに) |
| `native/nv_curve_main.cpp` | 検証用ドライバ `nv_curve_test`。サブコマンド curve / clip / interp / tile / time / recip-selftest (書式はファイル先頭のコメント) |
| `native/Makefile.curve` | `nv_curve_test: nv_curve_main.cpp nv_curve.cpp nv_curve.h` → `$(CXX) $(CXXFLAGS) -o $@ nv_curve_main.cpp nv_curve.cpp -lpthread`。verify / clean。CXXFLAGS は native/Makefile と同じ |
| `native/verify_curve.py` | Python とのビット比較 (verify_quality.py と同じ形式)。環境変数 `NV_CURVE_EXE` で別のバイナリ |
| `mutants/run_mutants.py`・`mutants.log` | 感度試験 |
| `tmp/` | 実写フレームのキャッシュ (`*_n*.npy`、rgb48le / 257)、計時用 `f312.bin`、ログ (`run*.log`・`portable.log`)、作業途中の控え `nv_curve_before_asm.cpp` (129 MB) |

## 2. 移植の構成と判断
- 写したのは Python の参照経路 (tile_histogram → clip_weight / luma_601 / nv_pixnoise.pixnoise / nv_lines.linefilt)。比較の相手は既定の genuine_noise_curve (batch=True の高速経路) で、両経路は同じビットを出した。C++ で変えたのは計算の形だけ (箱フィルタを複数レーン (列と転置した行) で同時に回す、斜めの線フィルタは nv_lines の `_general_bm` と同じくエントリ優先の表を作り線の末尾を −0.0 で埋める (S + (−0.0) は S とビット同一))。演算と順序は参照経路と 1 対 1。
- タイル並列 (threads > 1): タイルごとに採用した画素 (ビン番号と float32 のノイズ値) を保持し、double の加算はスケジューラの順・タイル内は行優先で 1 本に行う。-j 1 と -j 4 は同じビット (検証で両方を比較)。
- frecpe + frecps (FUN_009ee04c = ratio_eps): Python 実装どおり、先頭 n & ~3 要素は FRECPE + FRECPS 1 段 (`a * (est * step)`)、残りの n & 3 要素は真の除算 `a / (b + eps)`。AArch64 では vrecpeq_f32 / vrecpsq_f32、それ以外 (と -DNVC_PORTABLE) ではソフトウェア版 frecpe32 / frecps32。両者は `nv_curve_test recip-selftest 4000000` で 4,000,441 組 (特殊値・非正規数・乱数) 差 0。**FRECPE/FRECPS の再現は必須** (真の除算で代用すると感度試験で 454 曲線中 92・25 タイル中 24 が不一致)。
- NaN のビット列 (統括の参考情報 1〜3 と同じ落とし穴を独立に踏んだ):
  1. clang -O2 は fast-math なしでも可換な fadd / fmul の被演算子を入れ替える。ペイロードの違う NaN が密に並ぶ合成入力 3 件で実際に不一致。画素処理の加減乗算をすべてインラインアセンブラの FA / FS / FM (スカラー版と 4 レーン版) にして Python と同じ順に固定。積和 (luma・ramp) は fmaf / vfmaq_f32 のまま (加数は入れ替わらず、乗数の入れ替えは片方が定数なので無害)
  2. est·step の順: NaN の d に対し est は +NaN、step は −NaN (FRECPS が op1 の符号を反転)。積の順が出力の符号を決める (感度試験で検出)
  3. 曲線段 (double) の NaN の選ばれ方は Python 側の値の型で違う (numpy 2.1.3 / Python 3.13.5、arm64 で観測): CPython の float 演算・numpy の配列演算・np.add.at は第 1 被演算子の NaN、numpy の float64 スカラーどうしの + と * は第 2 (− と / は第 1)。smooth_curve の `(m[i] + (m[i-1] + m[i+1]) * 0.5) * 0.5` は numpy スカラーなので第 2 優先の add2、それ以外の曲線段 (polyfit / _det / polyval / CurveInterp / 集計) は第 1 優先の add1 / mul1 (どちらも NaN を明示的に選ぶ関数)
  4. libSystem の pow と tanf は volatile な関数ポインタ経由で呼ぶ (定数畳み込みや pow(x,2) → x*x を防ぐ。Python も ctypes で同じ関数を呼ぶ)
- 定数はすべて Python 側の float32 ビットパターンから `bits(0x...)` (例: 0.33 = 0x3ea8f5c3、1e-4 = 0x38d1b717、π/4 は double の 0x1.921fb54442d18p-1)。
- IsClip: 最小・最大 (先頭 n & ~7 要素は NaN 伝播、端数部は NaN を無視、開始値 ±FLT_MAX)、nb = fcvtzs(range·8 + 1) の飽和、ビン番号の frintn、最頻ビン (同数なら小さいビン)、過半数または片側 10 倍の条件。bincount + argmax と同じ値を O(n) で求める。
- 移植していないもの: 取り消し (cancel)、nbins ≠ 8、斜めカーネルのモード 0 (FUN_00998d98 は使わない)、2px 未満のタイルとスカラー拡大の Wc < 2 (Python と同じく例外)、48px 未満の画像 (Python は ValueError、C++ は false)。

## 3. 検証 (`make -C native -f Makefile.curve && python3 native/verify_curve.py`)
```
curve: 8 real frames + 19 synthetic + 520 fuzz images
  C++ -j 1: 8.3 s for 547 images (Python, 4 procs: 16.0 s)
  C++ -j 4: 2.7 s for 547 images (Python, 4 procs: 16.0 s)
curve: 1094/1094 curves bit-exact (xs + ys), histograms 1094/1094 (178 cases with NaN in ys)
clip: 4000/4000 IsClip values bit-exact
interp: 20554/20554 queries bit-exact (640 tables)
tile: 25/25 pixnoise tiles bit-exact (noise, weight, bright)
total 40.7 s: ALL BIT-EXACT
```
(ログは `tmp/run5.log`)
- curve: 547 画像 × (-j 1, -j 4) = 1094 件。合格判定は xs・ys の 8 点 (double) の 64 ビット比較。ビン別の sums・counts も全件一致。
  - 実写 8 フレーム (全面): test_v02 n=0/100/312/576 (1920×1080)、test.mp4 n=0/300/450/750 (640×360)。ffmpeg の rgb48le / 257 (quality_draft._frame と同じ)
  - 合成 19 件: 勾配 + ガウスの 7 サイズ (1920×1080 / 640×360 / 97×61 / 48×48 / 101×67 / 333×211 / 49×50)、黒帯、白飛び、平坦 128、×1e18、1e20 のブロック、NaN / ±inf のブロック、孤立した 1e13〜1e20 の画素、4096×2160 の勾配、NaN ペイロード (±、quiet / signalling) を 40 画素に散らしたもの、ペイロードの違う NaN が密に並ぶ領域 3 件
  - 乱数 520 件 (48〜260px): analysis/noise/verify_curve.py の --fuzz と同じ生成器 + ガウス・1e13〜1e20 の画素・NaN ペイロード・NaN/巨大値のブロック・非正規数。奇数番は有限。ys に NaN を含むのは 178 件
- clip: 4000 面 (16×16 ブロック 2000、0〜700 要素の任意長 1200、端数部の境界の長さ 0/1/2/7/8/9/15/16/17/255/257 が 800)。値はクリップ・狭い値域・量子化・2 値・一様・巨大な値域・定数・1 値 + 裾に特殊値を散らしたもの
- interp: 640 表 (上の曲線 40 + 乱数表 600: n=2〜8、NaN の節点・値、重複した節点、ペイロードの違う NaN)、計 20,554 問い合わせ (節点・中点・範囲外・±inf・NaN)
- tile (診断用): 25 タイル (実写 3 フレームのタイル 18、合成 7: 120×108、97×61 (NaN と 1e20)、50×46、48×120、64×33、10×9、8×8) の noise / weight / bright
- 移植可能版 (-DNVC_PORTABLE。NEON もインラインアセンブラも使わず、FRECPE/FRECPS・FMIN/FMAX・NaN の規則をソフトウェアで模倣): 同じ全件で ALL BIT-EXACT (`tmp/portable.log`)。ただし x86_64 は既定の NaN が 0xffc00000 なので、x86 で NaN を含む入力のビット一致は保証しない

### 感度試験 (`mutants/run_mutants.py`。各回 454 曲線 + 1500 IsClip + 90 表 + 25 タイル)
| 注入した誤り | 結果 |
|---|---|
| ratio_eps の NEON 部を真の除算に | 検出 (曲線 92・タイル 24) |
| IsClip のビン番号を trunc に | 検出 (IsClip 205) |
| 行の箱フィルタで 4 画素のプレフィックス和をやめる | 検出 (曲線 266) |
| 輝度の FMA をやめる | 検出 (曲線 264) |
| ramp の NEON 部を非融合に | 検出 (曲線 38) |
| smooth を第 1 被演算子の NaN 規則に | 検出 (曲線 6) |
| est·step の順を逆に | 検出 (タイル 1) |
| 箱フィルタの S + x の順を逆に | 検出 (曲線 2) |
| double の和を float に | 検出 (曲線 268) |
| 線の末尾の詰め物を +0.0 に | 未検出。等価 (走査和 S は +0.0 から始まり −0.0 にならない) |
| F_i = f + g の順を逆に | 未検出。ほぼ等価とみている (証明なし。f が NaN の画素では E_i も NaN で採用されず、g も同じ NaN から作られる) |

## 4. 計測時間 (M2 Max、test_v02 n=312 の 1920×1080 1 フレーム、負荷平均 4.3〜5.6)
| 実装 | 時間 |
|---|---|
| C++ -j 1 | 0.484〜0.494 秒 (5 回) |
| C++ -j 4 | 0.127〜0.131 秒 (5 回) |
| Python の既定 (batch) | 0.693〜0.792 秒 (3 回) |
| Python の参照経路 (batch=False) | 11.6 秒 (1 回) |
- 計測: `native/nv_curve_test time sandbox/omo/wp_native_curve/tmp/f312.bin [-j 4] 5`。参考: AGENTS.md §6.5 の本家 (1 スレッド) は 0.44 秒。時間の大半は斜めの線フィルタ (表の gather / scatter) と箱フィルタの走査和

## 5. Python 側への不具合報告・注記 (直していない)
- 不具合は見つからなかった。docstring と実装の食い違いもなかった。
- smooth_curve は numpy の float64 スカラー (`m[i]`) で計算しているので、両隣が別ペイロードの NaN のとき第 2 被演算子の NaN が残る。これは numpy のビルドによる性質で、本家 (AArch64 の命令順) と一致する保証はない。本家との照合ではペイロードの違う NaN が隣り合う入力は含まれていないとみられる (推測)。本家に合わせる必要が出たら smooth の 2 か所を add1 に変えればよい。
- 画素処理の「配列 × スカラー」では numpy はスカラー側の NaN を残す (numpy 2.1.3 で観測) が、スカラーはすべて定数で NaN にならないので結果に影響しない。

## 6. 残る懸念
- 被演算子の順の固定は -O2 で確認。他の最適化レベルと LTO では未確認。
- std::fma / vfmaq_f32 の乗数どうしの入れ替えは、片方が定数なので無害と判断 (検証で不一致なし)。
- signalling NaN が演算を経ずに比較・選択だけに使われる経路は IsClip の最小・最大だけで、値は 1.0 に決まりペイロードは出力に出ない。
- ホストが本家を複数スレッドで動かすと本家の ys の末尾ビットが違い得る (Python 版と同じ制限、AGENTS.md §7)。C++ の threads は加算順を変えないので Python とは一致する。
