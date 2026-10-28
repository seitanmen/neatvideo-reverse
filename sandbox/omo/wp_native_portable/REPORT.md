# F4: arm64 以外の動作 (nv_portable) — REPORT

作業: 2026-10-27、worktree `.claude/worktrees/n1fix`。
対象: N1a nv_band・N1b nv_curve・N1e nv_model (必須)。N1d nv_flat も同じ作りにした (追加)。

## やったこと

### `native/nv_portable.{h,cpp}` (新規、namespace `nvp`)

- 演算: float の add / sub / mul / div / fma (FMADD a·b + c) / sqrt / fmin / fmax / frecpe / frecps と、
  double の add / sub / mul / div / sqrt。
- 結果はどの CPU でも AArch64 の 1 命令の結果とする。
  - NaN が入るとき: FPProcessNaNs (先頭の sNaN を quiet 化、なければ先頭の qNaN)。
    FMADD は FPProcessNaNs3(c, a, b) で、qNaN の c と inf·0 の組なら既定の NaN。
  - NaN でない被演算子から生じた NaN: 既定の NaN 0x7fc00000 / 0x7ff8000000000000 (x86 SSE は符号付きの 0xffc00000 になる)。
- 2 つの実装 (同じ結果):
  - AArch64 の既定 (`NVP_ASM = 1`): 1 演算 1 命令のインラインアセンブラ。4 レーンの NEON 形 (`nvp::V4`) も持つ。
  - portable (AArch64 以外、または `-DNV_PORTABLE`): NaN でない場合は素の C++ で計算し、上の規則を明示的に書く。
    FRECPE / FRECPS はソフトウェア版 (nv_pixnoise.frecpe32 / frecps32。nv_curve.cpp から移設)。
- その他: `fcvtzs` (飽和、NaN → 0)、`opaque`、`frecpe_hw / frecps_hw` (AArch64 では常に CPU の命令。recip-selftest 用)。
- 従来の `-DNVC_PORTABLE` は `NV_PORTABLE` の別名として受ける。

### 各ポートの書き換え

アセンブラ・NEON の記述は nv_portable.h だけに集めた。各ポートの #if は、nv_curve の 4 レーン経路を選ぶ `NVP_ASM` だけになった。

- nv_band.cpp: A / M / AD を nvp に寄せ、SUB / D を追加した。
  - 2 つの変数の減算 (lanes_run、band_from、std_exact、up2 など)、cv の `sqrt / m`、model_levels の比、
    `last·frn` (n == r + 1 で frn = 0、inf·0) を nvp 経由にした。
  - std::fma は nvp::fma にした。
  - x86 照合で見つけた 1 件: `mean_exact` / `std_exact` の `d / n` (n = 0 で 0/0) を nvp::div / nvp::sqrt にした (下記)。
- nv_curve.cpp:
  - FA / FS / FM は nvp を呼ぶテンプレートにした (float と V4)。
  - ratio_eps と ramp の NEON 部は nvp::V4 にした。
  - fma32 / frecpe / frecps / fmin / fmax / fcvtzs を nvp にした。
  - 倍精度の add1 / mul1 / add2 を nvp にし、div1 / sub1 を追加した (det の除算、曲線の平均、CurveInterp)。
  - 端数部の `a / (b + eps)` を nvp::div にした。
- nv_model.cpp:
  - A / S / M / D / FMA / SQRT / FRECPE / FRECPS / fmin_arm / fmax_arm / quiet / opaque を nvp にした。
    非 AArch64 で例外を投げていた FRECPE / FRECPS は、ソフトウェア版で動くようになった。
  - level_info の比を D にした。
- nv_flat.cpp (追加): A / M を nvp にし、色変換の減算と、flatness / level_terms / score の除算を SUB / D にした。
- Makefile:
  - `make portable`: `<name>_p` を `-DNV_PORTABLE` でビルドする。
  - `make x86`: `<name>_x86` を `-arch x86_64` でビルドする。
  - 対象は nv_band_test / nv_curve_test / nv_flat_test / nv_model_test。
  - `clean` で両方を消す。`make verify JOBS=N` で各スクリプトに `--jobs N` を渡す (既定は従来どおり)。
- nv_curve_main.cpp: recip-selftest 用の ub / fb を AArch64 のときだけ定義した (x86 の未使用警告を消すため)。

## 検証

### 1. arm64 既定 (asm)

`make -C native clean && make -C native all` の警告は 0 (`final_build.txt`)。
`make -C native verify JOBS=3` (`final_verify_all.txt`、7 分 56 秒) は全件一致した。

| スクリプト | 結果 |
|---|---|
| verify_quality | 4000/4000 |
| verify_band | 5706/5706 |
| verify_curve | 曲線 1094/1094、IsClip 4000/4000、補間器 20554/20554、タイル 25/25 |
| verify_lzma | 895/895 (本家のブロブ 228/228 を含む。長さ 0 は既知の差としてのみ表示) |
| verify_flat | 362/362 |
| verify_flat_cancel | 676/676 |
| verify_model | 1244/1244、ub 28/28 |
| verify_temporal | 474/474 |
| verify_spatial | 1287/1287 |

