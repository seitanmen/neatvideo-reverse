# 独立検証 VERDICT: ネイティブ N1i AutoFineTune (nv_aft) (2026-10-28)

独立検証者 (verifier サブエージェント) の最終応答を統括が転記した。

## 検証報告 (N1i AutoFineTune C++ 移植)

### 判定
**状態**: PASS → **APPROVE**(確信度: 高)
**阻害要因**: 0 件

自分で作った新しい入力すべてで、C++ と Python がビット単位で一致しました。作業者のキャッシュは使っていません。

### 確かめた内容

| 項目 | 結果 | 方法 | 出力 |
|---|---|---|---|
| ビルド | 成功 | 規約のフラグで `bin/` に出力 | 終了コード 0、警告 0 (`build.log` は 0 バイト) |
| 実写フレームの AFT | 成功 | `gen_expected.py` → `run_compare.py` | -j 1・-j 2 とも 16/16 一致 (状態、0xc300 バイト全体、out3) |
| 追加の AFT | 成功 | `extra_cases.py`・`mode2_skip.py` | 3/3 と 1/1 (diag と bins も一致) |
| 色の結合部 (実写) | 成功 | C++ の AFT 出力を C++ combine に渡し、本物の `apply_aft` と比較 | 6/6 |
| 色の結合部 (合成) | 成功 | `check_combine_syn2.py` | 198/198 |
| タイル順 | 成功 | 自作ドライバ `my_shuffle` | 41/41 |
| 保護対象 | 空 | `git status --short repro/ neatvideo_ofx_614/` と `git diff --stat repro/` | どちらも出力なし |

### 新しい入力と結果 (観測)

**実写フレーム**(作業者が使っていないもの)
- test_v02 n=30・200・640 (1080p)、test.mp4 n=120・600。
- 抽出は rgb24 と rgb48le/257 を混ぜ、自分で ffmpeg を呼びました。
- 本番と同じ経路で通しました: yuv420p → `build_profile_v5(spatial)` → `apply_aft`。`apply_aft` の中の `build_profile` と `auto_fine_tune` は、出力を取り出すために包んだだけで、差し替えていません。
- 領域は FindFlatArea が選んだもの: (832,0,128²)、(824,0,128²)、(1128,904,64²)、test.mp4 は 48²。
- 長方形の領域 (40,300,96,160) も 1 件入れました。
- mode 0 が 6 件、mode 1 が 1 件 (v02 n=200)、mode 2 が 1 件 (t_600)。

**合成入力**
- W=49 (タイル 1 列)、H=47 (状態 TOO_SMALL)、50×50 の mode 2。
- 飽和・NaN/inf・定数ブロックを含むもの (mode 0 と 1)。
- image2 あり、取り消し k=3 (状態 CANCELLED)、全面が定数。
- t_120 に、NaN/inf を混ぜた image2 を付けたもの。

**mode 2 の飛ばし経路**
- 平坦な白色ノイズ 512×320 で、「3 つのビンがすべて 32 個に達したら残りのタイルを飛ばす」判定を発火させました。
- 127 タイルが飛ばされ、-j 1・-j 2 とも diag・bins・out3 が一致しました。

**色の結合部 (合成 198 件)**
- 補償の値が 0 でない有限値になる和を 345 個含めました。
- q99 に別々のペイロードの NaN が 2 つ以上ある場合を 20 件、±inf・±0・非正規数を入れました。
- 曲線の語には負値・NaN を入れました。
- 中心の数を 9〜20 にした場合と、area_level・tot_ratio が NaN の場合も入れました。

**タイル順**
- N は 0〜2^20 から選び、4096 を超えるものも含めました (8K の 32400 など)。
- 照合の相手は Python 移植です。本家と照合されているのは N ≤ 4096 だけです。

### 陰性対照

| 対照 | 結果 |
|---|---|
| 入力の画素を全体で ×(1+2⁻²⁰) | 検出 (29 バイト違う) |
| プロファイルの R 面の語 0 を +1 ulp | 検出 (8 バイト違う) |
| G の 1 画素だけを +1 ulp | 不検出。ただし Python 側も bins・プロファイルとも変わらず、出力に影響しない入力でした。照合が空振りしているわけではありません |
| C++ の Neumaier 加算の順を `AD(x, denom)` に入れ替えた版 | 13 件の不一致で検出 |
| sigma の積の順を `MD` に入れ替えた版 | 26 件の不一致で検出 |

