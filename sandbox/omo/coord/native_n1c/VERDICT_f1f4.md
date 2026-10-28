# 独立検証 VERDICT: ネイティブ F1〜F4 (2026-10-28)

独立検証者 (verifier サブエージェント) の最終応答を統括が転記した (第 1 回)。

## 検証報告 (F1〜F4、独立検証)

### 判定
**全体**: INCOMPLETE。確信度は高。ブロッカーは 1 件 (F4)。

| 項目 | 判定 | 確信度 |
|---|---|---|
| F1 取り消し (状態 5) | APPROVE | 高 |
| F2 未到達経路の明示化 | APPROVE | 高 |
| F3 線カーネルの共通化 | APPROVE | 高 |
| F4 アーキテクチャ非依存化 | REJECT (軽微。1 行の修正で解消する見込み) | 高 |

### 見つけた問題

**[中] F4: `nv_flat.cpp:140` の `luma_601` に `std::fma` が残っている。** ここは nvp を経由していません。
```cpp
for (size_t i = 0; i < m; ++i) out[i] = std::fma(B[i], c, std::fma(G[i], b, M(a, R[i])));
```
- 観測: 1 画素に R=+inf と G=−inf がある入力 (inf − inf) では、x86_64 版 (Rosetta) の縮小輝度の診断出力 (`reduce[3]`) が 0xffc00000 になります。arm64 版と Python は 0x7fc00000 です。
  - 自作の 8 件中 1 件と、追加の 2 件で差が出ました (差の数は 6・6・12 値)。
  - arm64 の asm 版と portable 版は、Python と全値一致しました。
- 推定: x86 の libm の fmaf が x86 の既定 NaN を返すためです。arm64 では `std::fma` がハードウェアの fmadd になるので、`-DNV_PORTABLE` のビルドでは見つかりません。
- 参考: `-mavx2 -mfma` を付けた x86 ビルドでは、8 件中 4 件で NaN ペイロードの選び方まで変わりました。これは本来のビルド対象ではありません。
- 影響: 領域・状態コード・trace のスコアは、全件でアーキテクチャ間も Python とも一致しました。差は診断用のバイト列だけです。
  - 推定: NaN のペイロードは制御の流れに影響せず、NaN を含む候補は `v < MASKED` で落ちます。
- それでも「x86 と arm64 の出力はバイト単位で一致する」という主張は、flat については成り立ちません。作業者の `verify_x86.sh` の flat 494/494 は、この入力の型を含んでいませんでした。
- 修正案:
  - 140 行目を `nvp::fma` にする。
  - 同じ理由で、157 行目 `T[i] = luma[i] - Abuf[i]` も `SUB` にそろえる (2 変数の減算。出力には現れないことを確認済み)。
  - 修正後は `flat_luma_x86.py` を再実行して確かめる。

**[低] F4 の成果物にある「MIGRATION.md の現状更新」が worktree にありません。** 76 行目は「AArch64 前提。x86 では NaN を含む入力のビット一致は保証しない」のままです。統括が更新する予定なら問題ありません。

**[情報] F2:** 作業者の訂正は妥当です。経路 1 (pad ≥ 辺) は本家の未定義動作ではなく、「定義済みだが未移植の分岐」です。repro の docstring にある「edge row when p >= H」と合っています。

### 確かめたことと結果

| 確認 | 結果 | 新しい入力 | 根拠 |
|---|---|---|---|
| ビルド | 合格 | 規約のフラグで asm・portable・x86_64・HEAD (ca689d2) の 20 バイナリを自分でビルド | 警告 0 (`logs/build.log`) |
| F1 取り消し | 339/339 一致 | 実写 4 枚 (test_v02 n=33・n=640 (色モード 0)、test.mp4 n=505・n=77 (色モード 2、明示の曲線表)) と合成 2 件 (333×251 の傾斜と平坦部、160×120 で NaN・sNaN・inf・−0 を含む) | `logs/f1_cancel.log` |
| F1 x86_64 版 | 60/60 一致 | 合成 2 件、k は全値、-j 1 と -j 2 | `logs/f1_cancel_x86.log` |
| F2 | 3 ビルドとも全件合格 | 幾何の総当たり、直接呼び出し 88 件、model_full 5 件 | `logs/f2_ub*.log` |
| F3・F4 (HEAD・asm・portable) | 4 ツールすべてバイト一致 | band 432 件、curve 35 件、model 245 件、flat 8 件 | `logs/cmp_full.log` |
| F4 (x86_64) | band・curve・model は一致、flat は不一致 | 上と同じ全件 | `logs/cmp_x86*.log` |
| Python との一致 | 全件一致 | band 432/432、curve 35/35、model 25/25 (10 件に 1 件)、flat 8/8 | `logs/py_f34.log` |
| portable の実装 | 2,580,605/2,580,605 一致 | 特殊値の全組み合わせと乱数ビット列 | `ptest/` |
| 規約 | 違反なし | — | diff の確認 |
| repro/・neatvideo_ofx_614/ | 変更なし | — | 両ツリーの git status |