### 2. arm64 portable (`-DNV_PORTABLE`、アセンブラ・NEON なし)

`make portable` の警告は 0 (`final_build_portable_x86.txt`)。
Python との照合 (`final_verify_portable.txt`) は全件一致した。

| スクリプト | 結果 |
|---|---|
| verify_band | 5706/5706 |
| verify_curve | 1094/1094、4000/4000、20554/20554、25/25 |
| verify_model | 1244/1244、ub 28/28 |
| verify_flat | 362/362 |
| verify_flat_cancel | 676/676 |

`nv_curve_test_p recip-selftest 2000000`: 2,000,441 組で 0 差 (ソフトウェア版と CPU の FRECPE / FRECPS)。

### 3. x86_64 と arm64 のバイト比較 (`native/verify_x86.sh`、Python を介さない)

- 方法:
  1. 照合スクリプト (band / curve / model / flat / flat_cancel) の C++ をラッパーに差し替え、
     入力ブロブとコマンドラインを取り込む (その回の Python 照合もそのまま通る)。
  2. 取り込んだ全コマンドを `<name>`・`<name>_p`・`<name>_x86` で再実行し、出力をバイト比較する (Rosetta 2)。
  3. `--ref` を付けると、F1 より前のビルドとも比べる。
- 入力には NaN・inf・sNaN・ペイロード付き NaN を含む
  (band の NaN 系の型、曲線で ys に NaN が出る 178 件、model で NaN を含む 254 回、flat の NaN / inf / ペイロードの合成)。
- 1 回目 (`verify_x86_first_run.txt`):
  - curve 5/5、model 2/2、flat 494/494 が一致。band は 0/1。
  - band の差は 5706 ケース中 40 件で、すべて kind 3 (mean / std) の要素数 0 (W·H = 0)。
    arm64 が 0x7fc00000、x86 が 0xffc00000。原因は `mean_exact` / `std_exact` の `d / static_cast<double>(n)` が、
    素の C++ の 0/0 (x86 の既定 NaN は符号付き) だったこと。nvp::div / nvp::sqrt にして解消した。
  - その後の band の再比較は 1/1 一致 (`final_verify_x86.txt`)。
- F4 の最終ビルドでの再比較 (`final_verify_x86.txt`):
  - band 1/1、curve 5/5 一致。以前のビルドとも一致。
  - model と flat の再比較は **統括が実行中** (`final_verify_x86_model_flat.txt`)。
  - 1 回目では model 2/2・flat 494/494 が一致している。最終ビルドとの差は band の n = 0 の修正だけで、model・flat の入力に n = 0 の mean / std はない。
- libm (x86_64 版 / arm64 版で結果が違い得る関数) について:
  - tanf (線カーネル)、atan2・__sincos_stret (nv_model の方向ビン表)、pow (曲線の多項式) について、
    照合したすべての入力で両アーキの出力が一致し、差は見つからなかった (観測)。
  - 関数単体を全域で比べたのではない。tanf は 16 定数角、atan2 / sincos は格子のオフセット、
    pow は小さい整数の指数とクエリの x で、照合の入力が通る値に限られる。
- 感度試験 (x86 の比較が空振りしないこと): x86 の変異体を、取り込んだブロブで arm64 の出力と比べた。
  - process_nans の sNaN 優先を外した変異: band 1/1、model 2/2、curve 3/5 のコマンドが不一致 (検出)。
  - 既定 NaN の補正 (dn) を外した変異: band 1/1、model 2/2、curve 3/5 が不一致 (検出)。
  - 上の mean / std の n = 0 も、補正前の実コードで検出された。

## 計時 (M2 Max、負荷平均 3.2〜4.2、`timing.txt`。単位は秒、best 値)

| 対象 | 変更前 (F1 前の asm) | 変更後 asm | arm64 portable | x86_64 (Rosetta 2、参考) |
|---|---|---|---|---|
| band model_levels 12 面 (512² / 320² / 128² × 4) | 0.018〜0.020 | 0.019 | 0.040〜0.041 | 0.067〜0.070 |
| curve 1080p -j 1 | 0.485〜0.508 | 0.492〜0.503 | 1.776〜1.784 | 2.254〜2.266 |
| model 128² 組 −1 | 0.149 | 0.149 | 0.203 | 0.327 |
| model 320² 組 −1 | 0.812 | 0.810 | 1.234 | 1.936 |
| flat 1080p -j 1 (曲線込み) | 0.811〜0.814 | 0.837〜0.841 | 2.404 | 3.273 |

- 変更後の asm は band / curve / model で変更前と同じ速さ。
- flat は +3%。交互に 3 回ずつ測っても同じだった。曲線単体は変わらないので、帯域測度の部分 (約 0.33 → 0.35 秒) の差とみられる。
  原因は、nv_band の 2 変数の減算を命令固定 (インラインアセンブラ) にして、自動ベクトル化が効かなくなったことと推定する (未確認)。
  大幅な低下ではないので、asm 経路は維持した。