### 作業者が「未確認」とした点の判定 (観測)
- **numpy の NaN の選び方**: numpy 2.1.3 / arm64 で実測しました。float32 のスカラーでは、加算と乗算は第 2 被演算子、減算と除算は第 1 被演算子の NaN が残ります。配列と Python の float では第 1 被演算子です。`np.float64 × float` は第 2 被演算子です。C++ の AS / MS / MDS の使い分けはこれと一致しています。
- **Neumaier 加算**: CPython 3.12 以降の `builtin_sum_impl` の流れ (int の 0 から始める、補償を足す条件は `c && isfinite(c)`) を独立に書き直して比べました。Python 3.13.5 の `sum()` と 594/594 で一致し、C++ もこれと同じ形です。CPython のソースそのものは手元に無いため、実測での突き合わせです。
- **色の結合部の 2 つの NaN の順**: 上の 2 つの入れ替え版が検出されたので、現行の順 (`t = f + x` が (f, x)、sigma が (b, a)) がこの環境の Python と一致していることを確かめました。

### ソースの抜き取り確認
- fast-math・FMA の pragma、`std::fma` はありません。
- float 定数はすべて `bits(0x…)` から作っています。double の定数 (0.299 など) は Python のリテラルと同じ値です。
- 演算順を確かめた範囲: `penalty`・`similarity`・`curve_post`・`curve_fill`・`polyfit`・`det`・`cramer`・`combine`。いずれも Python を 1 行ずつ写した順です。
- 多スレッド時の挙動: 判定はスロット順に行い、飛ばしたタイルの結果は捨てています。飛ばしたタイルでモデル計算が失敗しても無視されます。

### 見つけた問題
重大なものはありません。

**中**
- 実写フレームでは NaN や飽和が bins に入りません。NaN を含むタイルは IsClip で 1.0 になり、外されます。そのため bins の NaN の扱いは、作業者の合成 finish 試験 (3000 件) に頼っています。
- NaN を混ぜた image2 は、46 タイルすべてが差分の判定で外されました。NaN が判定を素通りする経路は、通しの試験では通っていません。

**低**
- `verify_aft.py` のキャッシュの鍵に、numpy・Python の版と `data/ref_floor.npy` が入っていません。
- `--mutants` は native/ の `nv_aft_test` を作り直します。`--exe` を指定しても元に戻すのは既定の場所です。
- これ以外の点検 (キャッシュの鍵の残り、空振り、`_run_tiles` の包み方) では問題は見つかりませんでした。鍵は入力・領域・モード・cancel・ソースのハッシュを含み、import 先の nv_flat・nv_model なども `repro/nv_*.py` に入っています。比較は `zip` で切り詰めず、件数の不足は不一致になります。`_run_tiles` の包みは戻り値を記録するだけで、挙動を変えていません。
- 私の `extra_cases.py` は `run_compare` を import したときに本体が再実行されます (結果は同じで、私の作業ファイル内の不備です)。

### 未確認の点
- `curve_fill` の flag & 1 経路と `MODEL_ERROR` (作業者の報告どおり到達しない経路)。
- x86 や非 AArch64 の経路。
- Python 3.12 未満での結合部の期待値。
- 本家バイナリとの一致。正は aft_port の Python で、本家と照合されているのはタイル順だけです。

### 推奨
**APPROVE**。新しい実写・合成・mode 0/1/2・取り消し・小さいサイズ・mode 2 の飛ばし経路で、状態・プロファイル全体・out3・bins が一致し、結合部の NaN の順も実測と陰性対照で裏付けました。

### スクリプトとログの場所
すべて `/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/native_n1c/independent_aft/` の下にあります。
- バイナリ: `bin/nv_aft_test`・`bin/my_shuffle`・`bin/nv_aft_test_mutA`・`bin/nv_aft_test_mutB` (入れ替え版のソースは `mut/`)
- 期待値の生成: `gen_expected.py` (ログ `gen_expected.log`、計 148 秒、並列度 2)。期待値は `tmp/case_*.pkl`
- 照合: `run_compare.py`・`extra_cases.py`・`mode2_skip.py`・`check_shuffle.py`・`check_combine_syn.py`・`check_combine_syn2.py`
- ログ: `run_compare.log`・`extra_cases.log`・`mode2_skip.log`・`shuffle.log`・`combine_syn.log`・`combine_syn2.log`・`mut_combine.log`・`build.log`

## 統括の確認
- `python3 native/verify_aft.py --jobs 2` を再実行: sim 3000・finish 3000・aft 67 (-j 1/-j 2)・combine 69、ALL BIT-EXACT (`rerun_aft.log`。期待値はキャッシュ)。クリーンビルド警告 0。