**F1 の詳細**
- k は、曲線の途中・タイル数・タイル数+1・各サイズの先頭・総数・総数+1 を使いました。test.mp4 と合成では k を全値試しました。C++ は -j 1 と -j 3 です。
- (x,y,w,h,status) と問い合わせ回数が、Python とすべて一致しました。1080p では、問い合わせは 247 回 = 曲線 241 回 + サイズ 6 回でした。
- 取り消しなしの `flat` の出力ファイル全体が、HEAD のバイナリとバイト一致しました (回帰なし)。portable 版でも同じ確認をしています。
- サイズ先頭での取り消しで書きかけの最良値を返すケースは 23 回あり、repro と同じ値でした。

**F1 の仕様との食い違い**
- 作業者が repro を優先した判断は妥当です。理由は 2 つあります。
  - 共通規約の 1 で、repro が正とされている。
  - AGENTS.md §6.5 にある本家の観測 (「それまでの最良の正方形のまま状態 5」) とも一致する。
- タスク文の「書かない」に当てはまるのは、曲線の段での取り消しだけです。この場合は repro も C++ も 0 を返します。

**F2 の詳細**
- 幾何の総当たりは、作業者の区間による再構成とは別の方法で行いました。repro の `sad_grid` を 0 の画像に掛け、`best_direction_all` を呼んでいます。INF は幾何だけで決まることをコードで確認しました。
- 正方形で条件を満たすビンがなくなるのは、設定 0 で辺 13 以下、設定 1 で 28 以下でした。辺 84〜1536 の 11 通りの組み合わせでは失敗はありません。
- 「model_full からは到達しない」はコードで確かめました。`ad319c` は min(W,H)·3 < 83 で戻り、89 未満の場合は 3 倍に並べるので、`ad1ae0` に渡る辺は 84 以上です。
- 直接呼び出しは 88 件で、内訳は NotImplementedError が 48 件、正常が 40 件です。状態と分位点のビットが Python と一致しました。
- 呼び出し元に届く最小の大きさ (28×28、29×95、88×89、89×89、30×28) で model_full の組 4 を動かすと、状態 0 で 4096 語すべてが一致しました。
- 以上は asm・portable・x86_64 の 3 ビルドで同じ結果です。

**F3・F4 の入力**
- 新しい入力には次を含めました: NaN の対 (ペイロードと符号の違い)、sNaN (0x7f800001・0xff800777・0xffbfffff など)、±inf の隣接、和の溢れ (±3e38)、−0、非正規数、端や 4 レーン境界に置いた特殊値、全面 sNaN。
- 出力の中に 16 種類の NaN ビット列が現れたので、比較は空振りしていません。curve と flat は -j 2 でも同じ結果でした。

**portable の実装 (自作の命令オラクルとの比較)**
- 比べた演算: add・sub・mul・div・fmin・fmax・sqrt・FRECPE・FRECPS・FMADD (三つ組) と、double の add・sub・mul・div・sqrt。
- 比較元は自分で書いたインラインアセンブラです。arm64 上の portable 版と、x86_64 上の portable 版の両方で全件一致しました。
- ソースを読んだ範囲でも、ARM の規則どおりでした: sNaN 優先と quiet 化、FMADD の c 優先と qNaN c + inf·0 → 既定の NaN、FRECPS の被演算子の符号反転と inf·0 → 2、FRECPE の非正規数と 2^-128 未満の扱い。

