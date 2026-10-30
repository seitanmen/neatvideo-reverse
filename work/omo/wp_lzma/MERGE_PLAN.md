> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# wp_lzma マージ計画 (MERGE_PLAN.md)

対象リポジトリ側へ私からは書けないため、統括が行う作業の提案。優先度順。
`repro/nv_lzma.py` 自体は今回の全照合に合格しており、**コード変更は不要**。

## 必須 (推奨)

1. **感度試験の入力群の拡充** (`sandbox/lzma_verify/` 相当の箇所)
   - 追加する入力: `wp_lzma/data/extra_lit_minimal.bin` (3,113 B、18 差分の最小例)、
     `wp_lzma/data/hash4_plane_s1.bin` (2,048 B、辞書 4KiB 相当)、乱数面 16KiB/32KiB
     (blob の -1 経路; 生成は `verify_blob.py` の `synth_corpus` 参照)、辞書境界長
     (4095/4096/4097 など `verify_dict_boundaries.py` の長さ表)。
   - 効果 (実測に基づく): 14 変異すべてが自動検出される。
     `extra_lit_rep_cond` はこの入力で base==C・mutant≠C、`hash4_mask_minus` は 8 シード
     すべてで検出。
2. **AGENTS.md §6.5 の版記述の更新**
   - 現行: 「本家の SDK の版は特定していない (9.20 と 16.04 が本家と同じ出力、19.00 以降は異なる)」
   - 実測: 9.20〜18.05 が 228/228 ダンプで一致、18.06 が初差 (kNumOps 縮小と GetOptimum 書き換え)。
     静的判別子 (Init の構造体形状・Normalize 末尾に reduceSize ループなし・MatchFinder_Create の
     hs 直書き) も 9.20 形状。
   - 併記候補: props の辞書は 2<<i / 3<<i はしごで切り上げ (LzmaEnc_WriteProperties)。
     呼び出し容量は W·H·2 + 0x90 → destLen = n + 0x80、非圧縮面は n ≳ 10KiB で -1。

## 任意

3. **照合スクリプトの取り込み**: `wp_lzma/scripts/verify_blob.py`・`verify_dict_boundaries.py`・
   `verify_region_boundaries.py` を `analysis/noise/lzma_oracle/` (または model_verify) 相当へ移植するか。
   オラクル `src/lzo_cap.c` (容量指定) も一緒に。バイト比較の基準は C 9.20
   (`lzma_oracle/` に既にある SDK 9.20 と同じ)。
4. **本家最小領域ダンプの保存**: `wp_lzma/data/boundary/prof_r*.bin.lzma{0,1,2}`
   (32×32・45×45・46×46・64×64・512×512、test_v02 n=312 の (830,300)) を
   `analysis/noise/model_verify/tmp/` 相当の本家キャッシュに加えるか
   (最小面長の記録が 4,608 → 2,048 に更新される)。

## 実施しない事項 (理由)

- `repro/nv_lzma.py` の変更: 不要 (235 + 87 + 15 件で不一致 0、感度試験も充足)。
- ハーネス (`nvh`) の新サブコマンド: 不要 (領域指定 `nvh profile` で完結した)。
  `harness_request.md`・`nvh_patch.diff` は作成していない。
