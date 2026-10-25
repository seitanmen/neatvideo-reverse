> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# wp_speed → repro マージ計画

## 対象と結論
- `src/` 13 モジュールのうち変更は **5 ファイル** (SHA256 は notes/SRC_SHA256SUMS.final、
  未変更 8 ファイルは base と同一):
  - `nv_refnoise.py`: 命令テキストから生成した Python ループで実行 (旧解釈実行は `noise_ref` として残存)。
    (n, seed) ごとのメモ化付き。8.7 s → 約 0.1 s。
  - `nv_band.py`: `quantile` の件数計算を n ≤ 4096 はソート + searchsorted に (NaN を数えない等の
    意味が count_nonzero(less_equal) と同一。フューズ 2659 件で確認)。
  - `nv_model.py`: `kernels()` — LineKernel を (W, H, 角度ビット) でキャッシュ。
  - `nv_model_bm.py`: sad_grid の int16/int32 積分画像、sel_value のソート化、
    `best_direction_all` / `ratio_sums_all` / `_order_stat` (全ブロック一括の順序統計量)。
    旧 per-block 版 (best_direction / ratio_sums) は FAST=False 用に残存。
  - `nv_model_special.py`: `sad_selected` (候補 24 個のみの SAD) と `_offsets_at_fast`、
    sad_all の int32 総和、ad319c の U8 計算のループ外移動。
- すべて「演算順・丸めを変えない」変更で、参照版とビット一致 (検証結果は REPORT.md)。

## マージ手順 (統括)
1. 上記 5 ファイルを repro/ に上書き (base からの差分は本作業のみ)。
2. `data/ref_floor.npy` (51 語、参照実装で計算した固定値) を `repro/../data/ref_floor.npy`
   (= reverse/data/) に配置。nv_profile.reference_floor はこのパスを見るのでコード変更不要。
   - これにより nv_profile の初回呼び出しが毎回 8.7 s かけていたのが即座になる。
   - 自動生成にする場合は reference_floor に「なければ計算して保存」を足すのが最小変更
     (本作業では既存コードのまま配布ファイルとする)。
3. マージ後の確認: `analysis/noise/model_verify/verify_model_cases.py --sets --jobs 6` が既存
   gcache と全件一致、`verify_profile.py` (76 件) が pointers 6 語以外 0 不一致、
   `lzma_oracle/verify_lzma.py --oracle` が全件一致 (nv_lzma は本作業で不変)。

## 注意点
- `nv_model.kernels` のキャッシュは単一スレッドでの使用を想定 (ProcessPoolExecutor での
  プロセス分離では影響なし。既定の `_KCACHE_MAX` = 512 エントリで自動クリア)。
- `best_direction_all` は「有効ビンなし」の行があると旧版と同じ例外を投げる。
- hit() (分岐カウンタ) の呼び出し位置はベクトル化で変わった (語の出力には無関係)。
- 依存は numpy のみ (既存と同一)。C 拡張は使っていない。
