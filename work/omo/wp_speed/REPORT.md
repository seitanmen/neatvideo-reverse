> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# wp_speed 作業報告 (2026-10-17)

## 目的
nv_model / nv_lzma / nv_profile の高速化。`build_profile` の cProfile でボトルネックを特定し、
参照版 (base/repro = 2026-10-16 マージ時点の repro) と**全件ビット一致**を維持したまま高速化する。

## ベースライン (base/repro, M2 Max, REPS=2 の最小値, logs/bench_base.txt)
- model_full: 128² = 2.60 s / 320² = 9.07 s / 512² = 22.13 s
- build_profile: 128² = 19.10 s / 320² = 76.08 s
- nv_lzma.blob: 128² 3 面 = 1.08 s (約 11 µs/byte)
- なお base/repro の reference_floor はプロセスごとに 8.7 s 再計算していた (refnoise 解釈実行)。
  本作業で base/data・data 両方にキャッシュ (bit 一致を確認済みの値) を置いた。

## ボトルネック (cProfile, build_profile 128², logs/prof_profile_base_128.txt)
1. ad319c 12.5 s (特殊語): 内訳 sad_grid 4.9 s / _grid_fast+sad_all 2.9 s / best_direction 1.5 s /
   _aggregate_waves 1.8 s / ratio_sums 一括前は ad1ae0 9.4 s
2. tile_dev (32² タイル 147 件の model_full) 2.1 s
3. quantile (q99) 2.2 s (mean/std の _block_sums + 件数計算)
4. linefilt/_general 2.1 s
5. lzma_part 1.4 s

## 実施した高速化 (src/, すべて参照版とビット一致を維持する変更)
1. **nv_refnoise**: 命令テキストを Python ソースに 1 回だけ codegen して実行 (解釈実行の廃止)。
   8.7 s → 約 0.1 s (プロセス 1 回)。加えて (n, seed) ごとのメモ化。
2. **reference_floor のファイルキャッシュ** (data/ref_floor.npy, 参照実装で計算した値を保存。
   base 側にも同一値を配置)。プロファイル 1 回あたり 8.7 s → 0 (ロードのみ)。
3. **nv_band.quantile**: n ≤ 4096 はソート + searchsorted で件数計算 (NaN は数えない等の意味が
   count_nonzero(less_equal) と同一)。フューズ 2659 件でビット一致を確認。
4. **nv_model.kernels**: LineKernel を (W, H, 角度の float32 ビット) でキャッシュ (表は構築後不変)。
5. **nv_model_bm.sad_grid**: U を int16、積分画像を int32 に (|差| ≤ 255、512²·255 < 2³¹ なので
   整数値は同一)。
6. **nv_model_bm.sel_value**: 件数計算をソート + searchsorted に (int64 なので NaN の注意不要)。
7. **best_direction_all / ratio_sums_all / _order_stat**: FUN_009d4a2c の二分探索が「区間 [0, 最大有限値]
   上での順序統計量」に一致することを使い、全ブロックを一括 (np.sort + argmin) で計算。
   失敗ブロック (有効ビンなし) の挙動・選択リストの構成は元の per-block 版と同一。
   ad1ae0 の FAST 経路を差し替え。フューズ + ad1ae0 fast-vs-ref 6 件でビット一致。
8. **ratio_sums 一括**: grid2 の全位置の (s, l_) を一括計算し、float32 の演算順を維持したまま
   ratios ベクトルを直接作る。
9. **sad_all**: 総和を int32 で計算 (255·1024 < 2³¹) 後 int64 に戻す。

## 検証 (すべて本作業フォルダ内のスクリプト、最終コードで実行)
1. **nv_refnoise**: `verify/check_refnoise.py` — コンパイル済みループ vs 解釈実行、(n, seed) 7 組で
   全語ビット一致。65536 値の生成 2.50 s → 約 0.1 s。
2. **quantile**: `verify/check_quantile.py` — base vs src、サイズ 0〜6000 / NaN・inf・-0・同値・
   巨大・微小・非連続 / q・flag・div・m・sd の組合せ 2659 件、全値ビット一致 (sha256 同一)。
3. **nv_model_bm**: `verify/check_bm.py` — best_direction_all vs best_direction (5 段階の INF 率 × 2 設定)、
   ratio_sums_all vs ratio_sums (同上)、ad1ae0 fast-vs-ref (3 サイズ × 2 設定)、
   _offsets_at_fast vs _offsets_at (80 位置)。すべて一致。
4. **ad1ae0 中間値**: `verify/dump_ad1ae0.py` — base vs src で C1/sels/Fimg/C2/rs/最終 2 語の
   全中間値が同一 (128²・48² × 2 設定)。
5. **LZMA**: `verify/mv/verify_lzma_fast.py --oracle --fuzz 30` — ポート == SDK 9.20 C オラクル
   96/96 入力 (ファジング 30 件を含む)、SDK 9.20 == SDK 16.04 96/96、行網羅 862/867
   (未実行 5 行は辞書ラップ等の未移植経路。nv_lzma 自体は本作業で不変)。
6. **model_full vs 本家**: `verify/mv/verify_model_fast.py` — 既存 gcache (2026-10-16 の本家結果 753 件、
   463 面 × 組 −1・0..4) と 4096 語全比較。最終コードでの全件再実行結果は下記。
7. **build_profile vs 本家**: `verify/mv/verify_profile_fast.py` — `nvh profile` (領域指定のため
   FindFlatArea 不使用、nvh 同時 3、selftest ガード確認済み) との 0xc300 バイト比較、76 領域。
   本家結果は内容アドレスキャッシュ (tmp/pCache) に保存。

## 数値 (最終)
- (チェーン完了後に記載)

## 要相談
- LZMA の純 Python 実装は約 10.8 µs/byte のまま (本家比 28〜30 倍遅い点は未解消)。
  C 拡張 (clang 直接・libSystem のみ) にすれば桁違いに速くなるが、repro の合流方針
  「Python 依存は numpy/scipy/pywt/Pillow のみ」に触れるため実装を見送った。判断を依頼。
- model_full 内の linefilt (_general) と _block_sums/_fmla_chain (mean/std) は演算順を厳密に
  保ったままの更なる高速化が難しく手を付けていない (cProfile 上、各 1.5〜2 s/128²)。
