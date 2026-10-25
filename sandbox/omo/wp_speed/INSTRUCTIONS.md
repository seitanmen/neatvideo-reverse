# WP speed: ビット一致を保った高速化 (nv_model・nv_profile・nv_lzma)

前提: `../COMMON.md`。現状: `nv_model` は本家の約 28〜30 倍遅い (1 面 128² で約 2.8 秒)、`nv_lzma` は 1 バイトあたり約 10µs (128² の 3 面で約 1 秒)。

## 手順
1. プロファイル (cProfile) で熱い箇所を測る。
2. `src/` のコピー上で高速化する (numpy のベクトル化・まとめ処理。C 拡張を使う場合は libSystem 以外にリンクしない純粋な計算に限り、本家は読み込まない)。
   演算順・丸め (fma と非融合、float32、NaN の扱い) を変えないこと。
3. 比較:
   - 高速版と参照版 (base のコピー) のビット一致: `analysis/noise/model_verify/` の入力 (コピー) 全件、`verify_lzma.py` の入力と fuzz。
   - 本家との一致: `verify_model_cases.py --sets` 相当と `verify_profile.py --quick` 相当を高速版で再実行 (本家の結果はキャッシュを使う)。
   - 速度・メモリ: 1 面 128/320/512、プロファイル 128²/320²、LZMA の 3 面。負荷平均・回数を記録。
