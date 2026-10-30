> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# MERGE_PLAN (wp_integrate → 統括)

対象: `REPORT.md` (2026-10-17) の成果。**実写・合成のどちらも baseline (現行 v5 既定) を上回る変種がないため、
既定の動作変更は提案しない**。マージ候補は下記 2 件 (A は推奨、B は記録のみ)。

## A. `repro/nvfilter.py` へのオプション追加 (推奨: 既定動作は完全不変)

`sandbox/omo/wp_integrate/src/nvfilter.py` (SHA-256 `366f1f7eb3a16a96b59cb455819c15adbbc405f9744b675d87c0891157ebe0bd`)
の差分のうち、`build_profile_v5` への 3 引数 (`curve_src="spatial"`, `band_src=None`, `chroma_gain=True`) と
補助 `_profile_rgb`・`_model_band_sig` (計約 70 行) を `repro/nvfilter.py` に移植する。

- 既定値で呼んだ場合の出力は現行と同一 (引数を渡さなければ分岐に入らない。区間 A/B の baseline 再測定が
  参照値 47.117/50.985/51.179・47.222/50.920/51.189 と一致したことで確認、`logs/calib_296.log`・`calib_560.log`)。
- 依存: `nv_curve`・`nv_profile` (既に repro/ にある、本家とビット一致の移植)。`band_src="model"` は
  `nv_profile` を遅延 import (使わなければ依存なし)。
- 統合後の再検証: ① `python3 -m py_compile nvfilter.py` ② `python3 validate_v5.py` が 38.70 (baseline) を
  再現 ③ `denoise_video_v5.py --params` 無指定の区間 A 出力が PSNR 47.117 を再現。
- UI (`nv_ui.py`)・CLI への公開はしない (実写で利益が無いため。オプションは照合・実験用)。

## B. 記録のみ (コード変更なし)

### Agents.md への追記案 (§6.5 の末尾に 1 項)

```
- フィルタの σ とプロファイル語の対応 (2026-10-17、wp_integrate。静的解析):
  時間 FUN_0099afb4・空間 FUN_00990f74 の閾値はモデル面ブロック (+0x4c+i·0x1f54) の q99 語 (語 5+k) を使い
  (t = C_k·1.1·s_k·√1.64·0.9·|σ_k| / t = (σ_cfg[k]+1e-6)·|model[k]|·1.1、同じランプ式)、FUN_0099f79c→FUN_0099ce38 が
  設定ゲインを語に掛け、輝度曲線 (+0xbf44) は FUN_0099aae90 が色行列とモデル値の二乗で画素単位の σ² に変換する。
  v5 の σ 源を本家側 (ENLC 曲線・領域モデル q99) に置き換えた変種 (a)〜(d) は区間 A/B で baseline を下回った
  (Y −0.03〜−0.28 dB、色差への ENLC 流用は −7〜−9.2 dB。ENLC は輝度のみを測るため)。合成白色ノイズでは
  (a) +0.53 dB。統合点の本家照合: ENLC 8 値・q99 語 30 語、不一致 0 (test_v02 f312)。
```

### exp_results.txt への追記案 (「BuildNoiseModel full port」節の後)

```
== wp_integrate: genuine sigma sources in the v5 profile (2026-10-17) ==
Variant comparison, native YUV PSNR vs the genuine output (calibrate_v5.py, COMMON §6 procedure,
params = v5 defaults threshold 4 / luma 0.7 / spatial 2.0; logs wp_integrate/logs/calib_296.log, calib_560.log):
  A (n=304-319): baseline auto 47.117/50.985/51.179 (Y/U/V; reference 47.12/50.98/51.18 reproduced)
    a genuine ENLC curve 46.984/41.346/40.984   b model band 46.838/50.987/51.201
    c both 46.988/41.445/41.300                 d no chroma gain 46.988/48.561/49.446
  B (n=568-583): baseline 47.222/50.920/51.189
    a 47.199/42.005/41.480   b 47.139/50.915/51.198   c 47.175/41.906/41.657   d 47.175/48.888/49.819
  chroma of (a)/(c) substitutes the LUMA ENLC curve (sigma 13.1/17.3 vs measured chroma noise ~2.5):
  the genuine's chroma sigma comes from AutoFineTune's tables, not producible in the harness.
  synthetic sigma-15 RGB (wp_integrate/logs/validate_v5.log): noisy 24.54 / baseline 38.70 /
  a 39.23 / b 38.98 / c 39.35 / d 39.42.
  model band ratios f312 Y: 1, .516, .279, .165 (v5 area: 1, .502, .282, .168; chroma ratios >=1 are
  capped to the 0.6/level prior). Coarsest bands fall back to the area sigma (the genuine marks
  too-small levels negative): Y band 4 (128px area), Y 3-4 + chroma 2-3 (64px, segment B).
Integration cross-check vs the genuine (f312, nvh selftest=3 first): ENLC ys 8/8 identical
(%.17g); nv_profile q99 words 6 planes x 5 levels 30/30 identical (mode 1, area 792,0,128,128).
Verify scripts: wp_integrate/verify/nvh_integration_f312.py, profile_word_check.py (injections detected).
```

## 前提と注意

- 追加する事実はすべて実測で出典ログ付き (上記)。推定を含む部分 (§1.1 の param_13 読みの対立) は
  Agents.md には「wp_temporal とは別読みがある」形で書かないこと (未決着のため。REPORT §5.1 を参照)。
- `band_src="model"` のフォールバック挙動 (負語 → 領域 σ) は本家の「小さすぎる段は無効」規約に従う
  実装側の判断。マージする場合はこの点を docstring に残したままにする。
- 区間 A/B の数値は 1080p・721f の test_v02 1 本のみ。別素材での確認は今後課題 (AGENTS §7 と同じ制約)。