**規約**
- fast-math や FMA 融合を許す pragma は増えていません。
- 新しい float 定数もありません。
- 線カーネルの定数は HEAD と同じビット列です。`QUARTER_PI_D` は 16 進表記と 10 進表記の値が等しいことを確認しました。

### 未確認の点
- 作業者の `verify_x86.sh` は実行していません。作業者の出力先に書いてしまうためです。
- x86 では libm (tanf・atan2・__sincos_stret・pow) を関数単体で全域にわたって比べていません。確認したのは自分の入力が通る値だけです。
- x86 の計時はしていません。
- -j 3 で確かめたのは F1 だけです。ほかは -j 2 です (CPU の制約のため)。
- 実写に NaN が入るケースは試していません。

### 推奨
REQUEST_CHANGES。F1〜F3 はこのままマージしてよいと考えます。F4 は `nv_flat.cpp:140` の `std::fma` を `nvp::fma` にし (157 行目の減算も `SUB` にそろえるとよい)、x86 で再照合してからマージしてください。

### スクリプトとログ
すべて `/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/native_n1c/independent_f/` にあります。
- スクリプト: `build.sh`、`frames.py`、`f1_cancel.py`、`f2_ub.py`、`gen_f34.py`、`cmp_f34.sh`、`py_f34.py`、`flat_luma_x86.py`、`ptest/ptest.cpp`
- ログ: `logs/` (`build.log`、`f1_cancel*.log`、`f2_ub*.log`、`cmp_full.log`、`cmp_x86*.log`、`py_f34.log`、`flat_luma_x86.log`)
- バイナリは `bin/`、入出力ブロブは `tmp/`、フレームは `frames/` にあります。合計 207 MB で、照合が済めば消してかまいません。


---

# 第 2 回 (統括が nv_flat.cpp の 2 行を修正した後)

## 再検証報告: F4 (`nv_flat.cpp` の 2 行の修正後)

### 判定
- **F4: APPROVE (確信度: 高)。** 前回の [中] は解消しました。修正による回帰もありません。
- F1〜F3 は前回の APPROVE のままです。修正後のバイナリで F1 も再確認しました。
- ブロッカーは 0 件です。残っている指摘は、worktree の MIGRATION.md 76 行目が未更新という [低] 1 件だけです。

### 修正の差分
- 控えの `nv_flat.cpp.pre_luma_fix` と比べた差は、統括が書いたとおりの 2 行だけでした。
  - 140 行目: `std::fma` を `nvp::fma` にした。
  - 157 行目: 減算を `SUB(luma[i], Abuf[i])` にした。
- worktree の `nv_flat.cpp` には、ほかの変更はありません。

### 確認した結果

| 確認 | 結果 | 根拠 |
|---|---|---|
| 再ビルド | 自分の `bin/` で flat を作り直した: asm・portable・x86_64、参考に x86_64 の `-mavx2 -mfma`。規約のフラグで警告 0。変更前のバイナリは `*_preluma` の名前で残した | `logs/build_luma_fix.log` |
| 縮小輝度の差 (`flat_luma_x86.py`、10 件) | inf − inf の 3 件を含め、Python と違う値は asm・portable・x86_64 のどれも 0。修正前の x86_64 は 6・6・12 値違っていた | `logs/flat_luma_fix.log` |
| flat の出力全体 | `flat_full` 8 件・`flat_x86` 2 件・`flat_luma` 10 件の 3 つのブロブで全一致。比べたのは portable・x86_64・`-mfma` 版・修正前の asm・HEAD・asm の -j 2・x86_64 の -j 2 で、どれも asm とバイト一致 | `logs/flat_luma_fix.log` |
| 実写フレーム | test.mp4 n=505 と test_v02 n=33 で、x86_64 と HEAD が新しい asm とバイト一致 (-j 2) | `logs/flat_luma_fix_real.log` |
| F1 の取り消し | 合成 2 件で k を全値試し、90/90 が Python と一致 (asm の -j 1・-j 3、x86_64 の -j 1) | `logs/flat_luma_fix_real.log` |
| quantile の追加照合 | 6000/6000 が Python と一致 (asm・portable・x86_64)。結果が NaN になった例は 1061 件、inf は 25 件 | `logs/quant_fuzz.log` |

