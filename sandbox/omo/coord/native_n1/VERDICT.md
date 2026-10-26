# N1a・N1b・N1c の独立検証 (2026-10-26)

検証者 (サブエージェント、opus) の最終応答の要点を統括が転記した。検証者のファイルは scratchpad/verify_n1/ (ws/run_*.log・scripts/t_band.py・t_curve.py・t_lzma.py・consts.py)。

## 判定: **APPROVE** (阻害要因 0)
- 方法: native/ を scratchpad にコピーして新しくビルドし、そこで照合した (native/・repro/・sandbox/ には書いていない)。ビルドは警告 0。
- 作業者の照合の再実行: N1a 5706/5706、N1b 曲線 1094/1094・ヒストグラム 1094/1094・clip 4000/4000・interp 20554/20554・tile 25/25、N1c 949/949。
- 独自の入力 (test_v02 n=50/250/650、test.mp4 n=600/123、別の切り出し・乱数・特殊値):
  - N1a: model_levels 80/80、band_levels 80/80 (4096 語)
  - N1b: 曲線 22/22 (-j1 と -j3。1080p の実写全面 2 枚、3840×2160、非正規数・溢れを含む)、IsClip 500/500、interp 1122/1122
  - N1c: 102/102 (上限超えの −1 を 3 件含む)
- 検証者の照合の感度: 1ulp の変異などで band 22/29 件、lzma 14 件、curve 15 件 + clip 1 件の不一致を検出した。
- -O3 と移植可能版 (-DNVC_PORTABLE) でも一致した。
- 規約:
  - フラグを確かめた。
  - float 定数 20 項目が Python 側のビットと一致した。
  - SDK ソースの SHA256 が 13/13 と 12/12 で一致し、配布物とも cmp で同一だった。
  - repro/ は無変更 (git status・diff が空、スナップショットの 28 行が OK)。
- 照合から除外しているのは 2 つだけで、どちらも妥当とされた。
  - N1c の長さ 0 の入力の props。誤っているのは Python の `props_bytes`。
  - 本家の保存データのヘッダのバイト 13..15 (ヒープの残骸)。

## 注意 (統合時)
- 中: ルートの `.gitignore` が `*` の許可リスト式なので、native/ の新しいファイル (nv_band.*・nv_curve.*・nv_lzma.*・nv_*_main.cpp・verify_{band,curve,lzma}.py・Makefile.*・lzma/) はすべて無視される。コミットするときは例外を足すか -f が必要。
- 低:
  - N1c の長さ 0 の props の差。N1f の照合でも同じ除外が要るかもしれない。
  - 未移植のものがある (nvb::quantile の q ≥ 1・div < 0、nv_curve の取り消し・nbins ≠ 8)。N1e で使うのは q < 1 なので問題にならない見込み。
  - native/Makefile の clean に、使われない tmp_curve_*.bin がある。
  - 128 MiB の辞書上限と、本家での −1 の実例は未照合。
- 報告の軽い言い過ぎ:
  - N1b の「16×16 ブロック 2000」は、実際は長さ 256 の 1 次元配列 (結果は等価)。
  - N1c の境界 15 件は、バイト列 96 件の内数。
