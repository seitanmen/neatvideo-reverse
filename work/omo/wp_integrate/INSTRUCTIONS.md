# WP integrate: 本家と一致したノイズ推定を v5 のデノイズに使う (不一致の解消)

前提: `../COMMON.md`、`Agents.md` §4・§6.3・§6.5。現状の不一致:
- v5 のデノイズは輝度別 σ に `noise_curve_spatial` を使う (本家曲線との比 test_v02 1.04〜1.20、test.mp4 0.73〜1.01)。
- auto の帯域形状は時間差分から (本家の領域モデルと段比 6% 以内、レベルは 15〜22% 高い)。
- `CHROMA_GAIN` (Cb 3.0/Cr 4.0) は本家由来でない較正値。

## 手順
1. リバース (他の WP と重複しない範囲): 本家がプロファイル (6 面 × 5 段のモデル、輝度曲線 +0xbf40、タイルずれ RMS) のどの語をフィルタの σ に変換するか、
   その式を `FUN_009a84f8`・Filter の入口で確かめる (wp_temporal / wp_spatial と食い違ったら両方を報告)。
2. 実装: `src/` に v5 の変種 (`nvfilter.py` のコピーを改変)。案: (a) 曲線を `nv_curve.genuine_noise_curve`、(b) 帯域の σ を `nv_profile.build_profile` の領域モデル、
   (c) 両方、(d) (c) + 色差の較正値なし。
3. 比較: 区間 A/B の Y/U/V PSNR を v5 と並べる (COMMON §6)。test.mp4 には正解がないので、合成ベンチ (`validate_v5.py` のコピー) も測る。
   改善しない場合も結果をそのまま報告する。出力動画は自分の `tmp/` へ (repro/out に書かない)。