- arm64 の出力は修正の前後で変わっていません。修正で変わったのは x86_64 の結果だけです。

### nvp を経由しない演算の抜き取り確認
対象は nv_flat・nv_band・nv_curve・nv_model・nv_linekernel です。grep で 2 項演算の候補を拾い、コードを読んで仕分けしました。候補の一覧は `logs/grep_raw_ops.txt` にあります。

**定数との演算で、問題にならないもの**
- `recip * S[l]`、`* C0_25`・`* C0_75` など、有限で 0 でない定数との積や商がほとんどでした。
- NaN の被演算子が 1 つだけなら、x86 も ARM も同じ NaN を quiet 化して返すので、差は出ません。
- nv_flat の flatness の `thr[]` は定数から作った値なので、同じく問題ありません。

**データ同士の生の演算だが、出力に影響しないもの (コードで確認)**
- nv_curve の `is_clip_genuine` (`mx - mn`、`k * dv`): できた NaN は `fcvtzs(rint(p))` で常に 0 になり、出力は整数比 `peak/nn` です。NaN の符号は結果に出ません。
- nv_model の `image_u8` (`C_255 / (mx - mn)`、`s * (-mn)`): できた NaN は `u8_of` で常に 0 になります。

**データ同士の生の演算で、照合では差が出なかったもの**
- nv_band の quantile、529〜530 行の `(upper - lower) / k` と `upper - slope * c_up`。
- inf・巨大値・NaN を多く含む 3000 本で照合し、x86_64 も含めて全件一致しました。
- ただし、この分岐に inf が実際に届いたかまでは確かめていません。安全と証明したのではなく、観測した範囲で差がなかったということです。

**すでに nvp を経由しているもの**
- nv_curve の double の det・polyfit・平滑化・補間器は `add1`・`mul1`・`add2`・`div1` を使っています。
- nv_model の `level_info` の比と FMA も nvp 経由です。

**単項の演算**
- `std::sqrt(std::fabs(x))`、`std::trunc`、`std::rint` などは単項です。x86 で無効演算は起きず、NaN の扱いも ARM と同じです。

### 未確認の点
- quantile の 529〜530 行で inf が届く経路があるかは、静的にも動的にも確定していません。
- x86 の libm (tanf・atan2・sincos・pow) を、関数単体で値域全体にわたって比べてはいません。
- 統括が実行中の Python 回帰照合の結果は見ていません。

### 推奨
APPROVE。F1〜F4 はマージしてよいと考えます。MIGRATION.md 76 行目の「x86 では NaN を含む入力のビット一致は保証しない」は、今回の結果に合わせて更新が必要です。

### 場所
すべて `/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/native_n1c/independent_f/` にあります。
- 今回追加したスクリプト: `quant_fuzz.py`
- 今回のログ: `logs/build_luma_fix.log`、`logs/flat_luma_fix.log`、`logs/flat_luma_fix_real.log`、`logs/grep_raw_ops.txt`、`logs/quant_fuzz.log`
- バイナリ: `bin/flat_{asm,p,x86,x86a}` (修正後) と `bin/flat_*_preluma` (修正前)

---

## 統括の確認 (2026-10-28)
- 第 1 回の [中] (nv_flat.cpp の `std::fma` と減算の 2 行が nvp を経由していない) は統括が worktree で修正 (控え `nv_flat.cpp.pre_luma_fix`)。
  修正後の worktree で verify_flat (asm・portable) ALL BIT-EXACT、verify_flat_cancel 676/676 (`wp_native_portable/postfix_*.txt`)。
- x86 照合の残り (`verify_x86.sh --only model,flat`、修正前のビルド) を統括が実行: model 2/2・flat 494/494 コマンドがバイト一致 (`wp_native_portable/final_verify_x86_model_flat.txt`)。
  修正後の x86 は検証者の第 2 回で確認。
- 元のツリーへ取り込み (控え `coord/backup/*.20261028_pre_n1c`)、単独の makefile に nv_linekernel / nv_portable を追加、native/Makefile に N1f・N1i を追加。
  クリーンビルド 9 本で警告 0 (`build_all.log`)、`make -C native verify JOBS=3` 11 本とも全件一致 (`rerun_all.log`、9 分 17 秒)。