- portable は NEON の 4 レーン経路を持たないので、curve で約 3.6 倍、flat で約 2.9 倍遅い。
  非 AArch64 で正しく動くことを優先した版である。
- x86 は Rosetta 2 上の数値で、ネイティブ x86 の速さではない。

## アーキテクチャ依存が残っている箇所 (F4 の必須範囲外、手を付けていない)

- `native/nv_ndimage.h`: AArch64 ではインラインアセンブラの A / S / M / D / SQRT / AD / SD / MD / DD / FMAD。
  非 AArch64 の代替は素の C++ で、NaN の規則も既定 NaN の符号も扱わない。nv_spatial (N1h) と nv_temporal (N1g) はこれを使う。
- `native/nv_ndimage.cpp`: 非 AArch64 用の SQRT / FMAD (std::sqrt / std::fma)。libm の expf・floor を volatile ポインタ経由で呼ぶ。
  expf は x86_64 版と arm64 版で結果が違い得る (未確認)。
- nv_spatial.cpp・nv_temporal.cpp・nv_quality.cpp: アセンブラ・NEON の記述はない。nv_quality は比較の明示的な扱いだけ。
  いずれも x86 との照合はしていない。
- nv_lzma: SDK の C ソースで、整数演算だけ。x86 との照合はしていない。
- N1g / N1h の期待値が numpy 2.1.3 / scipy 1.15.3 (arm64) の演算順に依存するのは、検証ハーネス側の性質である (MIGRATION.md の既存の記述どおり)。

## 注意・未解決点

- `.gitignore` の `native/**` が `native/verify_x86.sh` を除外している (`!native/*.sh` の例外がない)。
  コミットには `git add -f` か .gitignore への例外の追加が必要。.gitignore は編集していない。
- ログは `sandbox/omo/wp_*/**` の例外に `.log` がないので `.txt` にした。
  `*_interim.txt` は途中の照合 (band 修正前の asm / portable の Python 照合) で、最終結果は `final_*.txt`。
- `sandbox/omo/wp_native_portable/x86/`: 取り込んだブロブ (約 2.9 GB、--keep)。
  統括の model / flat の再比較が終わったら削除してよい (`rm -rf sandbox/omo/wp_native_portable/x86`)。

## MIGRATION.md への追記案 (統括が N1f / N1i と合わせて反映する)

### F1〜F4 (2026-10-27、REPORT は sandbox/omo/wp_native_{flat_cancel,model_ub,linekernel,portable}/)

- **F1 取り消し**: `nvf::find_flat_area_genuine(..., diag, cancel)` と `nvc::genuine_noise_curve(..., threads, cancel)`
  (`nvc::Cancel = std::function<bool()>`)。問い合わせは曲線がタイル数 + 1 回、各サイズの先頭に 1 回。
  サイズ段の取り消しはそれまでの最良の正方形 (切り詰め前) を返す (repro どおり。タスク文の「書かない」とは異なる)。
  複数スレッドでも回数は 1 スレッドと同じ。verify_flat_cancel.py 676/676。
- **F2**: nv_model_bm の 2 経路はどちらも model_full から到達しない (ad1ae0 の辺 ≥ 84。幾何の全数検査で確定)。
  pad ≥ 辺は本家では定義済みの端複製 (未移植)、方向ビンなしは本家ではリスト未初期化。
  C++ は両方 NOT_IMPLEMENTED に統一した (`nvm::bm_quantiles` で直接照合 28/28)。
- **F3**: 線カーネルは `native/nv_linekernel.{h,cpp}` に一本化した (挙動不変、全照合一致)。
- **F4**: 浮動小数点演算は `native/nv_portable.{h,cpp}` に集約した (N1a / N1b / N1d / N1e)。
  - AArch64 の既定はインラインアセンブラ。`-DNV_PORTABLE` と非 AArch64 は、FPProcessNaNs・既定 NaN 0x7fc00000・
    ソフトウェア FRECPE / FRECPS を明示的に書いた版。
  - arm64 portable は Python と全件一致。x86_64 (Rosetta 2) は arm64 とバイト一致 (`native/verify_x86.sh`)。
  - 速さは asm 経路で変わらず (flat は +3%)。portable は curve で約 3.6 倍遅い。
  - x86 で見つけた落とし穴: 素の C++ の 0/0 などは x86 では符号付きの既定 NaN (0xffc00000) になるので、
    NaN が生じ得る演算はすべて nvp 経由にする。
  - 残るアーキ依存: nv_ndimage (N1g / N1h) の asm と、その非 AArch64 代替 (NaN の規則なし)、expf。
- 「既知の差」の `-DNVC_PORTABLE` の記述を `-DNV_PORTABLE` (NVC_PORTABLE も別名として受ける) に置き換え、
  「x86 では NaN を含む入力のビット一致は保証しない」を「x86_64 は arm64 とバイト一致 (N1a / b / d / e)」に更新する。
