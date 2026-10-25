# WP lzma: LZMA 移植の残り

前提: `../COMMON.md`、`neatvideo_ofx_614/analysis/noise/lzma_oracle/README.md`、`repro/nv_lzma.py`、`Agents.md` §6.5 の LZMA の項。
本家の圧縮データは既存ログ (`neatvideo_ofx_614/analysis/blackbox/logs_20261016/lzma/`・`sandbox/lzma_verify/logs`) と
`analysis/noise/model_verify/tmp/` のキャッシュにある。新たに本家を呼ぶなら `nvh profile ... x y w h` (領域指定、32〜512px) だけ。

## 項目
1. **SDK の版の絞り込み**: 7-zip.org の LZMA SDK 9.20 と 16.04 の間の版 (例 9.22, 9.35, 9.38, 15.xx, 16.02/16.03 の公開物) を `dl/` に取得し
   (SHA256 を記録)、C オラクルを `src/` でビルドして既存の本家ダンプと比べる。出力が 9.20 と異なり始める版を特定する。
   加えて、本家バイナリの静的な文字列・定数 (LzmaEnc の表や版文字列) から版を示す痕跡がないか読む。
2. **検出されなかった突然変異 2 種** (`hash4_mask_minus`・`extra_lit_rep_cond`、独立検証者の記録は `sandbox/lzma_verify/`): 等価であることの
   証明か、差が出る入力の構成。差が出るなら照合の入力に加える案を書く。
3. **`blob_size` の未試験**: 試験を追加 (移植の関数を照合に含める)。
4. **境界の照合の追加**: 辞書サイズの切り上げ境界 (4KiB の倍数 ±1)、長さ 0 の面に相当する最小領域 (32×32)、最大 512×512 の面。

比較: オラクル (C 9.20) と本家ダンプのバイト比較。不一致 0 を示し、感度試験も行う。
