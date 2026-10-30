# F3: N1e の線カーネル重複の解消 — REPORT

作業: 2026-10-27、worktree `.claude/worktrees/n1fix`。結果のビットは 1 つも変わっていない。

## やったこと

- 共通の翻訳単位 `native/nv_linekernel.{h,cpp}` (namespace `nvl`) を新設した。
  - `LineKernel` と `make_kernel(W, H, a)` (FUN_00990448、nv_lines.LineKernel)。
  - `LinePlan` と `make_plan(K)` (nv_lines.LinePlan)。
  - 内部の定数 (F_0_01・F_PI・F_HALF_PI・F_NEG_HALF_PI・F_0_26・QUARTER_PI_D) と、volatile ポインタ経由の tanf。
- nv_curve.cpp: 自前の LineKernel / make_kernel / LinePlan / make_plan と専用の定数・g_tanf を削除し、
  `using nvl::…` に置き換えた。
- nv_model.cpp: 計画のフィールドを抱き込んだ自前の LineKernel を、`struct LineKernel : nvl::LineKernel, nvl::LinePlan {}` と
  薄い `make_kernel` (斜めの角度のときだけ make_plan) に置き換えた。K.m / K.G / K.inv などの参照はそのまま。
- Makefile: nv_curve_test / nv_flat_test / nv_model_test に nv_linekernel.cpp を追加した。

## 2 つの複製の差と、統一の判断

| 項目 | nv_curve | nv_model | 統一後 |
|---|---|---|---|
| 丸め | `std::rint` | `std::nearbyint` | `std::rint`。既定の丸めモード (最近接偶数) では同値で、どちらのコードも丸めモードを変えない |
| π/4 | `0x1.921fb54442d18p-1` | `0.7853981633974483` | 同じ double (Python の `(0.7853981633974483).hex()` で確認) |
| make_plan の検査 | 書いてから重複検査、範囲検査なし | 書く前に範囲 (`pix < 0 || pix >= HW`) と重複を検査 | model 側の厳しい方。違いが出るのは不正なカーネルで例外になるときだけ |

## 検証 (リファクタの前後で同じ件数の全件一致)

- `python3 native/verify_curve.py` (F3 直後、ログ `verify_curve_f3.txt`): 曲線 **1094/1094**、IsClip **4000/4000**、
  補間器 **20554/20554**、pixnoise タイル **25/25**。ALL BIT-EXACT。
- `python3 native/verify_model.py --jobs 3 --genuine` (ログ `verify_model_f3.txt`): **1244/1244**、ub **28/28**、
  本家のダンプ **469/469**。
- `python3 native/verify_flat.py --jobs 3`: **362/362** (曲線を使う。ログ `sandbox/omo/wp_native_flat_cancel/verify_flat_regress.txt`)。
- バイナリ単位の確認: `native/verify_x86.sh --ref <F1 より前のビルド>` で、F1〜F4 の前にビルドしたバイナリと
  現在のバイナリの出力をバイト比較した。入力は、照合スクリプトの入力ブロブを取り込んだもの。
  ログは `sandbox/omo/wp_native_portable/verify_x86_first_run.txt` と `final_verify_x86.txt`。
  - curve: 5/5 コマンドが一致 (以前のビルドと比べたのも 5 件)。
  - model: 以前のビルドと比べた 1 件 (model_full) が一致。bm は以前のビルドにないので飛ばした。
  - flat: 以前のビルドと比べた 66 件が一致 (cancel は以前のビルドにないので飛ばした)。
  - band: 一致 (F4 で x86 の修正を入れる前の 1 回目でも、以前のビルドとは同じ)。
- F4 の後も同じ件数で一致した (`final_verify_all.txt`)。

## 計時

線カーネルは大きさごとに 1 回作ってキャッシュするので、処理時間への影響はない。
曲線 1080p の -j 1 は、変更前 0.485〜0.508 秒、F3 + F4 の後 0.492〜0.503 秒 (`timing.txt`)。

## 未解決点・Python 側への不具合報告

なし。
