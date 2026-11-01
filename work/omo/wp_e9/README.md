# E9: 本家比較のやり直し (別素材) + INT-4 の本家比較 (2026-11-01〜02)

ここにあるのは、比較の道具・評価結果・本家の画面記録だけである (計 6 MB)。

## リポジトリに入れていないもの
素材・入力・出力 (計約 20 GB) は、統括の作業場所 `work/omo/wp_e9/` (main の作業ツリー、git の追跡外) に置いたまま。
- `media/`: Xiph derf の y4m を範囲取得したもの。SHA256 は `media/SHA256SUMS.txt`。
- `clips/`: 雑音入りの y4m と、本家に渡した 16 ビット TIFF。作り方とハッシュは `clips/<src>/manifest.json`。
- `runs/`: 当方の出力。
- `genuine/` と `genuine_noeffect/`: 本家の 16 ビット TIFF の書き出し。

素材とクリップは、`tools/make_clips.py` で作り直すとバイト単位で同じものになる。

## 素材のクレジットと出典
- SVT の park_joy / old_town_cross (1080p50): https://media.xiph.org/video/derf/ ("believed to be freely redistributable")
- Sintel © Blender Foundation | durian.blender.org (CC-BY 3.0)。予告編の y4m (Xiph derf) から、フレーム 530〜679 を使った。

## 主な文書
- `eval/E9_progressive_findings.md`: プログレッシブ 6 組の比較 (本家と当方)。
- `eval/E9_interlaced_findings.md`: インターレース 4 組の比較 (本家、当方のフレーム単位、当方のフィールド単位の 4 変種)。
- `eval/summary_*.txt`・`diag_tables_*.md`: 全表。
- `genuine_obs/`: 本家の Auto Profile の画面 (縮小した JPEG)。
- `logs/lead_*.log`: 統括が流した eval_all と summarize のログ。
